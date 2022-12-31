# DshowPlayer
适用于低配机器，从USB摄像头拉H264流的Qt播放器demo

## 前言  
这里为什么不用FFmpeg拉H264流呢？实测了几款USB摄像头，FFmpeg都拉不了H264流，只拉到了MJPEG流。

## 开发环境
* VS2017
* Qt5.14.1
* FFmpeg4.2.1

## 播放器流程
* dshow拉H264流
* h264_qsv/h264_cuvid硬解码h264流
* d3d渲染NV12帧

## 介绍  
### 一 拉H264流  
最近发现FFmpeg不支持从USB摄像头拉H264，只能拉到MJPEG流。而MJPEG流又不能用来推流，目测只能用H264推流。所以下面直接用DirectShow拉H264流。

dshow的用法可以参考msdn和amcap源码。

这里我创建了个CaptureWorker类，继承ISampleGrabberCB。这样就能在回调函数BufferCB中拿到H264数据包。  
然后将pBuffer和lBufferSize填入AVPacket，拿到AVPakcet后一面可以直接拿去推流，另一面可以解码得到AVFrame。
### 二 硬解码  
实践发现，FFmpeg软解码（编码也是）在低配机器（主要是CPU）跑太耗时间和CPU占用率了，特别是解码1080P，解码一帧可能要几十甚至上百毫秒，这就导致非常高的延迟了。要保证FPS是30，整个解码显示的过程不能超过33.33毫秒。使用GPU硬解码，即使是低配机器也能保证1080p解码耗时低。

可以通过以下命令查看ffmpeg支持的h264硬解码器

``` C
ffmpeg -codecs | findstr "h264"
``` 
(decoders: h264 h264_qsv h264_cuvid )  

intel GPU用h264_qsv，nvidia GPU用h264_cuvid

FFmpeg硬解码代码和软解码差不到，主要区别是查找解码器：

``` C
decoder = avcodec_find_decoder_by_name("h264_qsv");
```

一般硬解码出来的帧好像都是NV12格式
### 三 d3d渲染  
解码得到的帧，如果是硬解码是NV12格式，软解码一般是YUV格式，目前我的USB摄像头是YUVJ420P格式。但是Qt的QImage只支持RGB格式，所以要通过sws_scale转换像素格式。

同样FFmpeg的sws_scale这个API也是非常占用CPU资源的，刚开始我用libyuv代替sws_scale，libyuv耗时会低一点，但是CPU资源占用和sws_scale差不到，我的电脑跑release版本都占用10%多的CPU，这在低配机器就更多了。所以后面我改用d3d直接渲染nv12帧，这样就不需要经过像素格式转换，节省CPU资源。

调用方法很简单，InitD3D_NV12初始化，传一个QLabel的句柄和宽高，Render_NV12传AVFrame->data数据。
