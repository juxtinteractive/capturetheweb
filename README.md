# capturetheweb

A web browser that feeds whatever it renders to Syphon.


## Usage

Start browser (`cefclient.app`), then start a Syphon Client and see the browser's contents being streamed!

Simple Client is a good Syphon client ([download](https://github.com/Syphon/Simple/releases/download/public-beta-2/Syphon.Demo.Apps.Public.Beta.2.dmg)).


## Caveats

Mac only! (for now)


## Build

  1. Clone repo
  2. `cd` into the cloned directory
  3. `mkdir build && cd build`
  5. `cmake -G "Xcode" -DPROJECT_ARCH="x86_64" ..`
  6. `cmake --build .`
  7. When the build fails because `Syphon.h` doesn't have an empty line at the end of the file ... add an empty line at the end of `Syphon.h`.
  8. `cmake --build .`
  9. Now the project should build, and the result should be in `<REPO>/build/cefclient/Debug/cefclient.app`
