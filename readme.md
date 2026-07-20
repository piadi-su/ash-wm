![logo](img/ashwm-logo.png)

---

# ash-wm


**ash-wm** is a suckless X11 window manager inspired by `dwm`.
It follows its philosophy: to configure or change something, you edit the source and compile it.


## dependencies
```txt
- libx11 libxrender libxcursor
```

## Features

- ash-wm uses Dwindle as its main tiling layout
- It is compact, with only is ~1800 lines of code
- IPC and EWMH support
- uses less than ~4mb of ram 


## config
To configure keys, commands, and behavior, edit the config.h header file before compiling.


## installation
```bash
git clone https://github.com/piadi-su/ash-wm.git
cd ash-wm
sudo make install

```

## DEMO

![AshWM Demo](img/demo.gif)

---
the bar on top is ashes, a bar especially made for ash-wm
Link: (https://github.com/piadi-su/ashes.git)

pls report with an issue or even a PR if you find any bugs, thx



---

## License

Released under the GPLv3 (or later) License.
