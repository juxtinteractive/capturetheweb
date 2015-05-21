# capturetheweb

A web browser that feeds whatever it renders to Syphon.


## Usage

Start browser (`cefclient.app`), then start a Syphon Client and see the browser's contents being streamed!

Send OSC messages on port `7000`. Current commands are below.

  - `string`: any string is assumed to be a URL, and the browser will navigate to it.
  - `float`: any float value will trigger a mouse move event. `0.0` is top left and `1.0` is bottom right of the window (any value inbetween is a point on the line between those two positions, e.g. `0.5` is the middle of the window).


### Syphon & OSC test apps

Simple Client is a good Syphon client ([download page](https://github.com/Syphon/Simple/releases)).

OSCTestApp is good for testing OSC ([download page](https://github.com/mrRay/vvopensource#im-not-a-programmer-i-just-want-to-download-a-midiosc-test-application)).


## Caveats

Mac only! (for now)


## Build

  1. Clone repo
  2. `cd` into the cloned directory
  3. `mkdir build && cd build`
  5. `cmake -G "Xcode" -DPROJECT_ARCH="x86_64" ..`
  6. `cmake --build .`
  7. When the build fails because `Syphon.h` doesn't have an empty line at the end of the file ... so add an empty line at the end of `<REPO>/build/vendor/syphon/src/syphon-build/build/Debug/Syphon.framework/Headers/Syphon.h`.
  8. `cmake --build .`
  9. Now the project should build, and the result should be in `<REPO>/build/cefclient/Debug/cefclient.app`
