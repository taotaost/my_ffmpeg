// my_ffmpeg.cpp : �������̨Ӧ�ó������ڵ㡣
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
	 * �ڴ˴�����������Ƶ��Ϣ�Ĵ���
	 * ȡ����pFormatCtx��ʹ��fprintf()
	 */
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
	//ǰ���av_frame_alloc������ֻ��Ϊ���AVFrame�ṹ��������ڴ棬  
	//�������͵�ָ��ָ����ڴ滹û���䡣�����av_malloc�õ����ڴ��AVFrame����������  
	//��Ȼ���仹������AVFrame��������Ա  
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
	//packet->data�е�����ڴ�������ĺ���av_read_frame�л����	 
	while(av_read_frame(pFormatCtx, packet)>=0){
		//printf("1 - packet->data 0x%x.\n", packet->data);
		if(packet->stream_index==videoindex){
				/*
				 * �ڴ˴�������H264�����Ĵ���
				 * ȡ����packet��ʹ��fwrite()
				 */
			//pFrame�е�����ڴ�������Ľ��뺯��avcodec_decode_video2�л��Զ�����	 
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if(ret < 0){
				printf("Decode Error.\n");
				return -1;
			}
			//printf("1 - pFrame->data[0] 0x%x.\n", pFrame->data[0]);
   			//srcSliceY ��ǰ����������ͼ���е���ʼ�����꣬
   			//srcSliceH ��ǰ��������ĸ߶ȡ�       
       		//srcSliceY �� srcSliceH�����ͼ���е�һ��������
			//��������Ŀ�Ŀ�����Ϊ�˲��л������Խ���֡ͼ�񻮷ֳ�N���������͸�N���̲߳��д����ӿ��ٶȡ�
       		//������������ֲ����ԣ�srcSliceY = 0�� srcSliceH = srcHeight��
			if(got_picture){
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, 
					pFrameYUV->data, pFrameYUV->linesize);
				//printf("Decoded frame index: %d\n",frame_cnt);

				/*
				 * �ڴ˴�������YUV�Ĵ���
				 * ȡ����pFrameYUV��ʹ��fwrite()
				 */

				frame_cnt++;

			}
		}
		av_free_packet(packet);
		//printf("2 - packet->data 0x%x.\n", packet->data);
	}
	
	sws_freeContext(img_convert_ctx);
	av_frame_free(&pFrameYUV);
	//printf("2 - pFrame->data[0] 0x%x.\n", pFrame->data[0]);  //��avcodec_decode_video2�з���ĵ�ַ���ڵ�
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	system("pause");
	return 0;
}

