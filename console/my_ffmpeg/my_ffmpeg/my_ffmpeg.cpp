// my_ffmpeg.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

extern "C"
{
	#include "libavcodec/avcodec.h"
	#include "libavformat/avformat.h"
	#include "libswscale/swscale.h"
};

int main(int argc, char *argv[])
{
	char			*pFilePath;
	AVFormatContext	*pFormatCtx;
	int				i, videoindex, frame_cnt;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	AVFrame			*pFrame,*pFrameYUV;
	uint8_t			*out_buffer;
	AVPacket		*packet;
	int				y_size;
	int				ret, got_picture;
	struct SwsContext *img_convert_ctx;

	pFilePath = "Titanic.ts";
	if(argc == 2){
		pFilePath = argv[1];
	}
	
	printf("FFmpeg configuration info:\n%s\n", avcodec_configuration());
	
	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	if(avformat_open_input(&pFormatCtx, pFilePath, NULL, NULL)!=0){
		printf("Couldn't open input stream.\n");
		return -1;
	}
	if(avformat_find_stream_info(pFormatCtx, NULL)<0){
		printf("Couldn't find stream information.\n");
		return -1;
	}
	else{
		printf("stream info: nb_streams %d\n", pFormatCtx->nb_streams);		
	}
	
	videoindex=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++){ 
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
			videoindex=i;
			break;
		}
	}
	if(videoindex==-1){
		printf("Didn't find a video stream.\n");
		return -1;
	}

	pCodecCtx=pFormatCtx->streams[videoindex]->codec;
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL){
		printf("Codec not found.\n");
		return -1;
	}
	else{
		printf("find codec %s, video w/h %d/%d.\n", 
			pCodec->name, pCodecCtx->width, pCodecCtx->height);
	}
	
	if(avcodec_open2(pCodecCtx, pCodec, NULL)<0){
		printf("Could not open codec.\n");
		return -1;
	}
	/*
	 * 在此处可添加输出视频信息的代码
	 * 取自于pFormatCtx，使用fprintf()
	 */
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
	//前面的av_frame_alloc函数，只是为这个AVFrame结构体分配了内存，  
	//而该类型的指针指向的内存还没分配。这里把av_malloc得到的内存和AVFrame关联起来。  
	//当然，其还会设置AVFrame的其他成员  
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
	packet = (AVPacket *)av_malloc(sizeof(AVPacket));

	//printf("0 - pFrame->data[0] 0x%x.\n", pFrame->data[0]);
	printf("0 - packet->data 0x%x.\n", packet->data);
	printf("--------------- File Information ----------------\n");
	av_dump_format(pFormatCtx,0,pFilePath,0);
	printf("-------------------------------------------------\n");
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL); 

	frame_cnt=0;
	//packet->data中的相关内存在下面的函数av_read_frame中会分配	 
	while(av_read_frame(pFormatCtx, packet)>=0){
		//printf("1 - packet->data 0x%x.\n", packet->data);
		if(packet->stream_index==videoindex){
				/*
				 * 在此处添加输出H264码流的代码
				 * 取自于packet，使用fwrite()
				 */
			//pFrame中的相关内存在下面的解码函数avcodec_decode_video2中会自动分配	 
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if(ret < 0){
				printf("Decode Error.\n");
				return -1;
			}
			//printf("1 - pFrame->data[0] 0x%x.\n", pFrame->data[0]);
   			//srcSliceY 当前处理区域在图像中的起始纵坐标，
   			//srcSliceH 当前处理区域的高度。       
       		//srcSliceY 和 srcSliceH定义出图像中的一个条带，
			//这样做的目的可能是为了并行化，可以将整帧图像划分成N个条带，送给N个线程并行处理，加快速度。
       		//如果不考虑这种并行性，srcSliceY = 0， srcSliceH = srcHeight。
			if(got_picture){
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, 
					pFrameYUV->data, pFrameYUV->linesize);
				//printf("Decoded frame index: %d\n",frame_cnt);

				/*
				 * 在此处添加输出YUV的代码
				 * 取自于pFrameYUV，使用fwrite()
				 */

				frame_cnt++;

			}
		}
		av_free_packet(packet);
		//printf("2 - packet->data 0x%x.\n", packet->data);
	}
	
	sws_freeContext(img_convert_ctx);
	av_frame_free(&pFrameYUV);
	//printf("2 - pFrame->data[0] 0x%x.\n", pFrame->data[0]);  //在avcodec_decode_video2中分配的地址还在的
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	system("pause");
	return 0;
}

