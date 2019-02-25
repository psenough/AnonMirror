AnonMirror
  
You need:
* a Mac with MacOSX (10.6 or higher)
* a Kinect
* latest Xcode (https://developer.apple.com/xcode/)
* latest LibCinder (http://libcinder.org/)
* latest BlockOpenNI (https://github.com/pixelnerve/BlockOpenNI/tree/multidevices)
* latest OpenNI + NITE + SensorKinect drivers package, ZigFu bundle package will do just fine (http://zigfu.com/en/downloads/browserplugin/)

You should:
1. boot the mac
2. plugin the kinect
3. install xcode (if you haven't already)
4. download and extract libcinder
5. download and extract blockopenni, place the dir in libcinder/blocks
6. install the openni+nite+sensorkinect drivers (if you haven't already)
7. download and extract anonmirror, place the dir in blockopenni/samples
8. launch the xcode project, figure out broken path dependencies, compile and run
9. press 'f' for fullscreen, 'l' for language toggle