# SGRandR

Simple GTK Display settings

>[NOTE]
> The wlroots backend is in progress, please be patient or use `nolibs` backend

## Features:

- [x] Change Resolution, Refresh rate, Rotation, Reflection, Scale and On/Off state
- [x] Change Resolutions, Refresh Rates list dynamically
- [x] Change Position of Display if there is more than one display
- [ ] Custom Resolution Creator

## Build

x11:
```
make x11
```

> This build requires X11/Xrandr headers

nolibs (using `xrandr`/`wlr-randr` commands):
```
make x11
```

> This build requires:
> - X11
>     - [`xrandr`](https://gitlab.freedesktop.org/xorg/app/xrandr) - Core functionality
>     - [`libdisplayinfo`](https://gitlab.freedesktop.org/emersion/libdisplay-info) (Optional) - Show output name 
> - Wayland
>     - [`wlr-randr`](https://sr.ht/~emersion/wlr-randr/) - Core functionality

## Backend differences

### X11
- Uses libxrandr

### Wayland (Not implemented)
- Will use wlr-output-management-unstable-v1

### Nolibs
- Uses and parses shell commands
- Is slower than other backend
- Cannot fetch all information

Standalone GUI for xrandr/wlr-randr