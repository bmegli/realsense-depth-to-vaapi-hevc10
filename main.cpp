/*
 * Realsense D400 depth stream to HEVC Main10 with VAAPI encoding
 *
 * Copyright 2019-2020 (C) Bartosz Meglicki <meglickib@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

/* This program is example how to use:
 * - VAAPI to hardware encode
 * - Realsense D400 depth stream
 * - to HEVC Main10 raw "video"
 * - with 10 bit depth encoding
 * - stored to disk as example
 *
 * See README.md for the details
 *
 */

// Hardware Video Encoder
#include "hve.h"

// Realsense API
#include <librealsense2/rs.hpp>

#include <fstream>
#include <iostream>
using namespace std;

const int WIDTH=0; //to be input through CLI
const int HEIGHT=0; //to be input through CLI
const int FRAMERATE=0; //to be input through CLI
const char *DEVICE=NULL; //to be input through CLI
const char *ENCODER="hevc_vaapi";//NULL for default (h264_vaapi) or FFmpeg encoder e.g. "hevc_vaapi", ...
const char *PIXEL_FORMAT="p010le"; //NULL for default (nv12) or pixel format e.g. "rgb0", ...
const int PROFILE=FF_PROFILE_HEVC_MAIN_10; //or FF_PROFILE_HEVC_MAIN, ...
const int BFRAMES=2; //max_b_frames, set to 0 to minimize latency, non-zero to minimize size
const int BITRATE=0; //average bitrate in VBR

//user supplied input
struct input_args
{
	int width;
	int height;
	int framerate;
	float depth_units;
	int seconds;
};

bool main_loop(const input_args& input, rs2::pipeline& realsense, hve *avctx, ofstream& out_file);
void dump_frame_info(const rs2::depth_frame &frame);
void init_realsense(rs2::pipeline& pipe, const input_args& input);
int process_user_input(int argc, char* argv[], input_args* input, hve_config *config);

int main(int argc, char* argv[])
{
	struct hve *hardware_encoder;
	//WIDTH, HEIGHT, FRAMERATE, DEVICE is overwritten with user input
	struct hve_config hardware_config = {WIDTH, HEIGHT, FRAMERATE, DEVICE, ENCODER, PIXEL_FORMAT, PROFILE, BFRAMES, BITRATE};
	struct input_args user_input = {0};

	ofstream out_file("output.hevc", ofstream::binary);
	rs2::pipeline realsense;

	if(process_user_input(argc, argv, &user_input, &hardware_config) < 0)
		return 1;

	if(!out_file)
		return 2;

	init_realsense(realsense, user_input);

	if( (hardware_encoder = hve_init(&hardware_config)) == NULL)
		return 3;

	bool status=main_loop(user_input, realsense, hardware_encoder, out_file);

	hve_close(hardware_encoder);

	out_file.close();

	if(status)
	{
		cout << "Finished successfully." << endl;
		cout << "Test with: " << endl << endl << "ffplay output.hevc" << endl;
	}

	return 0;
}

//true on success, false on failure
bool main_loop(const input_args& input, rs2::pipeline& realsense, hve *he, ofstream& out_file)
{
	const int frames = input.seconds * input.framerate;
	int f, failed;
	hve_frame frame = {0};
	uint16_t *color_data = NULL; //data of dummy color plane for P010LE
	AVPacket *packet;

	for(f = 0; f < frames; ++f)
	{
		rs2::frameset frameset = realsense.wait_for_frames();
		rs2::depth_frame depth = frameset.get_depth_frame();

		const int w = depth.get_width();
		const int h = depth.get_height();
		const int stride=depth.get_stride_in_bytes();

		if(!color_data)
		{  //prepare dummy color plane for P010LE format, half the size of Y
			//we can't alloc it in advance, this is the first time we know realsense stride
			//the stride will be at least width * 2 (Realsense Z16, VAAPI P010LE)
			color_data = new uint16_t[stride/2*h/2];
			for(int i=0;i<w*h/2;++i)
				color_data[i] = UINT16_MAX / 2; //dummy middle value for U/V, equals 128 << 8, equals 32768
		}

		//supply realsense frame data as ffmpeg frame data
		frame.linesize[0] = frame.linesize[1] =  stride; //the stride of Y and interleaved UV is equal
		frame.data[0] = (uint8_t*) depth.get_data();
		frame.data[1] = (uint8_t*) color_data;

		dump_frame_info(depth);

		if(hve_send_frame(he, &frame) != HVE_OK)
		{
			cerr << "failed to send frame to hardware" << endl;
			break;
		}

		while( (packet=hve_receive_packet(he, &failed)) )
		{ //do something with the data - here just dump to raw H.264 file
			cout << " encoded in: " << packet->size;
			out_file.write((const char*)packet->data, packet->size);
		}

		if(failed != HVE_OK)
		{
			cerr << "failed to encode frame" << endl;
			break;
		}
	}

	//flush the encoder by sending NULL frame
	hve_send_frame(he, NULL);
	//drain the encoder from buffered frames
	while( (packet=hve_receive_packet(he, &failed)) )
	{
		cout << endl << "encoded in: " << packet->size;
		out_file.write((const char*)packet->data, packet->size);
	}
	cout << endl;

	delete [] color_data;

	//all the requested frames processed?
	return f==frames;
}
void dump_frame_info(const rs2::depth_frame &f)
{
	cout << endl << f.get_frame_number ()
		<< ": width " << f.get_width() << " height " << f.get_height()
		<< " stride=" << f.get_stride_in_bytes() << " bytes "
		<< f.get_stride_in_bytes() * f.get_height();
}

void init_realsense(rs2::pipeline& pipe, const input_args& input)
{
	rs2::config cfg;
	cfg.enable_stream(RS2_STREAM_DEPTH, input.width, input.height, RS2_FORMAT_Z16, input.framerate);

	rs2::pipeline_profile profile = pipe.start(cfg);
	rs2::depth_sensor depth_sensor = profile.get_device().first<rs2::depth_sensor>();

	try
	{
		depth_sensor.set_option(RS2_OPTION_DEPTH_UNITS, input.depth_units);
	}
	catch(const exception &)
	{
		rs2::option_range range = depth_sensor.get_option_range(RS2_OPTION_DEPTH_UNITS);
		cerr << "failed to set depth units to " << input.depth_units << " (range is " << range.min << "-" << range.max << ")" << endl;
		throw;
	}

	cout << "Setting realsense depth units to " << input.depth_units << endl;
	cout << "This will result in:" << endl;
	cout << "-range " << input.depth_units * UINT16_MAX << " m" << endl;
	cout << "-precision " << input.depth_units*64.0f << " m (" << input.depth_units*64.0f*1000 << " mm)" << endl;
}

int process_user_input(int argc, char* argv[], input_args* input, hve_config *config)
{
	if(argc < 6)
	{
		cerr << "Usage: " << argv[0] << " <width> <height> <framerate> <depth units> <seconds> [device]" << endl;
		cerr << endl << "examples: " << endl;
		cerr << argv[0] << " 848 480 30 0.0001 5" << endl;
		cerr << argv[0] << " 848 480 30 0.0001 5 /dev/dri/renderD128" << endl;
		return -1;
	}

	config->width = input->width = atoi(argv[1]);
	config->height = input->height = atoi(argv[2]);
	config->framerate = input->framerate = atoi(argv[3]);
	input->depth_units = strtof(argv[4], NULL);
	input->seconds = atoi(argv[5]);
	config->device = argv[6]; //NULL as last argv argument, or device path


	cout << "Parsed arguments:" << endl;
	cout << "width: " << config->width << endl;
	cout << "height: " << config->height << endl;
	cout << "framerate: " << config->framerate << endl;
	cout << "depth units: " << input->depth_units << endl;
	cout << "seconds: " << input->seconds << endl;
	cout << "device: " << (config->device ? config->device : "default") << endl;

	return 0;
}
