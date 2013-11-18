// complie command
//gcc -I/usr/include -o test /usr/lib/x86_64-linux-gnu/libavcodec.a /usr/lib/x86_64-linux-gnu/libavformat.a test.c -lavformat -lavcodec -lavutil -Wall -lm -lz

#include <stdio.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>

int main(int argc, char* argv[])
{
	AVFormatContext* inputFormatContext = NULL;
	AVFormatContext* outputFormatContext = NULL;
	AVStream* inStream = NULL;
	AVStream* outStream;
	AVOutputFormat* outFormat = NULL;
	AVCodec* outCodec = NULL;
	AVPacket pkt, outpkt;
	char inputFile[200], outputFile[200];
	unsigned int i;
	unsigned int inStreamIndex = 0;
	int fps, pts = 0, last_pts = 0;
	int64_t inputEndTime;

	// intialize ffmpeg libraries	
	av_register_all();
	
	// Open input file
	sprintf(inputFile, "111.h264");
	if(avformat_open_input(&inputFormatContext, inputFile, NULL, NULL) != 0)
	{
		printf("\nopen %s file error!!!!!!!\n", inputFile);
		exit(-1);
	}

	// Find input file's stream info
	if((avformat_find_stream_info(inputFormatContext, NULL)) < 0)
	{
		printf("\nfind stream info error!!!!!!!\n");
		exit(-1);
	}
	else
	{
		printf("found inputfile's stream info\n");
	}

	// Dump information about the input file onto strerr
	av_dump_format(inputFormatContext, 0, inputFile, 0);

	for(i = 0; i < inputFormatContext->nb_streams; i++)
	{
		if(inputFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			inStreamIndex = i;
            inStream = inputFormatContext->streams[i];
            printf("found stream!!!! inStreamIndex : %d\n", inStreamIndex);

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
    if(inStreamIndex != -1 && inputFormatContext->streams[inStreamIndex])
    {
        if(inputFormatContext->streams[inStreamIndex]->r_frame_rate.num != AV_NOPTS_VALUE && inputFormatContext->streams[inStreamIndex]->r_frame_rate.den != 0)
        {
            fps =  (inputFormatContext->streams[inStreamIndex]->r_frame_rate.num)/ (inputFormatContext->streams[inStreamIndex]->r_frame_rate.den);
        }
    }
    else
    {
        fps = 30;
    }
    fps = 30;
    printf("FPS : %d\n", fps);

	// Create outputFile and allocate output format
	sprintf(outputFile, "VIDEO.ts");
	outFormat = av_guess_format(NULL, outputFile, NULL);
	printf("outFormat codec : %d\n", outFormat->video_codec);
	if(outFormat == NULL) // ts format doesnt exist
	{
		printf("output file format doesnt exist");
		exit(-1);
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
			exit(-1);
		}
	}

	// Add video stream to output format
	if((outFormat->video_codec != 0) && (inStream != NULL))
	{
		outCodec = avcodec_find_encoder(outFormat->video_codec);
		if(outCodec == NULL)
		{
			printf("could not find vid encoder");
			exit(-1);
		}
		else
		{
			printf("found out vid encoder");
			outStream = avformat_new_stream(outputFormatContext, outCodec);
            if(NULL == outStream)
            {
            	printf("failed to allocated output vid strm");
            	exit(-1);
            }
            else
            {	 // avcodec_copy_context() return 0 when ok
				if(avcodec_copy_context(outStream->codec, inputFormatContext->streams[inStreamIndex]->codec) != 0)
				{
					printf("Failed to copy context");
					exit(-1);
				}
				else
				{
					printf("TPF!!!!!!!!!!!!!!!!!!!!!!!! : %d\n", inStream->codec->ticks_per_frame);

					outStream->sample_aspect_ratio.den = inStream->codec->sample_aspect_ratio.den;
                    outStream->sample_aspect_ratio.num = inStream->codec->sample_aspect_ratio.num;
                    outStream->codec->codec_id = inStream->codec->codec_id;
                    outStream->codec->time_base.num = 2;
                    outStream->codec->time_base.den = fps * (inStream->codec->ticks_per_frame);
                    outStream->time_base.num = 1;
                    outStream->time_base.den = 30;
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
			exit(-1);
		}
		else
			printf("avio_open2 success!!!\n");
	}

    // Write the stream header, if any.
	if (avformat_write_header(outputFormatContext, NULL) < 0) 
	{
		printf("Error Occurred While Writing Header ");
		exit(-1);
	}
	else
		printf("Written Output header");

	// Now in while loop read frame using av_read_frame and write to output format using 
	// av_interleaved_write_frame(). You can use following loop
	// while(av_read_frame(inputFormatContext, &pkt) >= 0 && (m_num_frames-- > 0))
	while(av_read_frame(inputFormatContext, &pkt) >= 0)
	{
		if(pkt.stream_index == inStreamIndex)
		{
			av_rescale_q(pkt.pts, inStream->time_base, inStream->codec->time_base);
			av_rescale_q(pkt.dts, inStream->time_base, inStream->codec->time_base);
			
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
	
	// Close codec

	// free the format contexts
	avformat_free_context(inputFormatContext);
	avformat_free_context(outputFormatContext);
	
	printf("bye\n");
	
	return 0;
}