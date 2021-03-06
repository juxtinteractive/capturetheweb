# capturetheweb

A web browser that feeds whatever it renders to Syphon.


## Usage

Start browser (`cefclient.app`), then start a Syphon Client and see the browser's contents being streamed!


### Control browser

The browser listens for OSC messages on port `7000`. Current commands are below.

  - `string`: any string is assumed to be a URL, and the browser will navigate to it.
  - `float`: any float value will trigger a mouse move event. `0.0` is top left and `1.0` is bottom right of the window (any value inbetween is a point on the line between those two positions, e.g. `0.5` is the middle of the window).
  - `int`: any integer value will resize the window to a square with width and height of the integer received.


### Send OSC messages from JavaScript

The browser exposes a JavaScript API that let's you send OSC messages on port `3000`.

Example: `JuxtOSC.send('crispy bacon')` sends the string `'crispy bacon'` on to `127.0.0.1:3000`. The actual message sent is `/frombrowser : <OSCVal s "crispy bacon">`.



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
  7. On successful build the app binary is in `<REPO>/build/cefclient/Debug/cefclient.app`


------

*Development and other stuffs in [the GitHub wiki](https://github.com/juxtinteractive/capturetheweb/wiki)*
