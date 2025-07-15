# UVCast
A lightweight Linux application for easily viewing live streams from USB Video Class (UVC) devices. (Currently only tested on Ubuntu 25.04)
## Description
### What is UVCast?
UVCast is a Qt6 application designed for viewing the audio and feed from USB Video Class (UVC) devices. Most webcams and capture cards present themselves as UVC devices to the host OS, so this application should work with theoretically any USB webcam or capture card.
### Why was this made?
I am fond of USB capture cards like Genki's ShadowCast. Genki makes an application called Genki Arcade that has a lot of basic features for viewing your capture card's stream on PC and Mac but not Linux (at least, not without using a web browser.) I did not like the overhead and unecessary professional streaming options software like OBS Studio has for just doing something as simple as opening an app to play my Switch, and so UVCast was born. It is very bare bones for now, like you cant adjust volume or record your stream, but it gets the viewing job done.

## Features
 - View any UVC device in a clean and simple, resizeable window.
 - Selectable devices, pick the device you want to hear and see separately.

## Compiling Instructions
### Requirements (based on Ubuntu 25.04)
 - qt6-base-dev
 - qt6-multtimedia-dev
 - vainfo (maybe?)
 - make and cpp libraries
### Compilation
 - Change into the working directory containing `main.cpp` and `UVCast.pro`
 - Run `qmake6 ./UVCast.pro` to generate the Makefile.
 - Run `make` once the Makefile is generated.
## Running UVCast
Simply double click the binary or run the binary from the terminal. You may need to apply execution permissions for yourself.

## Contribution
Honestly contribute however you wish to this project as long as it remains within the license agreement. Send any improvements my way!
