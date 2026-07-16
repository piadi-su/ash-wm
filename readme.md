![logo](img/ashwm-logo.png)

---

# ash-wm


**ash-wm** is a suckless X11 window manager inspired by `dwm`.
It follows its philosophy: to configure or change something, you edit the source and compile it.


## dependencies
```txt
- libx11
- libxrender
- libxcursor
```

## Features

- ash-wm use Dwindle as it's main tyling layout
- it commpact with is ~1800 lines of code
- uses just ~4mb of ram 


## config
To configure keys, commands, and behavior, edit the config.h header file before compiling.


## installation
```bash
git clone https://github.com/piadi-su/ash-wm.git
cd ash-wm
sudo make install

```

## DEMO

![AshWM Demo](img/old_demo.gif)

(the gray thing on top is a bar i'm working on)
---

pls report with and issue or even pr if you find any bugs

### things to add
- [ ] window resize
- [ ] and more


---

## License

Released under the MIT License.
