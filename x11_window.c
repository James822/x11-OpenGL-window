#include <stdio.h>
#include <X11/Xlib.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <GL/gl.h>
#include <GL/glext.h>

#define TRUE 1
#define FALSE 0

int search_str_in_str(const char* string, const char* search_string)
{
      for(int i = 0; string[i] != '\0'; ++i) {
	    if(string[i] == search_string[0]) {
		  // now we can start to see if the rest is search_string
		  int ans = TRUE;
		  for(int j = 0; search_string[j] != '\0'; ++j) {
			if(string[i + j] != search_string[j]) {
			      ans = FALSE;
			      break;
			}
		  }
		  if(ans == TRUE) {
			return TRUE;
		  }
	    }
      }
      return FALSE;
}


int main()
{
      int display_exists = FALSE;
      int window_exists = FALSE;
      int glx_window_exists = FALSE;
      int glx_context_exists = FALSE;
      int glx_context_current = FALSE;

      
      /* @@ opening connection to X server */
      Display* display = NULL;
      int default_screen_id = 0;
      
      display = XOpenDisplay(NULL);
      if(display == NULL) {
	    printf("ERROR: failed to open X11 display, display name: \"%s\"\n", XDisplayName(NULL));
	    goto CLEANUP_AND_EXIT;
      } else {
	    display_exists = TRUE;
	    default_screen_id = XDefaultScreen(display);
      }
      /* @! */


      /* @@ glx initial setup */
      int error_base;
      int event_base;
      if(glXQueryExtension(display, &error_base, &event_base) != True) {
	    printf("ERROR: GLX extension not supported for this X Server\n");
	    goto CLEANUP_AND_EXIT;
      }

      int glx_major_version;
      int glx_minor_version;
      if(glXQueryVersion(display, &glx_major_version, &glx_minor_version) != True) {
	    printf("ERROR: glXQueryVersion() failed\n");
	    goto CLEANUP_AND_EXIT;
      }
      if( glx_major_version < 1 || (glx_major_version == 1 && glx_minor_version < 4) ) {
	    /* The reason why we require GLX 1.4 (or higher, although it's doubtful a new version will be released) is because the GLX_ARB_create_context/GLX_ARB_create_context_profile extensions (which we need) requires GLX 1.4, and we need this extension in order to create a modern OpenGL context, which is what we are doing. "Modern" is somewhat of an outdated term as GLX 1.4 was released in 2005/2006, so there's nothing really unportable as we require the user to have hardware that can at least support this "modern" OpenGL. */
	    printf("ERROR: GLX version is less than 1.4\n");
	    goto CLEANUP_AND_EXIT;
      }
      /* @! */


      /* @@ Checking for GLX extensions */
      const char* glx_extensions_string = glXQueryExtensionsString(display, default_screen_id);      

      /* @NOTE: GLX_ARB_create_context_profile is absolutely neccessary in order to create a GL context, we cannot proceed without it. Technically, we need only check if GLX_ARB_create_context is supported rather than GLX_ARB_create_context_profile, because the former implies the latter is supported, IF the implementation supports OpenGL 3.2 or later, but we are definitely using a version higher than OpenGL 3.2, version 4 actually. Still, just in case, we explicity check if GLX_ARB_create_context_profile is supported to be as robust as possible. See this documentation for more details: https://registry.khronos.org/OpenGL/extensions/ARB/GLX_ARB_create_context.txt, specifically check the "Dependencies on OpenGL 3.2 and later OpenGL versions" section. */
      if(search_str_in_str(glx_extensions_string, "GLX_ARB_create_context_profile") != TRUE) {
	    printf("ERROR: \"GLX_ARB_create_context\" extension not supported, cannot create GL context!");
	    goto CLEANUP_AND_EXIT;
      }
      /* EXT_swap_control give us the ability to synchronize the swap (v-sync) by an integer amount of frames greater or equal to 0. */
      if(search_str_in_str(glx_extensions_string, "GLX_EXT_swap_control") != TRUE) {
	    printf("ERROR: \"GLX_EXT_swap_control\" extension not supported!");
	    goto CLEANUP_AND_EXIT;
      }
      /* This allows us to use adaptive v-sync for when we miss a swap by specifying negative integer intervals. Adaptive vsync is a generally a good compromise with double-buffering. This extension depends upon the GLX_EXT_swap_control extension to be available/supported. */
      if(search_str_in_str(glx_extensions_string, "GLX_EXT_swap_control_tear") != TRUE) {
	    printf("ERROR: \"GLX_EXT_swap_control_tear\" extension not supported!");
	    goto CLEANUP_AND_EXIT;
      }
      /* @! */


      /* @@ loading GLX extension procedures (we need the "glxext.h" header for the function typedefs and declarations). */
      PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = NULL;
      PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = NULL;
      
      glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress((const GLubyte *)"glXCreateContextAttribsARB");      
      glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddress((const GLubyte *)"glXSwapIntervalEXT");

      if(glXCreateContextAttribsARB == NULL) {
	    printf("ERROR: glXCreateContextAttribsARB() function pointer NULL, cannot create GL context!\n");
	    goto CLEANUP_AND_EXIT;
      }
      if(glXSwapIntervalEXT == NULL) {
	    printf("ERROR: \"glXSwapIntervalEXT\" function pointer NULL\n");
	    goto CLEANUP_AND_EXIT;
      }
      /* @! */


      /* @@ selecting a GLX FB Config */
      /* This fbc attrib array is what specifies the kind of framebuffer we will get, so it's important to specify everything we need and not leave anything to defaults (unless we don't care what it is). It's possible that we specify a configuration that isn't supported, which means either we need to choose another one that is supported, or we deem the platform to be unsupported by our application. */
      int fbc_attribs[] = {
	    GLX_LEVEL, 0,
	    GLX_DOUBLEBUFFER, True,
	    GLX_STEREO, False,
	    GLX_RED_SIZE, 8,
	    GLX_GREEN_SIZE, 8,
	    GLX_BLUE_SIZE, 8,
	    GLX_ALPHA_SIZE, 8,
	    GLX_DEPTH_SIZE, 24,
	    GLX_STENCIL_SIZE, 8,
	    GLX_RENDER_TYPE, GLX_RGBA_BIT,
	    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
	    GLX_X_RENDERABLE, True,
	    GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
	    GLX_CONFIG_CAVEAT, GLX_NONE,
	    GLX_TRANSPARENT_TYPE, GLX_NONE,
	    None /* we must put this "None" value here as this array is passed in with no size parameter */
      };
      
      int num_fb_configs = 0;
      GLXFBConfig *fb_configs = glXChooseFBConfig(display, default_screen_id, fbc_attribs, &num_fb_configs);
      if(fb_configs == NULL || num_fb_configs == 0) {
	    printf("ERROR: glXChooseFBConfig() failed, or it returned no matching fb configs\n");
	    goto CLEANUP_AND_EXIT;
      }
      GLXFBConfig best_fb_config = fb_configs[0]; /* for now we are just picking the first FBConfig in the array, but you may want to actually sort through the array and select the one to your liking. */
      XFree(fb_configs);
      /* @! */


      /* @@ creating X11 Window and GLX window */
      Window parent_window = XDefaultRootWindow(display);
      Window window;
      int window_width = 960;
      int window_height = 540;
      
      XVisualInfo* visual_info = glXGetVisualFromFBConfig(display, best_fb_config); /* this GLX function gives us the structure we need to pass into X11 window create function so that our GLX fb config is consistent with the X11 window config. */
      if(visual_info == NULL) {
	    printf("ERROR: glXGetVisualFromFBConfig() failed to get a XVisualInfo struct\n");
	    goto CLEANUP_AND_EXIT;
      }

      XSetWindowAttributes window_attributes;
      window_attributes.background_pixmap = None;
      window_attributes.background_pixel = XWhitePixel(display, default_screen_id);

      Colormap cmap = XCreateColormap(display, parent_window, visual_info->visual, AllocNone);
      window_attributes.colormap = cmap;

      window = XCreateWindow(display, parent_window, 0, 0, (unsigned int)window_width, (unsigned int)window_height, 0, visual_info->depth, InputOutput, visual_info->visual, CWBackPixmap | CWBackPixel | CWColormap, &window_attributes);
      window_exists = TRUE;
      XFree(visual_info);

      
      /* now we create the GLX window which is basically just a handle to our X11 window, but it does accomplish very important things so we absolutely need it */
      /* we need to use the same exact FB config as used to create the context AND to create the X11 window, so we just pass that in here as well. */
      GLXWindow glx_window = glXCreateWindow(display, best_fb_config, window, NULL);
      glx_window_exists = TRUE;

      /* from now we use glx_window instead of window for any GLX/GL functions. For normal X functions, we continue to use the normal X window. */
      /* @! */


      /* @@ Creating GLX (GL) context, and making context current. */
      int context_attribs[] = {
	    /* @@ with GLX_CONTEXT_MAJOR_VERSION_ARB and GLX_CONTEXT_MAJOR_MINOR_VERSION, the context creation function may actually give us a higher version as long as it is backwards compatible, so we need to query the real OpenGL version with glGet. If we ask for 4.6, it's unlikely we'll get a higher version as they probably won't update OpenGL further. */
	    GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
	    GLX_CONTEXT_MINOR_VERSION_ARB, 6,
	    /* @! */
	    
	    GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,	    
	    GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB, /* @@ Forward compatible bit is set so that we avoid deprecated functions. This is optional and won't really change anything since OpenGL isn't going to change. */
	    None
      };
      
      GLXContext glx_context = glXCreateContextAttribsARB(display, best_fb_config, NULL, True, context_attribs);
      if(glx_context == NULL) {
	    printf("ERROR: failed to create glx context with glXCreateContextAttribsARB()\n");
	    goto CLEANUP_AND_EXIT;
      } else {
	    glx_context_exists = TRUE;
      }
      
      if(glXIsDirect(display, glx_context) != True) {
	    printf("ERROR: OpenGL Context was NOT created in direct mode (DRI). It is not supported to run a networked X server setup with the client and server not on the same device. Otherwise, some other error occured.\n");
	    goto CLEANUP_AND_EXIT;
      }
      
      if(glXMakeCurrent(display, glx_window, glx_context) != True) {
	    printf("ERROR: failed to make context current\n");	    
	    goto CLEANUP_AND_EXIT;
      } else {
	    glx_context_current = TRUE;
      }
      /* @! */


      /* @@ loading OpenGL procedures (we need the "glext.h" header for the function typedefs and declarations, "glext.h" and "glxext.h" are NOT the same, they are spelled very similar but the former is for OpenGL and the latter is for GLX). */
      PFNGLGETSTRINGIPROC glGetStringi = NULL;
      PFNGLGENBUFFERSPROC glGenBuffers = NULL;
      PFNGLBINDBUFFERPROC glBindBuffer = NULL;
      PFNGLBUFFERDATAPROC glBufferData = NULL;
      PFNGLCREATESHADERPROC glCreateShader = NULL;
      PFNGLSHADERSOURCEPROC glShaderSource = NULL;
      PFNGLCOMPILESHADERPROC glCompileShader = NULL;
      PFNGLCREATEPROGRAMPROC glCreateProgram = NULL;
      PFNGLATTACHSHADERPROC glAttachShader = NULL;
      PFNGLLINKPROGRAMPROC glLinkProgram = NULL;
      PFNGLDELETESHADERPROC glDeleteShader = NULL;
      PFNGLUSEPROGRAMPROC glUseProgram = NULL;
      PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = NULL;
      PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = NULL;
      PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = NULL;
      PFNGLBINDVERTEXARRAYPROC glBindVertexArray = NULL;
      
      glGetStringi = (PFNGLGETSTRINGIPROC)glXGetProcAddress((const GLubyte *)"glGetStringi");
      glGenBuffers = (PFNGLGENBUFFERSPROC)glXGetProcAddress((const GLubyte *)"glGenBuffers");
      glBindBuffer = (PFNGLBINDBUFFERPROC)glXGetProcAddress((const GLubyte *)"glBindBuffer");
      glBufferData = (PFNGLBUFFERDATAPROC)glXGetProcAddress((const GLubyte *)"glBufferData");
      glCreateShader = (PFNGLCREATESHADERPROC)glXGetProcAddress((const GLubyte *)"glCreateShader");
      glShaderSource = (PFNGLSHADERSOURCEPROC)glXGetProcAddress((const GLubyte *)"glShaderSource");
      glCompileShader = (PFNGLCOMPILESHADERPROC)glXGetProcAddress((const GLubyte *)"glCompileShader");
      glCreateProgram = (PFNGLCREATEPROGRAMPROC)glXGetProcAddress((const GLubyte *)"glCreateProgram");
      glAttachShader = (PFNGLATTACHSHADERPROC)glXGetProcAddress((const GLubyte *)"glAttachShader");
      glLinkProgram = (PFNGLLINKPROGRAMPROC)glXGetProcAddress((const GLubyte *)"glLinkProgram");
      glDeleteShader = (PFNGLDELETESHADERPROC)glXGetProcAddress((const GLubyte *)"glDeleteShader");
      glUseProgram = (PFNGLUSEPROGRAMPROC)glXGetProcAddress((const GLubyte *)"glUseProgram");
      glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)glXGetProcAddress((const GLubyte *)"glVertexAttribPointer");
      glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)glXGetProcAddress((const GLubyte *)"glEnableVertexAttribArray");
      glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)glXGetProcAddress((const GLubyte *)"glGenVertexArrays");
      glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)glXGetProcAddress((const GLubyte *)"glBindVertexArray");
      /* @! */


      /* @@ configuring window parameters */
      XStoreName(display, window, "X11 OpenGL Window");
      XSelectInput(display, window, KeyPressMask); /* so we can get keyboard inputs in the X11 event loop */

      /* even though we checked to see if the extension is present, we should still check again with glXQueryDrawable() to make sure it is there. This is a robust and defensive way of programming that you need to do. */
      unsigned int query_val;
      glXQueryDrawable(display, glx_window, GLX_LATE_SWAPS_TEAR_EXT, &query_val);
      if(query_val == 1) { /* late swaps is supported, i.e. adaptive v-sync */
	    glXSwapIntervalEXT(display, glx_window, -1); /* -1 is adaptive v-sync */	    
	    glXQueryDrawable(display, glx_window, GLX_SWAP_INTERVAL_EXT, &query_val);
	    if((int)query_val != -1) { /* even again here we check to make sure it was set properly */
		  printf("ERROR: failed to set swapinterval val correctly. \"-1\" was requested but we did not get it\n");
		  goto CLEANUP_AND_EXIT;
	    }
      } else {
	    glXSwapIntervalEXT(display, glx_window, 1); /* 1 is normal v-sync */
	    glXQueryDrawable(display, glx_window, GLX_SWAP_INTERVAL_EXT, &query_val);
	    if(query_val != 1) {
		  printf("ERROR: failed to set swapinterval val correctly. \"1\" was requested but we did not get it\n");
		  goto CLEANUP_AND_EXIT;
	    }
      }

      glViewport(0, 0, window_width, window_height);
      /* @! */


      /* @@ setting up rendering of basic hello triangle */
      #define VERT_SIZE 9
      GLfloat vertices[VERT_SIZE];
      vertices[0] = -0.5f;
      vertices[1] = -0.5f;
      vertices[2] = 0.0f;
      vertices[3] = 0.5f;
      vertices[4] = -0.5f;
      vertices[5] = 0.0f;
      vertices[6] = 0.0f;
      vertices[7] = 0.5f;
      vertices[8] = 0.0f;

      const char * vert_shader_source = "#version 460 core\n"
	    "layout (location = 0) in vec3 vpos;\n"
	    "void main()\n"
	    "{\n"
	    "gl_Position = vec4(vpos.x, vpos.y, vpos.z, 1.0);\n"
	    "}\n\0";
      const char * frag_shader_source = "#version 460 core\n"
	    "out vec4 frag_color;\n"
	    "void main()\n"
	    "{\n"
	    "frag_color = vec4(0.1f, 0.7f, 0.5f, 1.0f);\n"
	    "}\n\0";

      GLuint vert_shader;
      GLuint frag_shader;
      GLuint shader_program;
      
      vert_shader = glCreateShader(GL_VERTEX_SHADER);
      frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
      shader_program = glCreateProgram();
      
      glShaderSource(vert_shader, 1, &vert_shader_source, NULL);
      glShaderSource(frag_shader, 1, &frag_shader_source, NULL);

      glCompileShader(vert_shader);
      glCompileShader(frag_shader);

      glAttachShader(shader_program, vert_shader);
      glAttachShader(shader_program, frag_shader);

      glLinkProgram(shader_program);

      glDeleteShader(vert_shader);
      glDeleteShader(frag_shader);


      GLuint vao;
      GLuint vbo;
      glGenVertexArrays(1, &vao);      
      glGenBuffers(1, &vbo);

      glBindVertexArray(vao);
      
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferData(GL_ARRAY_BUFFER, VERT_SIZE * sizeof(GLfloat), (const void *)vertices, GL_STATIC_DRAW);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)0);
      glEnableVertexAttribArray(0);

      
      XMapRaised(display, window);
      /* @! */


      /* @@ main loop */
      int running = 1;
      while(running) {	    
	    glClearColor(0.1f, 0.15f, 0.19f, 1.0f);
	    glClear(GL_COLOR_BUFFER_BIT);

	    
	    /* int num_events = XPending(display); /\* identical to XEventsQueued(display, QueuedAfterFlush) *\/ */

	    
	    XFlush(display); /* rather than calling XFlush() here, we could use the XPending() function instead of XEventsQueued() which will flush the X buffer if needed, I want to ensure that it flushes every frame consistently so that's why this is done */
	    int num_events = XEventsQueued(display, QueuedAlready);
	    
	    XEvent event;
	    while(num_events > 0) {
		  XNextEvent(display, &event);
		  if(event.type == KeyPress) {
			running = 0;
		  }

		  num_events = num_events - 1;
	    }

	    
	    /* @@ rendering */
	    glUseProgram(shader_program);
	    glBindVertexArray(vao);
	    glDrawArrays(GL_TRIANGLES, 0, 3);
	    /* @! */

	    
	    /* @@ the order of these last few procedures are critical */
	    glXSwapBuffers(display, glx_window);	    
	    glFinish(); /* blocks until all previous GL commands to finish, including the buffer swap. */
	    /* @! */
      }
      /* @! */


      /* @@ cleanup and program exit */
 CLEANUP_AND_EXIT:
      if(glx_context_current == TRUE) {
	    /* This function releases the current context when called with appropriate parameters, we have to first make the context not current or released/unbound to be able to destroy it properly */
	    glXMakeContextCurrent(display, None, None, NULL);
      }
      if(glx_context_exists == TRUE) {
	    glXDestroyContext(display, glx_context);
      }
      if(glx_window_exists == TRUE) {
	    glXDestroyWindow(display, glx_window);
      }
      if(window_exists == TRUE) {
	    XDestroyWindow(display, window);
      }
      if(display_exists == TRUE) {
	    XCloseDisplay(display);
      }

      return 0;
      /* @! */
}

