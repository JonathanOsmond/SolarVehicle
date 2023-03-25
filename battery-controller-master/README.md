Battery Controller Code
=======================

This repository contains the code to be installed on the mbed that runs in the battery controller.

Compilation
-----------

This code is based around the mbed RTOS - see https://docs.mbed.com/

This guide is for Linux.

First install the build tools:
```bash
$ sudo pip install mbed-cli
```
`arm-none-eabi-gcc`, `arm-none-eabi-binutils` and `arm-none-eabi-newlib` are required, install these or similar with your distribution's package manager.

Then in a directory of your choice, run:

```bash
$ mbed import ssh://gitlab.com/ausrt/vcm.git
```

There may be instructions printed to install more packages with `pip`, if so, follow these instructions.

Next set up the target board and toolchain:
```bash
$ mbed target LPC1768
$ mbed toolchain GCC_ARM
```

Optionally add the `-G` flag to these commands to configure globally (recommended).

To compile:
```bash
$ ./build.sh # Note that mbed compile WILL NOT WORK as the code requires C++11 to be enabled - see the contents of the shell script for details
```

If this works correctly, a line will be printed giving the location of the generated binary which will look like this: `Image: ./.build/lpc1768/GCC_ARM/vcm.bin`.  Drag this file to the mbed mass storage device and reset the mbed to flash the program or alternatively find a copy of the `flash-mbed` Python script and use that instead (`flash-mbed .build/lpc1768/GCC_ARM/vcm.bin`).

Debugging
---------

To use the CMSIS-DAP on-chip debugger, install `pyocd`:

```bash
$ sudo pip install pyOCD
```

`cgdb` and `arm-none-eabi-gdb` are also required, install these or similar with your distribution's package manager.

To launch the debugger, in one terminal, run:

```bash
$ sudo pyocd-gdbserver
```

and, in the project directory, run:

```bash
$ ./gdb.sh
```

This will launch `cgdb` and upload the binary file to the mbed.

Python Issues
-------------

Please note that `mbed-cli` and `mbed-os` will not work with Python 3.

Ensure all `pip` commands are run with the `pip` from a Python 2 installation.  This can be checked with the command `pip -V`.  If it is not correct, the correct version may be called `pip2`, otherwise it may not be installed, in which case it should be installed with your distribution's package manager.

Additionally if Python 3 is the systemwide default (check using `python -V`), `mbed-cli` may not work correctly as there are several files from `mbed-os` that do not correctly specify the python version.

To fix this, create a directory containing a symlink called `python` that points to your Python 2 executable:

```bash
$ mkdir ~/mbed-python-fix
$ ln -s `which python` ~/mbed-python-fix/python
```

Then add the following to your `~/.bashrc` file:

```bash
function mbed(){
	PATH=~/mbed-python-fix:$PATH command mbed "$@"
}
export -f mbed
```
