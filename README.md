# x11-OpenGL-window

Code for a simple X11 window on linux with an OpenGL context

## BUILD STEPS

To build this program, simply use GCC with the options:

`gcc x11_window.c -lX11 -lGLX -lGL -o x11_window`

If you get any errors, ensure you have `libgl` installed, along with drivers such as `mesa`. See your package manager for more details on installation.
