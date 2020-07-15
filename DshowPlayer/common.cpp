#include <dshow.h>
#include "common.h"

void DeleteMediaType(AM_MEDIA_TYPE *mediaType)
{
	if (mediaType->cbFormat != 0)
	{
		CoTaskMemFree((PVOID)mediaType->pbFormat);
		mediaType->cbFormat = 0;
		mediaType->pbFormat = NULL;
	}
	if (mediaType->pUnk != NULL)
	{
		mediaType->pUnk->Release();
		mediaType->pUnk = NULL;
	}
}

// Tear down everything downstream of a given filter
void RemoveDownstream(IBaseFilter *pFilter, IGraphBuilder *pGraphBuilder)
{
	IPin *pP = 0, *pTo = 0;
	ULONG u;
	IEnumPins *pPins = NULL;
	PIN_INFO pininfo;
	HRESULT hr;

	if (!pFilter || !pGraphBuilder)
	{
		return;
	}

	hr = pFilter->EnumPins(&pPins);
	pPins->Reset();

	while (hr == NOERROR)
	{
		hr = pPins->Next(1, &pP, &u);
		if (hr == S_OK && pP)
		{
			pP->ConnectedTo(&pTo);
			if (pTo)
			{
				hr = pTo->QueryPinInfo(&pininfo);
				if (hr == NOERROR)
				{
					if (pininfo.dir == PINDIR_INPUT)
					{
						RemoveDownstream(pininfo.pFilter, pGraphBuilder);
						pGraphBuilder->Disconnect(pTo);
						pGraphBuilder->Disconnect(pP);
						//pGraphBuilder->RemoveFilter(pininfo.pFilter);
					}
					//SAFE_RELEASE(pininfo.pFilter);
				}
				SAFE_RELEASE(pTo);
			}
			SAFE_RELEASE(pP);
		}
	}
	SAFE_RELEASE(pPins);
}

HRESULT GetUnconnectedPin(IBaseFilter *pFilter, PIN_DIRECTION pinDir, IPin **ppPin)
{
	IEnumPins *pEnum = 0;
	IPin *pPin = 0;
	int nOutputIndex = 0;

	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr)) return hr;

	while (pEnum->Next(1, &pPin, NULL) == S_OK)
	{
		PIN_DIRECTION ThisPinDir;

		pPin->QueryDirection(&ThisPinDir);

		if (ThisPinDir == pinDir)
		{
			if (nOutputIndex == 0)
			{
				//TRACE("--------output index = %d---------\n", nOutputIndex);
				IPin *pTmp;
				hr = pPin->ConnectedTo(&pTmp);//already connected,not the pin we want.
				if (SUCCEEDED(hr))
				{
					SAFE_RELEASE(pTmp);
				}
				else //未连接，就是我们要找的pin，空闲的pin
				{
					SAFE_RELEASE(pEnum);
					*ppPin = pPin;
					return S_OK;   //程序找到的是第一个未连接的空闲pin
				}
			}
			nOutputIndex++;
		}
		SAFE_RELEASE(pPin); // 作用是 是pPin = NULL,pPin这个变量还存在，不会被销毁。

	}
	//获取空闲pin失败
	SAFE_RELEASE(pEnum);
	return E_NOTIMPL;
}