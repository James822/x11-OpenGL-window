# x11-OpenGL-window

Code for a simple X11 window on linux with an OpenGL context.

This is just the repository for the full code. See my [blog post](https://james822.github.io/posts/x11_window_opengl.html) for the full tutorial on how to get a GL context and window with linux/X11 yourself.

## BUILD STEPS

To build this program, simply use GCC with the options:

`gcc x11_window.c -lX11 -lGLX -lGL -o x11_window`

If you get any errors, ensure you have `libgl` installed, along with drivers such as `mesa`. See your package manager for more details on installation.
