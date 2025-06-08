# SGRandR

Simple GTK xrandr-gui

>[NOTE]
> The wlroots backend is in progress, please be patient.

## TO DO: 

- [x] Get a List of Resolutions and change the display resolution
- [x] Get a List of Refresh Rates and change display refresh rate 
- [x] Change Refresh Rate list dynamically 
- [x] Button to change Rotation of display
- [x] Slider to change scale of display
- [x] Get a List of output and on/off selected output
- [x] Change Position of Display if there is more than one display
- [x] Hide Output / Position options if there is only one output
- [ ] Custom Resolution Creator

- [x] Change the list of Resolutions dynamically

## Build 

x11:
```
make x11
```

standalone GUI for xrandr, with options to Resolutions, Refresh rate, rotate, scale. on/off output and add custom resolutions