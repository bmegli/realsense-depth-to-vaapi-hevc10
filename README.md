# realsense-depth-to-vaapi-hevc10

This program is example how to use:
 - VAAPI through [HVE](https://github.com/bmegli/hardware-video-encoder) (FFmpeg) to hardware encode
 - Realsense D400 depth stream
 - to HEVC Main10 raw "video"
 - with 10 bit depth encoding
 - stored to disk as example
 
See [hardware-video-streaming](https://github.com/bmegli/hardware-video-streaming) for other related projects.

## CPU usage

As reported by `htop` (percentage used, 100% would mean core fully utilzed).

| Platform               | CPU       |  848x480@30   |
|------------------------|-----------|---------------|
| Latte Panda Alpha      | M3-7Y30   |to be done     |
| High end laptop (2017) | i7-7820HK |  10%          |

## Platforms 

Unix-like operating systems (e.g. Linux).
Tested on Ubuntu 18.04.

## Hardware

- D400 series camera
- Intel VAAPI compatible hardware encoder ([Quick Sync Video](https://ark.intel.com/Search/FeatureFilter?productType=processors&QuickSyncVideo=true)), at least Kaby Lake

Tested with D435 camera. There is possibility that it will also work with AMD hardware.

## What it does

- process user input (width, height, framerate, depth units, time to capture)
- init file for raw HEVC output
- init Realsense D400 device
- init VAAPI encoder with HVE
- read depth data from the camera
- encode to HEVC Main10 profile
- write to raw HEVC file
- cleanup

Realsense and VAAPI devices are configured to work together (no software depth processing on the host)
- VAAPI is configured for HEVC 10 bit per channel [P010LE](https://github.com/bmegli/hardware-video-encoder/issues/18#issuecomment-569501602) pixel format
- Realsense is configured to ouput P016LE (Y plane) compatible depth data
- P016LE data is binary compatible with P010LE data
- the data output by Realsense is directly fed to VAAPI hardware encoder
- the P010LE color data is filled with constant value

We have 10 bits to encode 16 bit Realsense depth data which means range/precission trade-off:
- the trade-off is controlled with depth units (0.0001 - 0.01)
- the best precision/worst range is 6.4 mm/6.5535 m (for depth units 0.0001)
- the worst precission/best range is 64 cm/655.35 m (for depth units 0.01)
- all trade-offs in between are possible

Note that this program uses video codec for depth map encoding. It will not work perfectly.

If you are not concerned with CPU usage and realtime requirements consider using [HEVC 3D extension](https://hevc.hhi.fraunhofer.de/3dhevc) reference software encoder instead. As far as I know those extensions are currently not supported by hardware encoders.

## Dependencies

Program depends on:
- [librealsense2](https://github.com/IntelRealSense/librealsense) 
- [HVE Hardware Video Encoder](https://github.com/bmegli/hardware-video-encoder)
   - FFmpeg avcodec and avutil (requires at least 3.4 version)

Install RealSense™ SDK 2.0 as described on [github](https://github.com/IntelRealSense/librealsense) 

HVE is included as submodule, you only need to meet its dependencies (FFmpeg).

HVE works with system FFmpeg on Ubuntu 18.04 and doesn't on 16.04.
Ubuntu 16.04 has outdated FFmpeg (compile your own or use non-official packages).

## Building Instructions

Tested on Ubuntu 18.04.

``` bash
# update package repositories
sudo apt-get update 
# get avcodec and avutil (and ffmpeg for testing)
sudo apt-get install ffmpeg libavcodec-dev libavutil-dev
# get compilers and make
sudo apt-get install build-essential
# get cmake - we need to specify libcurl4 for Ubuntu 18.04 dependencies problem
sudo apt-get install libcurl4 cmake
# get git
sudo apt-get install git
# clone the repository (don't forget `--recursive` for submodule!)
git clone --recursive https://github.com/bmegli/realsense-depth-to-vaapi-hevc10.git

# finally build the program
cd realsense-depth-to-vaapi-hevc10
mkdir build
cd build
cmake ..
make
```

## Running 

``` bash
# realsense-depth-to-vaapi-hevc10 <width> <height> <framerate> <depth units> <seconds> [device]
# e.g
./realsense-depth-to-vaapi-hevc10 848 480 30 0.0001 5
```

Details:
- width and height have to be supported by D400 camera and HEVC
- framerate and depth units have to be supported by D400 camera

### Troubleshooting

If you have multiple VAAPI devices you may have to specify Intel directly.

Check with:
```bash
sudo apt-get install vainfo
# try the devices you have in /dev/dri/ path
vainfo --display drm --device /dev/dri/renderD128
```

Once you identify your Intel device run the program, e.g.

```bash
./realsense-depth-to-vaapi-hevc10 848 480 30 0.0001 5 /dev/dri/renderD128
```

## Testing

Play result raw HEVC file with FFmpeg:

``` bash
# output goes to output.h264 file 
ffplay output.hevc
```

You will see:
- dark colors for near depth (near 0 value)
- light colors for far depth (near 65536 value)
- black where there is no data (0 value)
- all above is somewhat counterintuitive

## License

realsense-depth-to-vaapi-hevc10 and HVE are licensed under Mozilla Public License, v. 2.0

This is similiar to LGPL but more permissive:
- you can use it as LGPL in prioprietrary software
- unlike LGPL you may compile it statically with your code
- the license works on file-by-file basis

Like in LGPL, if you modify the code, you have to make your changes available.
Making a github fork with your changes satisfies those requirements perfectly.

Since you are linking to FFmpeg libraries. Consider also avcodec and avutil licensing.
