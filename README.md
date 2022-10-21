# Cooledit

`cooledit` is a modern programmers text editor and integrated development environment and graphical debugger.

![Cooledit Desktop](screenshot.gif)

Contents
========

 * [Installation](#installation)
 * [Usage](#usage)
 * [History](#history)
 * [Source Map](#source-map)


### Installation
---


#### Prerequisites

`cooledit` requires the [`FreeType`](http://freetype.org/) library for fonts.
Many features do not work without the support of additional packages.
You will need the files `pkg-config`, `gdb`, `aspell`, `grotty`, `libfreetype.so.6`, and `libX11.so` to be installed on your system.
See [INSTALL.freebsd](https://github.com/paulsheer/cooledit/INSTALL.freebsd) for required FreeBSD packages, or check for an available port.
For Ubuntu install `libx11-dev`, `libfreetype-dev`, `gdb`, `aspell`, `aspell-en`, and `groff-base`.


#### Quick Build: Install From Source

```bash
$ ./configure
$ make
$ make install
```

### Usage
---

Here are some startup options for `cooledit`:


```shell
cooledit
cooledit -font large
cooledit -h
coolman gcc
coolman cooledit
```


### History
---

`cooledit` began as the text edit `mcedit` for the Midnight Commander
project in 1998 and was presented by me at the 1999 Atlanta Linux Showcase.


### Source Map
---

```shell
cooledit/
├── widget [widget library]
│   └── syntax [unit tests for syntax highlighting]
├── notosans [selected google .ttf files to cover most of unicode 15 ]
├── remotefs [remote access server]
├── man [documentation]
├── rxvt [built in shell terminal]
├── syntax [syntax highlighting rules]
└── editor [executables]
```

