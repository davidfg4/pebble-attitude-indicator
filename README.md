Attitude Indicator
=============

This app uses the Pebble's accelerometer to simulate an attitude indicator. The main indicator shows an artificial horizon and degrees of pitch up or down. The bank indicator at the top displays left or right tilt, with markers at 10, 20, 30, 45, and 60 degrees.

By default the backlight is kept on, although it can be turned off by pressing the top right button and waiting a few seconds.

Because an accelerometer is used and not a gyroscope, this app is only accurate while stopped or at a constant velocity, not while turning. I would not recommend using this for navigation anyway. ;)

This app only works on the Pebble Time (basalt platform), not the original Pebble.

Install
-------

```
pebble build && pebble install
```
