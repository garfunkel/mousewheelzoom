# mousewheelzoom

[![Build Status](https://travis-ci.org/garfunkel/mousewheelzoom.svg)](https://travis-ci.org/garfunkel/mousewheelzoom)

gnome-shell mouse wheel magnifier/zoomer

This program is based on tobiasquinn's excellent gnome-shell-mousewheel-zoom gnome-shell plugin, which can be found here: [https://github.com/tobiasquinn/gnome-shell-mousewheel-zoom/](https://github.com/tobiasquinn/gnome-shell-mousewheel-zoom).

## Dependencies
* gnome-shell
* libx11-dev
* libglib2.0-dev

## Build
`make`

## Install
`sudo cp mousewheelzoom.desktop /etc/xdg/autostart/`
`sudo cp mousewheelzoom /usr/local/bin/`

## Start
`/usr/local/bin/mousewheelzoom`

## Usage
* Zoom in: `Alt++` or `Alt+Scroll Up`
* Zoom out: `Alt+-` or `Alt+Scroll Down`
