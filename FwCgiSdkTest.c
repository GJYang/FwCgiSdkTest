#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
// tweaked by SungboKang //////////
#include <pthread.h>
#include <unistd.h>
#define LENGTH_FFMPEG_COMMAND 60
#define FILENAME_LENGTH 10
#define MAX_QUEUE_N 10
#define FRAMECOUNT 300
#define PLAYLIST_FILENAME "playlist.m3u8"

// complie command
//gcc -I/usr/include -o test /usr/lib/x86_64-linux-gnu/libavcodec.a /usr/lib/x86_64-linux-gnu/libavformat.a test.c -lavformat -lavcodec -lavutil -Wall -lm -lz
#define USE_FFMPEG_LIBRARY
#ifdef USE_FFMPEG_LIBRARY
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/avutil.h>
#endif
///////////////////////////////////
///////////////////////////////////
#ifdef linux
#include <sys/types.h>
#include <sys/socket.h>		// basic socket definitions
#include <sys/time.h>		// timeval{} for select()
#include <netinet/in.h>		// struct sockaddr_in, htonl()
#include <arpa/inet.h>		// inet_aton(), inet_ntoa(), inet_pton(), inet_ntop()
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>
#include <net/if.h>

#include "syt_sys.h"
#include "sock_util.h"
#endif // linux

#ifdef WIN32
#ifndef _WINSOCKAPI_
	#include <winsock2.h>
#endif
	#include <io.h>
#endif

//=============================================================================
#define	__HEADER_STREAM_MODE_SUPPORT
//=============================================================================
#include "FwCgiLib.h"
#include "jpeguser.h"
#include "JesPacket.h"

// #define DECODER_ON

#ifdef DECODER_ON
#include "ffm4l.h"
#endif

//=============================================================================
//	Please set for your test environment
//=============================================================================
#define OPEN_TIMEOUT		8
#define MAX_PACK_SIZE		1024*1024
#define TARGET_IPADDR		"embedded.snut.ac.kr" //"192.168.2.20"
#define TARGET_PORT			8888
#define VSMID				0
#define PAUSE_TIME			0
#define RECV_IMAGE_CNT		150
#define	FW_USER_ID			"root"
#define	FW_PASS_WD			"root"
//=============================================================================

#ifdef WIN32
#define SOCK_STARTUP() { \
	WORD wVersionRequested; \
	WSADATA wsaData; \
	int err; \
	wVersionRequested = MAKEWORD( 2, 2 ); \
	err = WSAStartup( wVersionRequested, &wsaData ); \
	if ( err != 0 ) { \
        exit(0); \
	} \
	if (wsaData.wVersion != MAKEWORD(2,2)) {\
		WSACleanup();\
		exit(0);\
	}\
}
#define SOCK_CLEANUP()					WSACleanup()
#else
#define	SOCK_STARTUP()
#define	SOCK_CLEANUP()
#endif
//=============================================================================

static	long TestCamGetCgiV30Wp(int fwmodid, char *ip_addr, int port, int PortId, char *id, char *pwd, char *pRcvBuf, int BufLen, char *ip_addr_wp, int port_wp);
static	long TestStCtrCgiV30Wp(int fwmodid, int AppKey, int DaemonId, char *Action, char *CamList, unsigned long PauseTime,char *ip_addr, int port, char *id, char *pwd, char *pRcvBuf, int BufLen, char *ip_addr_wp, int port_wp);

#ifdef DECODER_ON
static int CodecChange(Cffm4l*, SytJesVideoCodecTypeEnum);
static int Decoding(Cffm4l* ffm4l, pjpeg_usr_t pImgBuff, char *filename);
#endif

static int SavePacket(char *pImgBuff, char *filename, int ImageSize);

// tweaked by SungboKang //////////////////////////////////////////////////////////
typedef struct {
	int head;
	int tail;
	int queue[MAX_QUEUE_N];
} CircularQueue;

typedef short Boolean;
#define FALSE 0
#define TRUE 1

void* Control_thread_function();
void* Ffmpeg_thread_function(void*);
#ifdef USE_FFMPEG_LIBRARY
	int ConvertH264toTS(int);
#endif
void Enqueue(int);
int Dequeue();
Boolean QueueIsEmpty();
Boolean QueueIsFull();
Boolean getControlThreadEndFlag();
void setControlThreadEndFlag(Boolean arg);
Boolean FileExist(char*);
void WriteM3U8(char*);
void RemoveForemostMedia(unsigned int);

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t flag_mutex = PTHREAD_MUTEX_INITIALIZER;

Boolean queueFlag = FALSE; // 'FALSE' means that there is nothing to read newly, 'FALSE' means that something is there.
Boolean controlThreadEndFlag = FALSE; // 'FALSE' means the main thread asks for finishing the control thread
CircularQueue queue = {0, 0, {0}};
////////////////////////////////////////////////////////////////////////////////////

SytJesVideoCodecTypeEnum setuped_codec;
static	char	wp_domain[] ="";
static	short	wp_port=0;

const int MAX_FRAME_SIZE = (500*1024);
const int MAX_RGB_SIZE = (3*2048*1536);

int main(int argc, char *argv[])
{
	// tweaked by SungboKang ///////////////////////////////////////////////////
	int frameCnt = 0;
	int tempSeparateH264FileNumber = 0;
	int thread_id;
	pthread_t controlThread;
	////////////////////////////////////////////////////////////////////////////

	char *			pImgBuff;
	SOCKET			StreamSock;
	char			strQuery[MAX_QUERY];

	int				AppKey;
	unsigned long	DaemonId = 0;
	int				ScanMode = SCAN_RAW_MODE;

	int				idx = 0;
	long			nRead;
	int				ImageSize;
	int				nRetCode;
	
	char			tmp_img_file[_MAX_PATH];
	
	SytJesVideoCodecTypeEnum current_codec;
	
#ifdef DECODER_ON
	Cffm4l ffm4l;
#endif
	
	int First_IFrame_Flag = 0;

	pImgBuff = (char*)malloc(MAX_PACK_SIZE);
	if(pImgBuff == NULL)
		return -1;

	
#ifdef DECODER_ON
	if (ffm4l.CreateFfm4lDecoder(CODEC_ID_H264)!=FFM4L_COMMON_SUCCESS)
	{
		printf("ffmpeg Create failed\n");
		free(pImgBuff);
		
		return -1;
	}
#endif

	current_codec = JES_VIDEO_CODEC_H264;
	setuped_codec = JES_VIDEO_CODEC_H264;

	SOCK_STARTUP();

	//=============================================================================
	//Get FlexWATCH System Information
// 	printf("FwSysGetCgiWp\n");
// 	memset(pImgBuff, 0, MAX_PACK_SIZE);
// 	nRead = FwSysGetCgiWp(VSMID, TARGET_IPADDR, TARGET_PORT, FW_USER_ID, FW_PASS_WD, pImgBuff, MAX_PACK_SIZE, wp_domain, wp_port);
// 	if(nRead > 0)
// 		printf("FwSysGetCgiWp=[%s]\n",pImgBuff);
	//=============================================================================

	//=============================================================================
	//Open Cgi Stream 
	AppKey = rand();

#ifdef __HEADER_STREAM_MODE_SUPPORT
	sprintf(	strQuery,
				STREAM_CGI_FMT_V30, 
				VSMID, 
				AppKey,
				"0",	// Port Id List (0,1,2,3)
				0x00, // 0x00 = Normal Mode, 0x0f = Header Only Mode 
				PAUSE_TIME, 
				FWCGI_VERSION);

#else	// Normal Mode Only 
	sprintf(	strQuery,
				STREAM_CGI_FMT_V30, 
				VSMID, 
				AppKey,
				"0,1",	// Port Id List (0,1,2,3)
				PAUSE_TIME, 
				FWCGI_VERSION);
#endif

	StreamSock = FwOpenGetCgiWp(TARGET_IPADDR, TARGET_PORT, FW_USER_ID, FW_PASS_WD, strQuery, OPEN_TIMEOUT, wp_domain, wp_port);
	if(StreamSock == INVALID_SOCKET)
	{
		SOCK_CLEANUP();
		free(pImgBuff);
		return 0;
	}
	//=============================================================================

	//=============================================================================
	
	// tweaked by SungboKang ////
	thread_id = pthread_create(&controlThread, NULL, Control_thread_function, NULL); // make the control thread
	if(thread_id < 0) // when thread creating doesnt work properly
	{
		perror("thread control creation error\n");
		exit(-1);
	}

	// if playlist file exists already, wipe it out and make a new one
	sprintf(tmp_img_file, PLAYLIST_FILENAME);
	if(!FileExist(tmp_img_file))
	{
		WriteM3U8("#EXTM3U");
		sprintf(tmp_img_file, "#EXT-X-TARGETDURATION:%d", FRAMECOUNT/30);
		WriteM3U8(tmp_img_file);
		WriteM3U8("#EXT-X-MEDIA-SEQUENCE:0");
	}

	#ifdef USE_FFMPEG_LIBRARY
	// Initialize ffmpeg library
	av_register_all();

	#endif
	/////////////////////////////
	
	// tweaked by GJ Yang
	// printf("%c[2J%c[0;0H",27,27);
	/////////////////////////////
	// Get Cgi Stream
	//for(idx=0 ; idx < RECV_IMAGE_CNT ; idx++)
	while(1)
	{
		// tweaked by GJ Yang
		// printf("Getting frame...\n");
		// printf("%c[s",27);fflush(stdout); //save the cursor location
		// printf("%c[%d;%dH",27, 31, 2);fflush(stdout);
		// printf("\033[%dmGetting frame...%2d\033[0m for",42, frameCnt);
		// printf("  \033[%dmVIDEO%d.h264\033[0m\n", 41, tempSeparateH264FileNumber);
		// printf("%c[u",27);	fflush(stdout);//restore the cursor locarion
		///////////////////////////////////
		
		nRetCode = FwRcvCgiStream(StreamSock, pImgBuff, MAX_PACK_SIZE, &ImageSize, &ScanMode, &DaemonId);
		
		/******************************** CODE ADDED BY SEYEON TECH START **********************************/
			p_jpeg_usr_t	pJesHeader;    // oject of the class that contains the header information
			unsigned short	JesHeaderSize; //
			char*			pH264Image;
			int				H264FrameSize;

			pJesHeader = (p_jpeg_usr_t)pImgBuff; // magic code

//			pJesHeader->start_of_jpg[0] : It mean JPEG or H.264
//			pJesHeader->start_of_jpg[1] : It mean Sequence Number 0,1,2,3... 

			JesHeaderSize = (unsigned short)(ntohs(pJesHeader->usr_length) + 2) + 1; // previously 2 // added 1 // IT WORKS!!!

			// tweaked by GJ Yang
			// printf("################################ Header Size: %d\n", JesHeaderSize);
			////////////////////////////////////
			
			// modify here remove the first 0x00 always
			// look at research log, june 20, 2012
			H264FrameSize = ImageSize - JesHeaderSize;
			pH264Image = pImgBuff + JesHeaderSize + 1;

		/******************************** CODE ADDED BY SEYEON TECH END ***********************************/

		if( nRetCode < 0 )
		{
			// tweaked by GJ Yang ///////////////////
			// printf("%c[%d;%dH",27, 32, 2);fflush(stdout);
			// printf("\033[%dmGetCgiStream Error=%d\033[0m\n", 41, nRetCode);
			////////////////////////////////////////
		}
		else
#ifdef DECODER_ON
		{
			sprintf(tmp_img_file, "img%d_3_%02d.raw",__LINE__, idx);
			
			if(IsJesPacketVideo((pjpeg_usr_t)pImgBuff) )
			{
				current_codec = GetVideoCodecTypeOfJesPacket((pjpeg_usr_t)pImgBuff);
				
				if(current_codec == setuped_codec) 
				{
					if(current_codec == JES_VIDEO_CODEC_JPEG)	
						Decoding(&ffm4l, (pjpeg_usr_t)pImgBuff, tmp_img_file);				
					else if(current_codec == JES_VIDEO_CODEC_H264)
					{
						if(IsJesPacketVideoH264IFrame((pjpeg_usr_t)pImgBuff) ) 
						{
							if(First_IFrame_Flag == 0)
								First_IFrame_Flag = 1;
						}
						else if(First_IFrame_Flag == 0)
							continue;
					
						Decoding(&ffm4l, (pjpeg_usr_t)pImgBuff, tmp_img_file);
					}
						
				}
				else if(current_codec != setuped_codec) 
				{
					CodecChange(&ffm4l, current_codec);
									

				}
			}
		}	
#else
		{				
			if(IsJesPacketVideo((pjpeg_usr_t)pImgBuff) ) 
			{
				if (GetVideoCodecTypeOfJesPacket((pjpeg_usr_t)pImgBuff) == JES_VIDEO_CODEC_JPEG)
					sprintf(tmp_img_file, "img%d_3_%02d.jpg",__LINE__, idx);	
				else if(GetVideoCodecTypeOfJesPacket((pjpeg_usr_t)pImgBuff) == JES_VIDEO_CODEC_H264) 
				{
					if(IsJesPacketVideoH264IFrame((pjpeg_usr_t)pImgBuff) ) 
					{
						if(First_IFrame_Flag == 0)
							First_IFrame_Flag = 1;
					}
					else if(First_IFrame_Flag == 0)
						continue;
					
					// sprintf(tmp_img_file, "img%d_3_%02d.264",__LINE__, idx);	// original function call. saves one frame into one file
					// sprintf(tmp_img_file, "VIDEO.h264"); // new function call. saves many frames into one file	
					
					// tweaked by SungboKang //
					sprintf(tmp_img_file, "VIDEO%d.h264", tempSeparateH264FileNumber); // newer function call. saves many frames into numbered files	
					///////////////////////////
				}
				// SavePacket(pImgBuff, tmp_img_file, ImageSize); // original function call. saves one frame into one file WITH HEADER DATA
				SavePacket(pH264Image, tmp_img_file, H264FrameSize); // new function call. saves a frame without the JES headers
				frameCnt++;
			}
		}
	
		// tweaked by SungboKang ///////////////////////////////////////////////////////////////////////
		// assume that a IP Camera sends 30 frames at any circumstances.
		// In here, it runs every 10 second(300 frames) to make a separate H264 file
		if(frameCnt == FRAMECOUNT)
		{
			while(QueueIsFull()); // waits till the queue is not full
			Enqueue(tempSeparateH264FileNumber); // put data into the queue

			tempSeparateH264FileNumber = (tempSeparateH264FileNumber + 1) % MAX_QUEUE_N;
			// tempSeparateH264FileNumber = tempSeparateH264FileNumber + 1;

			// If the next file numbered with 'tempSeparateH264FileNumber' exists, wipe it out
			// It is probable to have a flaw related to authority.
			// In addition, FileExist() works for files up to 2GB only
			sprintf(tmp_img_file, "VIDEO%d.h264", tempSeparateH264FileNumber);
			if(FileExist(tmp_img_file))
			{
				if(remove(tmp_img_file) < 0)
				{
					printf("%s file removing error\n", tmp_img_file);
					exit(-1);
				}
			}

			frameCnt = 0;
			
			First_IFrame_Flag = 0; // initialize Iframe checker

			// makes 30 h264 files then quit looping
			// It should be less than MAX_QUEUE_N, otherwise working infinitely
			// if(tempSeparateH264FileNumber == (MAX_QUEUE_N-1))
				// break;			
		}
		////////////////////////////////////////////////////////////////////////////////////////////////
#endif
	}
	//=============================================================================

	//=============================================================================
	// Control Cgi Stream

	memset(pImgBuff, 0, MAX_PACK_SIZE);

	//=============================================================================
	// Set Stream as Normal mode ( Header + Image )
//	nRead = TestStCtrCgiV30Wp(VSMID, AppKey, DaemonId, "Normal", "0,1,2,3", PAUSE_TIME,TARGET_IPADDR, TARGET_PORT, FW_USER_ID, FW_PASS_WD, pImgBuff, MAX_PACK_SIZE, wp_domain, wp_port);

	//=============================================================================
	// Set Stream as Header mode (Header Only)
//	nRead = FwStCtrCgiV30Wp(VSMID, AppKey, DaemonId, "Header", "0,1,2,3", PAUSE_TIME,TARGET_IPADDR, TARGET_PORT, FW_USER_ID, FW_PASS_WD, pImgBuff, MAX_PACK_SIZE, wp_domain, wp_port);

	//=============================================================================
	// Add Camera 3,4 --> PortId (2,3)
//	nRead = TestStCtrCgiV30Wp(VSMID, AppKey, DaemonId, "Add", "2,3", PAUSE_TIME,TARGET_IPADDR, TARGET_PORT, FW_USER_ID, FW_PASS_WD, pImgBuff, MAX_PACK_SIZE, wp_domain, wp_port);

	//=============================================================================
	// Remove Camera 2 --> PortId (1)
//	nRead = TestStCtrCgiV30Wp(VSMID, AppKey, DaemonId, "Remove", "1", PAUSE_TIME,TARGET_IPADDR, TARGET_PORT, FW_USER_ID, FW_PASS_WD, pImgBuff, MAX_PACK_SIZE, wp_domain, wp_port);

	//=============================================================================
	// Set pause time 1000 msec
//	nRead = TestStCtrCgiV30Wp(VSMID, AppKey, DaemonId, "Set", NULL, 100,TARGET_IPADDR, TARGET_PORT, FW_USER_ID, FW_PASS_WD, pImgBuff, MAX_PACK_SIZE, wp_domain, wp_port);

	//=============================================================================
	// Get Cgi Stream
#if 0
	for(idx=0 ; idx < RECV_IMAGE_CNT ; idx++)
	{

		sprintf(tmp_img_file, "img%d_3_%02d.jpg",__LINE__, idx);
#ifdef WIN32
		tmp_img_filep=fopen(tmp_img_file, "wb");
#else
		tmp_img_filep=fopen(tmp_img_file, "wb");
#endif
		nRetCode = FwRcvCgiStream(StreamSock, pImgBuff, MAX_PACK_SIZE, &ImageSize, &ScanMode, &DaemonId);
		if( nRetCode < 0 )
		{
			printf("GetCgiStream Error=%d\n", nRetCode);
		}
		else
		{
//			printf("-------- Jpeg Header --------\n");
//			PrintJpegHeader(pImgBuff);
			printf("DaemondId  : 0x%08lx\n", DaemonId);
			printf("FwModId(0~): 0x%04x\n", GET_FW_STREAM_MOD_ID( nRetCode ));
			printf("PortId(0~3): 0x%04x\n", GET_FW_STREAM_PORT_ID( nRetCode ));
			fwrite(pImgBuff, sizeof(char), ImageSize, tmp_img_filep);
		}
		fclose(tmp_img_filep);
	}
	//=============================================================================
#endif

	//=============================================================================
	// Close Cgi Stream
	FwCloseCgiWp(StreamSock);
	//=============================================================================

//	nRead = TestCamGetCgiV30Wp(VSMID, TARGET_IPADDR, TARGET_PORT, 0, FW_USER_ID, FW_PASS_WD, pImgBuff, MAX_PACK_SIZE, wp_domain, wp_port);
//	if(nRead > 0)
//		printf("FwSysGetCgiWp=[%s]\n",pImgBuff);

	// tweaked by SungboKang //////////////////////////////////////////////////
	setControlThreadEndFlag(TRUE);
	pthread_join(controlThread, NULL); // wait for the control thread

	// WriteM3U8("#EXT-X-ENDLIST");
	///////////////////////////////////////////////////////////////////////////

	SOCK_CLEANUP();
	free(pImgBuff);
	
	return 0;
}
/*********************************************************************************/


static	long TestCamGetCgiV30Wp(int fwmodid, char *ip_addr, int port, int PortId, char *id, char *pwd, char *pRcvBuf, int BufLen, char *ip_addr_wp, int port_wp)
{
	char	query[MAX_QUERY];
	long	nRead;

	sprintf(query, "/asp-get/fwcamget.asp?FwModId=%d&PortId=%d&FwCgiVer=0x%04x", fwmodid, PortId, FWCGI_VERSION);
	nRead = FwAccessAspPageWp(query ,ip_addr, port, id, pwd, pRcvBuf, BufLen, ip_addr_wp, port_wp, CGI_TIMEOUT); 

	return nRead;
}


static	long TestStCtrCgiV30Wp(int fwmodid, int AppKey, int DaemonId, char *Action, char *pPortList, unsigned long PauseTime,char *ip_addr, int port, char *id, char *pwd, char *pRcvBuf, int BufLen, char *ip_addr_wp, int port_wp)
{
	char	query[MAX_QUERY];
	long	nRead;

	if(pPortList)
	{
		sprintf(query, "/cgi-bin/fwstctr.cgi?FwModId=%d&AppKey=0x%08x&DaemonId=%d&Action=%s&PortId=%s&FwCgiVer=0x%04x", 
						fwmodid, AppKey, DaemonId, Action, pPortList, FWCGI_VERSION);
	}
	else
	{
		sprintf(query, "/cgi-bin/fwstctr.cgi?FwModId=%d&AppKey=0x%08x&DaemonId=%d&Action=%s&PauseTime=%d&FwCgiVer=0x%04x", 
						fwmodid, AppKey, DaemonId, Action, (int)PauseTime, FWCGI_VERSION);
	}
	nRead = FwAccessGetCgiWp(query ,ip_addr, port, id, pwd, pRcvBuf, BufLen, ip_addr_wp, port_wp, CGI_TIMEOUT); 
	return nRead;
}

#ifdef DECODER_ON

static int CodecChange(Cffm4l* ffm4l, SytJesVideoCodecTypeEnum codec)
{
	ffm4l->DeleteFfm4lDecoder();
					
	if(codec == JES_VIDEO_CODEC_JPEG)
	{
		if (ffm4l->CreateFfm4lDecoder(CODEC_ID_MJPEG)!=FFM4L_COMMON_SUCCESS)
		{	
			printf("ffmpeg Create failed\n");
			return -1;
		}
		setuped_codec = JES_VIDEO_CODEC_JPEG;
		printf("codec change jpeg\n");
	}
	else if(codec == JES_VIDEO_CODEC_H264)
	{
		if (ffm4l->CreateFfm4lDecoder(CODEC_ID_H264)!=FFM4L_COMMON_SUCCESS)
		{	
			printf("ffmpeg Create failed\n");
			return -1;
		}
		setuped_codec = JES_VIDEO_CODEC_H264;
		printf("codec change h264\n");
	}
	return 0;
}

static int Decoding(Cffm4l* ffm4l, pjpeg_usr_t pImgBuff, char *filename)
{
	unsigned long width = 0;
	unsigned long height = 0;
	int				nRetCode;
	FILE*			tmp_img_filep;
	
	char* bitmapBuffer = NULL;
	bitmapBuffer = (char*)malloc(MAX_RGB_SIZE);
	if (!bitmapBuffer)
	{
		printf("memory allocation failed\n");
		free(pImgBuff);
		return -1;
	}
	tmp_img_filep=fopen(filename, "wb");

	nRetCode = ffm4l->DecodeTargetBufToGeneralBuf((BYTE*)::GetPayloadOfJesPacket(pImgBuff)	,
					::GetPayloadSizeOfJesPacket(pImgBuff),
					&width,
					&height,
					0,
					(BYTE*)bitmapBuffer);
	if (nRetCode!=FFM4L_COMMON_SUCCESS)
		printf("ffmpeg.Decode failed\n");
	else
		printf("ffmpeg.Decode success[W:%d H:%d]\n", width, height);	

	fwrite(bitmapBuffer, sizeof(char), width * height * 3, tmp_img_filep); // Saved BGR 24bit Pattern
	fclose(tmp_img_filep);	

	free(bitmapBuffer);
	return 0;

}
#endif //DECODER_ON

static int SavePacket(char* pImgBuff, char *filename, int ImageSize)
{
	FILE*			tmp_img_filep;
	
	tmp_img_filep=fopen(filename, "ab");
	fwrite(pImgBuff, sizeof(char), ImageSize, tmp_img_filep);
	fclose(tmp_img_filep);

	return 0;
}

// tweaked by SungboKang //////////////////////////////////////////
#ifdef USE_FFMPEG_LIBRARY
int ConvertH264toTS(int fileNumber)
{
	AVFormatContext* inputFormatContext = NULL;
	AVFormatContext* outputFormatContext = NULL;
	AVStream* inStream = NULL;
	AVStream* outStream = NULL;
	AVOutputFormat* outFormat = NULL;
	AVCodec* outCodec = NULL;
	AVPacket pkt;
	AVPacket outpkt;
	char inputFile[200], outputFile[200];
	unsigned int i, inStreamIndex = 0;
	int fps, pts = 0, last_pts = 0;
	int64_t inputEndTime;

	// intialize ffmpeg libraries	
	av_register_all();
	
	// Open input file
	sprintf(inputFile, "VIDEO%d.h264", fileNumber);
	if(avformat_open_input(&inputFormatContext, inputFile, NULL, NULL) != 0)
	{
		printf("\nopen %s file error!!!!!!!\n", inputFile);
		return -1;
	}

	// Find input file's stream info
	if((avformat_find_stream_info(inputFormatContext, NULL)) < 0)
	{
		printf("\nfind stream info error!!!!!!!\n");
		return -1;
	}
	else
	{
		// printf("found inputfile's stream info\n");
	}

	// Dump information about the input file onto strerr
	av_dump_format(inputFormatContext, 0, inputFile, 0);

	for(i = 0; i < inputFormatContext->nb_streams; i++)
	{
		if(inputFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			inStreamIndex = i;
            inStream = inputFormatContext->streams[i];
            // printf("found stream!!!! inStreamIndex : %d\n", inStreamIndex);

            break;
		}
	}
	
	// if there is no duration info of inputFile(h264), deduce it from time_base
	if(inputFormatContext->duration == AV_NOPTS_VALUE)
	{
        if(inStreamIndex != -1 && inputFormatContext->streams[inStreamIndex])
        {	
            if(inputFormatContext->streams[inStreamIndex]->duration != AV_NOPTS_VALUE)
            {
                inputEndTime = (inputFormatContext->streams[inStreamIndex]->duration)/(inputFormatContext->streams[inStreamIndex]->time_base.den/inputFormatContext->streams[inStreamIndex]->time_base.num);
            }
        }
    }
    else
        inputEndTime = (inputFormatContext->duration)/(AV_TIME_BASE);

    // calculate frame per second(fps), but following code doesnt work properly
    // because h264 file doesn't have any context information such as fps, duration and etc.
    // therefore fps is now set arbitrarily as 30 according to h264 input file.
    // if(inStreamIndex != -1 && inputFormatContext->streams[inStreamIndex])
    // {
    //     if(inputFormatContext->streams[inStreamIndex]->r_frame_rate.num != AV_NOPTS_VALUE && inputFormatContext->streams[inStreamIndex]->r_frame_rate.den != 0)
    //     {
    //         fps =  (inputFormatContext->streams[inStreamIndex]->r_frame_rate.num)/ (inputFormatContext->streams[inStreamIndex]->r_frame_rate.den);
    //     }
    // }
    // else
    // {
    //     fps = 30;
    // }
    fps = 30;

	// Create outputFile and allocate output format
	sprintf(outputFile, "VIDEO%d.ts", fileNumber);
	outFormat = av_guess_format(NULL, outputFile, NULL);
	if(outFormat == NULL) // ts format doesnt exist
	{
		printf("output file format doesnt exist");
		return -1;
	}
	else // ts format exists
	{
		outputFormatContext = avformat_alloc_context();
		if(outputFormatContext != NULL)
		{
			outputFormatContext->oformat = outFormat;
			snprintf(outputFormatContext->filename, sizeof(outputFormatContext->filename), "%s", outputFile); // ?????
		}
		else
		{
			printf("outputFormatContext allocation error");
			return -1;
		}
	}

	// Add video stream to output format
	if((outFormat->video_codec != 0) && (inStream != NULL))
	{
		outCodec = avcodec_find_encoder(outFormat->video_codec);
		if(outCodec == NULL)
		{
			printf("could not find vid encoder");
			return -1;
		}
		else
		{
			printf("found out vid encoder : %s\n", outCodec->name);
			outStream = avformat_new_stream(outputFormatContext, outCodec);
            if(NULL == outStream)
            {
            	printf("failed to allocated output vid strm");
            	return -1;
            }
            else
            {	 // avcodec_copy_context() return 0 when ok
				if(avcodec_copy_context(outStream->codec, inputFormatContext->streams[inStreamIndex]->codec) != 0)
				{
					printf("Failed to copy context");
					return -1;
				}
				else
				{
					// time_base is used to calculate when to decode and show the frame
					outStream->sample_aspect_ratio.den = outStream->codec->sample_aspect_ratio.den;
                    outStream->sample_aspect_ratio.num = inStream->codec->sample_aspect_ratio.num;
                    outStream->codec->codec_id = inStream->codec->codec_id;
                    outStream->codec->time_base.num = 2;
                    outStream->codec->time_base.den = fps * (inStream->codec->ticks_per_frame);
                    outStream->time_base.num = 1;
                    outStream->time_base.den = 1000;
                    outStream->r_frame_rate.num = fps;
                    outStream->r_frame_rate.den = 1;
                    outStream->avg_frame_rate.num = fps;
                    outStream->avg_frame_rate.den = 1;
				}
			}
		}
	}
	else
		printf("stream context outputting fail !!!!!!!!!!!!!!!!\n");

	// in avformat.h, #define AVFMT_NOFILE 0x0001.
	// Demuxer will use avio_open, no opened file should be provided by the caller.
	if(!(outFormat->flags & AVFMT_NOFILE))
	{
		if (avio_open2(&outputFormatContext->pb, outputFile, AVIO_FLAG_WRITE, NULL, NULL) < 0) 
		{
			printf("Could Not Open File ");
			return -1;
		}
		// else
		// 	printf("avio_open2 success!!!\n");
	}

    // Write the stream header, if any.
	if (avformat_write_header(outputFormatContext, NULL) < 0)
	{
		printf("Error Occurred While Writing Header ");
		return -1;
	}
	else
		// printf("Written Output header");

	// Now in while loop read frame using av_read_frame and write to output format using 
	// av_interleaved_write_frame(). You can use following loop
	// while(av_read_frame(inputFormatContext, &pkt) >= 0 && (m_num_frames-- > 0))
	while(av_read_frame(inputFormatContext, &pkt) >= 0)
	{
		if(pkt.stream_index == inStreamIndex)
		{
			// av_rescale_q(pkt.pts, inStream->time_base, inStream->codec->time_base);
			// av_rescale_q(pkt.dts, inStream->time_base, inStream->codec->time_base);
			
			av_init_packet(&outpkt);
			
			if(pkt.pts != AV_NOPTS_VALUE) // AV_NOPTS_VALUE means undefined timestamp value
			{
				if(last_pts == pts)
				{
					pts++;
					last_pts = pts;
				}
				
				outpkt.pts = pts;   
			}
			else // pkt.pts is undefined
				outpkt.pts = AV_NOPTS_VALUE;

			if(pkt.dts == AV_NOPTS_VALUE) // if pkt's dts value is undefined
				outpkt.dts = AV_NOPTS_VALUE;
			// if pkt's dts value is defined with a value
			else
				outpkt.dts = pts;

			outpkt.data = pkt.data;
			outpkt.size = pkt.size;
			outpkt.stream_index = pkt.stream_index;
			outpkt.flags |= AV_PKT_FLAG_KEY; // #define AV_PKT_FLAG_KEY 0x0001, which means the packet contains a keyframe
			last_pts = pts;

			if(av_interleaved_write_frame(outputFormatContext, &outpkt) < 0)
				printf("failed video write\n");
			else
			{
				// printf("video write ok!!!!\n");
				outStream->codec->frame_number++;
			}
			
			av_free_packet(&outpkt);
			av_free_packet(&pkt);
		}
	}

	// Finally write trailer and clean up everything
	av_write_trailer(outputFormatContext);
	
	// free the memory
	if(inStream && inStream->codec)
		avcodec_close(inStream->codec);
	if(inputFormatContext)
		avformat_close_input(&inputFormatContext);
	if(outStream && outStream->codec)
		avcodec_close(outStream->codec);
	if(outputFormatContext)
	{
		avformat_close_input(&outputFormatContext);
		outputFormatContext = NULL;
	}
	
	return 0;
}
#endif

void* Ffmpeg_thread_function(void* arg)
{
	int fileNumber = *((int *)arg);
	#ifdef USE_FFMPEG_LIBRARY
		char addEXTINF[LENGTH_FFMPEG_COMMAND];
		char fileName[FILENAME_LENGTH];
	#else
		char commandFFmpeg[LENGTH_FFMPEG_COMMAND], addEXTINF[LENGTH_FFMPEG_COMMAND];
		char fileName[FILENAME_LENGTH];
	#endif
	
	// tweaked by GJ Yang /////
	// printf("%c[2J%c[0;0H",27,27);fflush(stdout);
	// printf("%c[%d;%dH",27, 1, 1);fflush(stdout);
	///////////////////////////
	
	#ifdef USE_FFMPEG_LIBRARY // use FFmpeg Library
		
		sprintf(fileName, "VIDEO%d.ts", fileNumber);
		if(FileExist(fileName))
		{
			if(remove(fileName) < 0)
			{
				printf("%s file removing error\n", fileName);
				exit(-1);
			}
		}
		
		if(ConvertH264toTS(fileNumber) < 0)
				printf("converting h264 to ts error!!!! it has been skipped\n");
	#else // use command line
		sprintf(commandFFmpeg, "ffmpeg -r 30 -i VIDEO%d.h264 -vcodec copy -f mpegts -y VIDEO%d.ts &", fileNumber, fileNumber);
		system(commandFFmpeg); // execute ffmpeg command
	#endif
		sprintf(addEXTINF, "#EXTINF:%0.2f,\nhttp://embedded.snut.ac.kr:8989/hls_test/VIDEO%d.ts", ((float)FRAMECOUNT/30.0), fileNumber);
		WriteM3U8(addEXTINF);

	pthread_exit(0);
}


void* Control_thread_function()
{
	pthread_t threadFFmpeg; // declare FFmpeg thread object
	int thread_id, thread_arg;
	unsigned int sequenceCnt = 0, sequenceFlag = 0;

	 // This loop ends when 'controlThreadEndFlag' is TRUE and the queue is empty as well
	while(!(getControlThreadEndFlag() && QueueIsEmpty()))
	{
		if(QueueIsEmpty() == FALSE) // there is something to read in the queue
		{
			// create and run a new thread
			thread_arg = Dequeue();
			thread_id = pthread_create(&threadFFmpeg, NULL, Ffmpeg_thread_function, (void *)&thread_arg);
			if(thread_id < 0) // when thread doesnt work properly
			{
				perror("FFmpeg thread create error\n");
				exit(-1);
			}

			pthread_join(threadFFmpeg, NULL);

			// remove the foremost media segment in playlist.m3u8
			if(sequenceFlag < MAX_QUEUE_N)
				sequenceFlag++;
			else
				RemoveForemostMedia(sequenceCnt++);
		}
		// else{continue}; // there is nothing to read in the queue
	}

	pthread_exit(0);
}

void Enqueue(int data) // put H264 VIDEO File number into the queue
{
	pthread_mutex_lock(&queue_mutex); // lock queue

	queue.queue[(queue.tail)] = data;
	queue.tail = ((queue.tail) + 1) % MAX_QUEUE_N;

	pthread_mutex_unlock(&queue_mutex); // unlock queue
	
	return;
}

int Dequeue() // get the earliest H264 VIDEO File's number 
{
	int fileNumber;

	pthread_mutex_lock(&queue_mutex); // lock queue

	fileNumber = queue.queue[queue.head];
	queue.head = ((queue.head) + 1) % MAX_QUEUE_N;

	pthread_mutex_unlock(&queue_mutex); // unlock queue
	
	return fileNumber;
}

Boolean QueueIsEmpty()
{
	pthread_mutex_lock(&queue_mutex); // lock queue

	if(queue.head == queue.tail)
	{
		pthread_mutex_unlock(&queue_mutex); // unlock queue
		
		return TRUE;
	}
		
	pthread_mutex_unlock(&queue_mutex); // unlock queue
	
	return FALSE;
}

Boolean QueueIsFull()
{
	pthread_mutex_lock(&queue_mutex); // lock queue

	if(((queue.tail + 1) % MAX_QUEUE_N) == queue.head) // the last slot should be empty to check fullness
	{
		pthread_mutex_unlock(&queue_mutex); // unlock queue
		
		return TRUE;

	}
		
	pthread_mutex_unlock(&queue_mutex); // unlock queue
		
	return FALSE;
}


Boolean getControlThreadEndFlag() // getter of controlThreadEndFlag: to check whether finish the control thread
{
	Boolean returnValue;
	
	pthread_mutex_lock(&flag_mutex); // lock flag

	returnValue = queueFlag;
	
	pthread_mutex_unlock(&flag_mutex); // unlock flag
	
	return returnValue;
}

void setControlThreadEndFlag(Boolean arg) // setter of controlThreadEndFlag
{
	pthread_mutex_lock(&flag_mutex); // lock flag

	queueFlag = arg;
	
	pthread_mutex_unlock(&flag_mutex); // unlock flag
	
	return;
}

// FileExist() works for files up to 2GB only
Boolean FileExist(char *fileName)
{
	struct stat buffer;

	// get file's status and return it
	return(stat(fileName, &buffer) == 0);
}

void WriteM3U8(char* printData)
{
	FILE *fp = fopen(PLAYLIST_FILENAME, "a");

	fprintf(fp, "%s\n", printData);
	
	fclose(fp);
}

void RemoveForemostMedia(unsigned int sequenceCnt)
{
	int fileSize;
	int cnt = 0;
	int retCnt = 0;
	char* fileString;
	char printData[30];
	FILE *fp;
	
	fp = fopen(PLAYLIST_FILENAME, "r");

	// calculate the size of playlist.m3u8
	fseek(fp, 0, SEEK_END);
	fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	// copy the file content to fileString
	// + 2 is for a case of raising TARGETSEQUENCE digit
	// fileString = (char*)malloc(sizeof(char) * (fileSize + 1));
 	fileString = (char*)malloc(sizeof(char) * (fileSize));
 	fread(fileString, sizeof(char), fileSize, fp);
	// fileString[fileSize + 1] = '\0';

	// truncate file to zero length("w" argument) and reopen
	freopen(PLAYLIST_FILENAME, "w", fp);
	
	while(cnt < fileSize)
	{
		if(retCnt < 2 || retCnt > 4)
			fputc(fileString[cnt], fp);
		else if(retCnt == 2)
		{
			sprintf(printData, "#EXT-X-MEDIA-SEQUENCE:%d\n", sequenceCnt);
			fputs(printData, fp);
			while(fileString[cnt] != '\n')
				cnt++;
		}
		
		if(fileString[cnt] == '\n')
			retCnt++;

		cnt++;
	}
	
	fclose(fp);

	free(fileString);
}
///////////////////////////////////////////////////////////////////