# DshowPlayer
适用于低配机器，从USB摄像头拉H264流的Qt播放器demo

##### 前言  
这里为什么不用FFmpeg拉H264流呢？实测了几款USB摄像头，FFmpeg都拉不了H264流，只拉到了MJPEG流。

##### 开发环境
* VS2017
* Qt5.14.1
* FFmpeg4.2.1

##### 播放器流程
* dshow拉H264流
* h264_qsv/h264_cuvid硬解码h264流
* d3d渲染NV12帧

