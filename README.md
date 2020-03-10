# realsense-depth-to-vaapi-hevc10

This program is example how to use:
 - VAAPI through [HVE](https://github.com/bmegli/hardware-video-encoder) (FFmpeg) to hardware encode
 - Realsense D400 depth stream
 - to HEVC Main10 raw "video"
 - with 10 bit depth encoding
 - stored to disk as example
 
See [benchmarks](https://github.com/bmegli/realsense-depth-to-vaapi-hevc10/wiki/Benchmarks) on wiki for CPU/GPU usage.

See [how it works](https://github.com/bmegli/realsense-depth-to-vaapi-hevc10/wiki/How-it-works) on wiki to understand the code.

See [hardware-video-streaming](https://github.com/bmegli/hardware-video-streaming) for other related projects.

See [video](http://www.youtube.com/watch?v=qnTxhfNW-_4) for wireless point cloud streaming example.

## Warning

This program uses video codec for depth map encoding. It will not work perfectly.

Consider [HEVC 3D extension](https://hevc.hhi.fraunhofer.de/3dhevc) software encoder if you are not concerned with:
- CPU usage
- realtime requirements

## Platforms 

Unix-like operating systems (e.g. Linux).
Tested on Ubuntu 18.04.

## Hardware

- D400 series camera
- Intel VAAPI compatible hardware encoder ([Quick Sync Video](https://ark.intel.com/Search/FeatureFilter?productType=processors&QuickSyncVideo=true)), at least Kaby Lake

Tested with D435 camera.

## Dependencies

Program depends on:
- [librealsense2](https://github.com/IntelRealSense/librealsense) 
- [HVE Hardware Video Encoder](https://github.com/bmegli/hardware-video-encoder)
   - FFmpeg avcodec and avutil (requires at least 3.4 version)

Install RealSense™ SDK 2.0 as described on [github](https://github.com/IntelRealSense/librealsense) 

HVE is included as submodule, you only need to meet its dependencies (FFmpeg).

HVE works with system FFmpeg on Ubuntu 18.04 and doesn't on 16.04 (outdated FFmpeg and VAAPI ecosystem).

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
# output goes to output.hevc file
ffplay output.hevc
```

You will see:
- dark colors for near depth (near 0 value)
- light colors for far depth (near 65472 value)
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

## Beyond Depth Encoding

The next logical step is to add texture to the depth map.

[RNHVE](https://github.com/bmegli/realsense-network-hardware-video-encoder) already does that. If you are interested see:
- its pipelines [documentation](https://github.com/bmegli/realsense-network-hardware-video-encoder/wiki/How-it-works#encoding-pipelines)
- specifically the [texture encoding](https://github.com/bmegli/realsense-network-hardware-video-encoder/wiki/Infrared-encoding-in-P010LE-UV-plane)
- the [video](https://www.youtube.com/watch?v=zVIuvWMz5mU)
