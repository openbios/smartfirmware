/*  Copyright (c) 1992-2005 CodeGen, Inc.  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Redistributions in any form must be accompanied by information on
 *     how to obtain complete source code for the CodeGen software and any
 *     accompanying software that uses the CodeGen software.  The source code
 *     must either be included in the distribution or be available for no
 *     more than the cost of distribution plus a nominal fee, and must be
 *     freely redistributable under reasonable conditions.  For an
 *     executable file, complete source code means the source code for all
 *     modules it contains.  It does not include source code for modules or
 *     files that typically accompany the major components of the operating
 *     system on which the executable file runs.  It does not include
 *     source code generated as output by a CodeGen compiler.
 *
 *  THIS SOFTWARE IS PROVIDED BY CODEGEN AS IS AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  (Commercial source/binary licensing and support is also available.
 *   Please contact CodeGen for details. http://www.codegen.com/)
 */

/* (c) Copyright 1998-1999 by CodeGen, Inc.  All Rights Reserved. */

/* X11-specific I/O + other stuff so we can test the display package.  */

#include "defs.h"

#ifdef LITTLE_ENDIAN
#undef LITTLE_ENDIAN
#endif

#ifdef STANDALONE_GUI

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>


#define ROWS		24
#define COLUMNS		80
#define SCREEN_WIDTH	(FONT_WIDTH * (COLUMNS + 1))
#define SCREEN_HEIGHT	(FONT_HEIGHT * (ROWS + 1))


Display *g_display;
int g_screen_num;
Colormap g_cmap;
GC g_gc;

static Window g_win;

static XImage *g_image = NULL;

unsigned long g_colors[256];

#define ISO_COLORS	16
struct
{
	unsigned short r, g, b;
} g_iso_colors[ISO_COLORS] =
{
	{ 0, 0, 0 },				/* black */
	{ 0, 0, 43690 },			/* blue */
	{ 0, 43690, 0 },			/* green */
	{ 0, 43690, 43690 },		/* cyan */
	{ 43690, 0, 0 },			/* red */
	{ 43690, 0, 43690 },		/* magenta */
	{ 43690, 21845, 0 },		/* brown */
	{ 43690, 43690, 43690 },	/* white */
	{ 21845, 21845, 21845 },	/* gray */
	{ 21845, 21845, 65535 },	/* bright blue */
	{ 21845, 65535, 21845 },	/* bright green */
	{ 21845, 65535, 65535 },	/* bright cyan */
	{ 65535, 21845, 21845 },	/* bright red */
	{ 65535, 21845, 65535 },	/* bright magenta */
	{ 65535, 65535, 21845 },	/* yellow */
	{ 65535, 65535, 65535 },	/* bright white */
};


extern Retcode init_gui(int argc, char **argv);
extern void fini_gui(void);

Retcode
init_gui(int argc, char **argv)
{
	unsigned i;
	XGCValues values;
	Visual *visual;
	int depth;
	char *imagedata;

	/* connect to X server */
	if ((g_display = XOpenDisplay(NULL)) == NULL)
	{
		dprintf("cannot connect to X server %s\n", XDisplayName(NULL));
		return NO_ERROR;
	}

	/* get screen size from display structure macro */
	g_screen_num = DefaultScreen(g_display);

	/* create opaque window */
	g_win = XCreateSimpleWindow(g_display, RootWindow(g_display, g_screen_num),
			0, 0,			/* x, y */
			SCREEN_WIDTH,
			SCREEN_HEIGHT,
			4,				/* border width */
			BlackPixel(g_display, g_screen_num),
			BlackPixel(g_display, g_screen_num));

	depth = DefaultDepth(g_display, g_screen_num);
	visual = DefaultVisual(g_display, g_screen_num);
	g_cmap = DefaultColormap(g_display, g_screen_num);

	/* set ISO 6429-1983 standard colors */
	for (i = 0; i < ISO_COLORS; i++)
	{
		XColor color;

		color.flags = DoRed | DoGreen | DoBlue;
		color.red = g_iso_colors[i].r;
		color.green = g_iso_colors[i].g;
		color.blue = g_iso_colors[i].b;
		XAllocColor(g_display, g_cmap, &color);
		g_colors[i] = color.pixel;
	}

	/* set the rest to black */
	for (; i < 255; i++)
		g_colors[i] = g_colors[0];

	/* for non-ISO B&W */
	g_colors[255] = g_colors[15];

	{
		XSizeHints size_hints;
		char *window_name = "SmartFirmware";
		XWMHints wm_hints;
		XClassHint class_hints;
		XTextProperty window_prop, icon_prop;

		/* Set size hints for window manager.  */
		size_hints.flags = PSize | PMinSize | PMaxSize;
		size_hints.max_width = size_hints.min_width = SCREEN_WIDTH;
		size_hints.max_height = size_hints.min_height = SCREEN_HEIGHT;

		if (XStringListToTextProperty(&window_name, 1, &window_prop) == 0)
		{
			dprintf("structure allocation for window_prop failed.\n");
			return E_ABORT;
		}

		if (XStringListToTextProperty(&window_name, 1, &icon_prop) == 0)
		{
			dprintf("structure allocation for icon_prop failed.\n");
			return E_ABORT;
		}

		wm_hints.initial_state = NormalState;
		wm_hints.input = True;
		wm_hints.flags = StateHint | IconPixmapHint | InputHint;

		class_hints.res_name = argv[0];
		class_hints.res_class = "SmartFirmware";

		XSetWMProperties(g_display, g_win, &window_prop, &icon_prop,
				argv, argc, &size_hints, &wm_hints,
				&class_hints);
	}


	/* Select event types wanted */
	XSelectInput(g_display, g_win, ExposureMask | KeyPressMask);


	/* Create default Graphics Context */
	g_gc = XCreateGC(g_display, g_win, 0, &values);
	XSetState(g_display, g_gc, g_colors[0], g_colors[0], GXcopy, AllPlanes);


	/* Display window */
	XMapWindow(g_display, g_win);
	XSync(g_display, /* no discard */ 0);


	/* create image buffer to handle drawing of varying screen depths */
	g_image = XCreateImage(g_display, visual,
			depth,			/* depth of image (bitplanes) */
			ZPixmap,
			0,				/* offset */
			NULL,			/* malloc() space after */
			SCREEN_WIDTH, SCREEN_HEIGHT,	/* x & y size of image */
			32,				/* # bits of padding */
			0);				/* bytes_per_line, let X11 calculate */

	if (!g_image)
	{
		dprintf("couldn't XCreateImage()\n");
		return E_ABORT;
	}

	imagedata = malloc(g_image->bytes_per_line * SCREEN_HEIGHT);

	if (!imagedata)
	{
		dprintf("imagedata: malloc(%d) returned error; depth=%d\n",
				g_image->bytes_per_line * SCREEN_HEIGHT, depth);
		return E_OUT_OF_MEMORY;
	}

	g_image->data = imagedata;

	XFlush(g_display);
	return NO_ERROR;
}

void
fini_gui(void)
{
}

void
draw_image(Environ *e)
{
	int x, y;
	unsigned long color;
	uByte *data;
	int bpl = g_image->bytes_per_line;
	uByte *fb = (uByte *)(uPtr)e->framebuf;

	switch (g_image->bits_per_pixel)
	{
	case 8:
		for (y = 0; y < SCREEN_HEIGHT; y++)
		{
			data = g_image->data + bpl * y;

			for (x = 0; x < SCREEN_WIDTH; x++)
				*data++ = g_colors[*fb++];
		}

		break;

	case 16:
		if (g_image->byte_order == LSBFirst)
		{
			for (y = 0; y < SCREEN_HEIGHT; y++)
			{
				data = g_image->data + bpl * y;

				for (x = 0; x < SCREEN_WIDTH; x++)
				{
					color = g_colors[*fb++];
					*data++ = color >> 0;
					*data++ = color >> 8;
				}
			}
		}
		else
		{
			for (y = 0; y < SCREEN_HEIGHT; y++)
			{
				data = g_image->data + bpl * y;

				for (x = 0; x < SCREEN_WIDTH; x++)
				{
					color = g_colors[*fb++];
					*data++ = color >> 8;
					*data++ = color >> 0;
				}
			}
		}

		break;

	case 24:
		if (g_image->byte_order == LSBFirst)
		{
			for (y = 0; y < SCREEN_HEIGHT; y++)
			{
				data = g_image->data + bpl * y;

				for (x = 0; x < SCREEN_WIDTH; x++)
				{
					color = g_colors[*fb++];
					*data++ = color >> 0;
					*data++ = color >> 8;
					*data++ = color >> 16;
				}
			}
		}
		else
		{
			for (y = 0; y < SCREEN_HEIGHT; y++)
			{
				data = g_image->data + bpl * y;

				for (x = 0; x < SCREEN_WIDTH; x++)
				{
					color = g_colors[*fb++];
					*data++ = color >> 16;
					*data++ = color >> 8;
					*data++ = color >> 0;
				}
			}
		}

		break;

	case 32:
		if (g_image->byte_order == LSBFirst)
		{
			for (y = 0; y < SCREEN_HEIGHT; y++)
			{
				data = g_image->data + bpl * y;

				for (x = 0; x < SCREEN_WIDTH; x++)
				{
					color = g_colors[*fb++];
					*data++ = color >> 0;
					*data++ = color >> 8;
					*data++ = color >> 16;
					*data++ = color >> 24;
				}
			}
		}
		else
		{
			for (y = 0; y < SCREEN_HEIGHT; y++)
			{
				data = g_image->data + bpl * y;

				for (x = 0; x < SCREEN_WIDTH; x++)
				{
					color = g_colors[*fb++];
					*data++ = color >> 24;
					*data++ = color >> 16;
					*data++ = color >> 8;
					*data++ = color >> 0;
				}
			}
		}

		break;

	default:
		for (y = 0; y < SCREEN_HEIGHT; y++)
			for (x = 0; x < SCREEN_WIDTH; x++)
				XPutPixel(g_image, x, y, g_colors[*fb++]);

		break;
	}

	XPutImage(g_display, g_win, g_gc, g_image, 0, 0, 0, 0,
			SCREEN_WIDTH, SCREEN_HEIGHT);
}

C(f_x11_open)			/* x11-open (--) */
{
	Retcode ret;
	Byte cmd[STR_SIZE];
	
	ret = execute_word(e, "default-font");
	
	if (ret != NO_ERROR)
		return ret;
	
	ret = execute_word(e, "set-font");
	
	if (ret != NO_ERROR)
		return ret;
	
	/* init frame-buffer var */
	e->framebuf = (uPtr)malloc(SCREEN_WIDTH * 8 * SCREEN_HEIGHT);

	if (e->framebuf == 0)
		return E_OUT_OF_MEMORY;

	/* init foreground & background colors to defaults as per OF spec */
	e->pixsize = 8;
	e->bg = e->fg = 0;
	
	/* see if we should invert fb/bg colors */
	if (get_config_bool(e, "inverse-video?", CSTR))
		e->bg = 15;
	else
		e->fg = 15;

	/* force nv-vars to whatever is defined in this file */
	bprintf(cmd, "setenv screen-#rows %d\nsetenv screen-#columns %d",
			ROWS, COLUMNS);
	ret = interp_text(e, cmd, CSTR);

	IFCKSP(e, 0, 4);
	PUSH(e, SCREEN_WIDTH);		/* screen width */
	PUSH(e, SCREEN_HEIGHT);		/* screen height */
	PUSH(e, COLUMNS);			/* columns */
	PUSH(e, ROWS);				/* lines */
	return execute_word(e, "fb8-install");
}

C(f_x11_close)			/* x11-close (--) */
{
	if (e->framebuf)
	{
		free((Byte*)(uPtr)e->framebuf);
		e->framebuf = 0;
	}

	return NO_ERROR;
}

C(f_x11_write)			/* write (buffer len -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Retcode ret;
	
	/* call routine that would have been installed as "write" by is-install */
	ret = execute_xtok(e, ((Entry*)inst->package->self)->xtok);
	
	if (ret != NO_ERROR)
		return ret;
	
	/* now blit our framebuffer out to the window to update it */
	draw_image(e);
	
	return NO_ERROR;
}

/* additional words for 8-bit color extension */

C(f_x11_color_set)		/* color! (r g b number --) */
{
	Int r, g, b, number;
	XColor color;
	
	IFCKSP(e, 4, 0);
	POP(e, number);
	POP(e, b);
	POP(e, g);
	POP(e, r);
	
	if (number < 0 || number > 255)
		return E_BAD_COLOR;
	
	color.flags = DoRed | DoGreen | DoBlue;
	color.red = r << 8;
	color.green = g << 8;
	color.blue = b << 8;

	if (g_colors[number] != g_colors[0])
		XFreeColors(g_display, g_cmap, &g_colors[number], 1, 0);

	XAllocColor(g_display, g_cmap, &color);
	g_colors[number] = color.pixel;

	return NO_ERROR;
}

C(f_x11_color_get)		/* color@ (number -- r g b) */
{
	Int r, g, b, number;
	XColor color;
	
	IFCKSP(e, 1, 3);
	POP(e, number);
	
	if (number < 0 || number > 255)
		return E_BAD_COLOR;
	
	color.pixel = number;
	XQueryColor(g_display, g_cmap, &color);
	r = color.red >> 8;
	g = color.green >> 8;
	b = color.blue >> 8;
	
	PUSH(e, r);
	PUSH(e, g);
	PUSH(e, b);
	return NO_ERROR;
}

C(f_x11_set_colors)		/* set-colors (addr number #numbers --) */
{
	uByte *addr;
	Int start, count, i;
	Retcode ret;
	
	IFCKSP(e, 3, 0);
	POP(e, count);
	POP(e, start);
	POPT(e, addr, uByte*);
	
	if (start < 0 || start + count > 255)
		return E_BAD_COLOR;
	
	for (i = start; i < start + count; i++)
	{
		PUSH(e, addr[0]);
		PUSH(e, addr[1]);
		PUSH(e, addr[2]);
		PUSH(e, i);
		ret = f_x11_color_set(e);

		if (ret != NO_ERROR)
			return ret;

		addr += 3;
	}
	
	return NO_ERROR;
}

C(f_x11_get_colors)		/* get-colors (addr number #numbers --) */
{
	uByte *addr;
	Int start, count, i, r, g, b;
	Retcode ret;
	
	IFCKSP(e, 3, 0);
	POP(e, count);
	POP(e, start);
	POPT(e, addr, uByte*);
	
	if (start < 0 || start + count > 255)
		return E_BAD_COLOR;
	
	for (i = start; i < count; i++)
	{
		PUSH(e, i);
		ret = f_x11_color_get(e);

		if (ret != NO_ERROR)
			return ret;

		POP(e, b);
		POP(e, g);
		POP(e, r);
		*addr++ = r;
		*addr++ = g;
		*addr++ = b;
	}
	
	return NO_ERROR;
}

C(f_x11_dimensions)		/* dimensions (-- width height) */
{
	IFCKSP(e, 0, 2);
	PUSH(e, SCREEN_WIDTH);	/* screen width */
	PUSH(e, SCREEN_HEIGHT);	/* screen height */
	return NO_ERROR;
}

C(f_x11_read)			/* read (buffer len -- actual) */
{
	Int len;
	Byte *addr;
	Int actual = -2;		/* no input pending */
	Retcode ret = NO_ERROR;

	XEvent event;
	XKeyEvent *key_event;
	KeySym keysym;
	XComposeStatus compose;
	char buffer[10];
	int charcount, i;
	XExposeEvent *expose_event;
	
	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);

	while (XPending(g_display) > 0)
	{
		XNextEvent(g_display, &event);

		switch (event.type)
		{
		case Expose:
			/* draw only last contiguous refresh */
			expose_event = (XExposeEvent *)&event;

			if (expose_event->count == 0)
				draw_image(e);

			break;

		case KeyPress:
			key_event = (XKeyEvent *)&event;
			charcount = XLookupString(key_event, buffer, sizeof buffer,
					&keysym, &compose);

			for (i = 0; i < charcount && len; i++, len--)
				*addr++ = buffer[i];
			
			if (actual < 0)
				actual = i;
			else
				actual += i;

			break;

		default:
			break;
		}
	}

	PUSH(e, actual);
	return ret;
}

C(f_x11_install_abort)			/* install-abort (--) */
{
	return NO_ERROR;
}

C(f_x11_remove_abort)			/* remove-abort (--) */
{
	return NO_ERROR;
}

C(f_x11_in_open)				/* open (-- ok?) */
{
	IFCKSP(e, 0, 1);
	PUSH(e, FTRUE);
	return NO_ERROR;
}


static const Initentry x11_keyboard_methods[] =
{
	{ "open",          f_x11_in_open,       INVALID_FCODE },
	{ "close",         f_x11_close,         INVALID_FCODE },
	{ "read",          f_x11_read,          INVALID_FCODE },
	{ "install-abort", f_x11_install_abort, INVALID_FCODE },
	{ "remove-abort",  f_x11_remove_abort,  INVALID_FCODE },
	{ NULL,            NULL },
};

static const Initentry x11_screen_methods[] =
{
	{ "x11-open",      f_x11_open,          INVALID_FCODE },
	{ "x11-close",     f_x11_close,         INVALID_FCODE },
	{ "write",         f_x11_write,         INVALID_FCODE },
	{ "dimensions",    f_x11_dimensions,    INVALID_FCODE },
	{ "color!",        f_x11_color_set,     INVALID_FCODE },
	{ "color@",        f_x11_color_get,     INVALID_FCODE },
	{ "set-colors",    f_x11_set_colors,    INVALID_FCODE },
	{ "get-colors",    f_x11_get_colors,    INVALID_FCODE },
	{ NULL,            NULL },
};


CC(install_gui)
{
	Retcode ret;
	Entry *ent, *went;
	Package *savpkg = e->currpkg;
	Package *pkg;

	if (!g_display || !g_image)
		return NO_ERROR;
	
	/* first we setup the input package, which is a fake keyboard */
	pkg = e->currpkg = new_pkg_name(e->root, "keyboard");
	
	if (pkg == NULL)
		return E_OUT_OF_MEMORY;
	
	prop_set_str(pkg->props, "device_type", CSTR, "serial", CSTR);
	ret = init_entries(e, pkg->dict, x11_keyboard_methods);
	
	if (ret != NO_ERROR)
		return ret;
	
	/* next we setup the output package, which is a fake display in a window */
	pkg = e->currpkg = new_pkg_name(e->root, "screen");
	
	if (pkg == NULL)
		return E_OUT_OF_MEMORY;
	
	prop_set_str(pkg->props, "device_type", CSTR, "display", CSTR);
	ret = init_entries(e, pkg->dict, x11_screen_methods);

	/* 16-color text extension property */
	prop_set_str(pkg->props, "iso6429-1983-colors", CSTR, "", CSTR);

	/* 8-bit color extension properties? */
	prop_set_int(pkg->props, "width", CSTR, SCREEN_WIDTH);
	prop_set_int(pkg->props, "height", CSTR, SCREEN_HEIGHT);
	
	/* save this for later */
	went = find_table(pkg->dict, "write", CSTR);
	
	if (went == NULL)
		return E_NO_METHOD;
	
	drop_table(pkg->dict, went);		/* drop it for now */
	
	/* need to pass in the xtok for our open routine to is-install */
	ent = find_table(pkg->dict, "x11-open", CSTR);
	
	if (ent == NULL)
		return E_NO_METHOD;
	
	/* add open, write, draw-logo, and restore */
	PUSH(e, ent->xtok);
	ret = execute_word(e, "is-install");
	
	if (ret != NO_ERROR)
		return ret;
	
	/* need to pass in the xtok for our close routine to is-remove */
	ent = find_table(pkg->dict, "x11-close", CSTR);
	
	if (ent == NULL)
		return E_NO_METHOD;
	
	PUSH(e, ent->xtok);
	ret = execute_word(e, "is-remove");		/* adds a close method */
	
	/* save is-package's write method */
	pkg->self = (struct pself*)find_table(pkg->dict, "write", CSTR);
	insert_table(pkg->dict, went);		/* put our write before is-install's */

	/* make console device aliases */
	ret = make_devalias(e, "keyboard", CSTR, "/keyboard", CSTR);

	if (ret == NO_ERROR)
		ret = make_devalias(e, "screen", CSTR, "/screen", CSTR);

	e->currpkg = savpkg;
	return ret;
}

#endif /* STANDALONE_GUI */
