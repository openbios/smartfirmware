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

// (c) Copyright 1997 by CodeGen, Inc.  All Rights Reserved.

// Be-specific I/O + other stuff so we can test the display package.


extern "C" {
extern int bprintf(char *, const char *, ...);
#define main real_main
#include "main.c"
#undef main
EC(install_be);
}

#include <unistd.h>
#include <app/Application.h>
#include <interface/Window.h>
#include <interface/View.h>
#include <interface/Bitmap.h>
#include <interface/Screen.h>
#include <kernel/OS.h>
#include <storage/Path.h>


#define LINES			24
#define COLUMNS			80
#define SCREEN_WIDTH	(FONT_WIDTH * (COLUMNS + 1))
#define SCREEN_HEIGHT	(FONT_HEIGHT * (LINES + 1))


class Window : public BWindow
{
public:
	Window(BRect frame);
	virtual bool QuitRequested();
};


class View : public BView
{
	int _thread;
	BBitmap *_framebuf;	// fake frame-buffer for SmartFirmware
	BBitmap *_window;	// bitmap to draw into our Be window
	uByte _cmap[256];	// fake 8-bit color map

public:
	View(BRect rect, char *name);
	virtual ~View();
	virtual	void AttachedToWindow();
	virtual void Draw(BRect rect);
	virtual void KeyDown(const char *bytes, int32 numbytes);
	uByte *cmap() { return _cmap; }
	BBitmap *framebuf() { return _framebuf; }
};

class App : public BApplication
{
	Window *_window;
	View *_view;

public:
	App();
	virtual ~App();
	void refresh();
	uByte *cmap() { return _view->cmap(); }
	BBitmap *framebuf() { return _view->framebuf(); }
};

App *g_app;
int g_keyloc;
Byte g_keybuf[STR_SIZE];


// dummy placeholders for STANDALONE_GUI hooks in real_main()
Retcode
init_gui(int argc, char **argv)
{
	return NO_ERROR;
}

void
fini_gui(void)
{
}


static long
sf_thread(void *data)
{
	// run the real main() routine with fake args to make it happy
	char *argv[] = { "SmartFirmware", NULL };
	long ret = real_main(1, argv);
	
	g_app->PostMessage(B_QUIT_REQUESTED);
	return ret;
}


App::App() : BApplication("application/x-codegen-smartfirmware")
{
	BRect rect;
	app_info info;
	BEntry entry, parent;
	BPath path;

	g_app = this;

	GetAppInfo(&info);
	entry.SetTo(&info.ref);
	entry.GetParent(&parent);
	parent.GetPath(&path);
	chdir(path.Path());
	DPRINTF(("chdir: %s\n", path.Path()));

	rect.Set(50, 50, 50 + SCREEN_WIDTH, 50 + SCREEN_HEIGHT);
	_window = new Window(rect);

	rect.OffsetTo(B_ORIGIN);
	_view = new View(rect, "SFView");
	_window->AddChild(_view);

	_window->Lock();
	_view->MakeFocus();
	_window->Unlock();

	_window->Show();
}

App::~App()
{
}

void
App::refresh()
{
	_window->Lock();
	_view->Invalidate();
	_window->Unlock();
}


Window::Window(BRect frame) :
		BWindow(frame, "SmartFirmware", B_TITLED_WINDOW,
				B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
}

bool
Window::QuitRequested()
{
	g_app->PostMessage(B_QUIT_REQUESTED);
	return TRUE;
}



View::View(BRect rect, char *name) :
		BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	_framebuf = new BBitmap(rect, B_COLOR_8_BIT);
	_window = new BBitmap(rect, B_COLOR_8_BIT);
	memset(_cmap, 0, sizeof _cmap);
}

void
View::AttachedToWindow()
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	
	// initialize our fake color-map for standard 16-color text
	BScreen screen;

	#define COLOR(r, g, b)	screen.IndexForColor(r, g, b)
	_cmap[0] = COLOR(0, 0, 0);			// black
	_cmap[1] = COLOR(0, 0, 170);		// blue
	_cmap[2] = COLOR(0, 170, 0);		// green
	_cmap[3] = COLOR(0, 170, 170);		// cyan
	_cmap[4] = COLOR(170, 0, 0);		// red
	_cmap[5] = COLOR(170, 0, 170);		// magenta
	_cmap[6] = COLOR(170, 85, 0);		// brown
	_cmap[7] = COLOR(170, 170, 170);	// white
	_cmap[8] = COLOR(85, 85, 85);		// gray
	_cmap[9] = COLOR(85, 85, 255);		// bright blue
	_cmap[10] = COLOR(85, 255, 85);		// bright green
	_cmap[11] = COLOR(85, 255, 255);	// bright cyan
	_cmap[12] = COLOR(255, 85, 85);		// bright red
	_cmap[13] = COLOR(255, 85, 255);	// bright magenta
	_cmap[14] = COLOR(255, 255, 85);	// yellow
	_cmap[15] = COLOR(255, 255, 255);	// bright white
	#undef COLOR

	// finally spawn the SmartFirmware thread to complete the initialization
	_thread = spawn_thread(sf_thread,
			"SFThread", B_NORMAL_PRIORITY, g_e);
	resume_thread(_thread);
}

void
View::Draw(BRect rect)
{
	if (!g_e || !g_e->framebuf)
		return;

	uByte *from = (uByte*)(uPtr)g_e->framebuf;
	uByte *to = (uByte*)_window->Bits();
	Int size = g_e->sheight * g_e->swidth;
	Int i;

	// copy the SF framebuf into our window bitmap while
	// converting each pixel's color through our 8-bit color map
	for (i = 0; i < size; i++)
		to[i] = _cmap[from[i]];

	DrawBitmapAsync(_window, rect, rect);
}

void
View::KeyDown(const char *bytes, int32 numbytes)
{
	int i, key;

	for (i = 0; i < numbytes; i++)
	{
	 	key = bytes[i];
	 	
		// CR key returns NL but SF needs a CR
		if (key == '\n')
			key = '\r';
		else if (key == '\r')
			key = '\n';

		if (g_keyloc < STR_SIZE)
			g_keybuf[g_keyloc++] = key;
	}
}

View::~View()
{
	kill_thread(_thread);
	delete _window;
	delete _framebuf;
}



C(f_be_open)			/* be-open (--) */
{
	IFCKSP(e, 0, 4);
	PUSH(e, e->swidth);			// screen width
	PUSH(e, e->sheight);		// screen height
	PUSH(e, COLUMNS);			// columns
	PUSH(e, LINES);				// lines
	return execute_word(e, "fb8-install");
}

C(f_be_close)			/* be-close (--) */
{
	return NO_ERROR;
}

C(f_be_write)			/* write (buffer len -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Retcode ret;
	
	// call routine that would have been installed as "write" by is-install
	ret = execute_xtok(e, ((Entry*)inst->package->self)->xtok);
	
	// force App to redraw itself
	g_app->refresh();
	return ret;
}

/* additional words for 8-bit color extension */

C(f_be_color_set)		/* color! (r g b number --) */
{
	Int r, g, b, number;
	
	IFCKSP(e, 4, 0);
	POP(e, number);
	POP(e, b);
	POP(e, g);
	POP(e, r);
	
	if (number < 0 || number > 255)
		return E_BAD_COLOR;
	
	// set RGB value for number
	BScreen screen;
	g_app->cmap()[number] = screen.IndexForColor(r, g, b);
	return NO_ERROR;
}

C(f_be_color_get)		/* color@ (number -- r g b) */
{
	Int number;
	rgb_color color;
	
	IFCKSP(e, 1, 3);
	POP(e, number);
	
	if (number < 0 || number > 255)
		return E_BAD_COLOR;
	
	// get RGB value for number
	color = system_colors()->color_list[g_app->cmap()[number]];
	PUSH(e, color.red);
	PUSH(e, color.green);
	PUSH(e, color.blue);
	return NO_ERROR;
}

C(f_be_set_colors)		/* set-colors (addr number #numbers --) */
{
	uByte *addr;
	Int start, count, i;
	
	IFCKSP(e, 3, 0);
	POP(e, count);
	POP(e, start);
	POPT(e, addr, uByte*);
	
	if (addr == NULL || start < 0 || start + count > 255)
		return E_BAD_COLOR;
	
	BScreen screen;

	for (i = start; i < start + count; i++)
	{
		g_app->cmap()[i] = screen.IndexForColor(addr[0], addr[1], addr[2]);
		addr += 3;
	}
	
	return NO_ERROR;
}

C(f_be_get_colors)		/* get-colors (addr number #numbers --) */
{
	uByte *addr;
	Int start, count, i;
	rgb_color color;
	
	IFCKSP(e, 3, 0);
	POP(e, count);
	POP(e, start);
	POPT(e, addr, uByte*);
	
	if (addr == NULL || start < 0 || start + count > 255)
		return E_BAD_COLOR;
	
	for (i = start; i < count; i++)
	{
		// read RGB values
		color = system_colors()->color_list[g_app->cmap()[i]];
		*addr++ = color.red;
		*addr++ = color.green;
		*addr++ = color.blue;
	}
	
	return NO_ERROR;
}

C(f_be_dimensions)		/* dimensions (-- width height) */
{
	IFCKSP(e, 0, 2);
	PUSH(e, SCREEN_WIDTH);
	PUSH(e, SCREEN_HEIGHT);
	return NO_ERROR;
}


C(f_be_read)			/* read (buffer len -- actual) */
{
	Int len;
	Byte *addr;
	Int actual = -2;		// no input pending
	Retcode ret = NO_ERROR;
	
	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);

	if (g_keyloc > 0)
	{
		if (len > g_keyloc)
			len = g_keyloc;
		
		memcpy(addr, g_keybuf, len);
		g_keyloc = 0;
		actual = len;
	}

	PUSH(e, actual);
	return ret;
}

C(f_be_install_abort)			/* install-abort (--) */
{
	return NO_ERROR;
}

C(f_be_remove_abort)			/* remove-abort (--) */
{
	return NO_ERROR;
}

C(f_be_in_open)				/* open (-- ok?) */
{
	IFCKSP(e, 0, 1);
	PUSH(e, FTRUE);
	return NO_ERROR;
}


static const Initentry be_in_methods[] =
{
	{ "open",          f_be_in_open,       INVALID_FCODE },
	{ "close",         f_be_close,         INVALID_FCODE },
	{ "read",          f_be_read,          INVALID_FCODE },
	{ "install-abort", f_be_install_abort, INVALID_FCODE },
	{ "remove-abort",  f_be_remove_abort,  INVALID_FCODE },
	{ NULL,            NULL },
};

static const Initentry be_out_methods[] =
{
	{ "be-open",       f_be_open,          INVALID_FCODE },
	{ "be-close",      f_be_close,         INVALID_FCODE },
	{ "write",         f_be_write,         INVALID_FCODE },
	{ "color!",        f_be_color_set,     INVALID_FCODE },
	{ "color@",        f_be_color_get,     INVALID_FCODE },
	{ "set-colors",    f_be_set_colors,    INVALID_FCODE },
	{ "get-colors",    f_be_get_colors,    INVALID_FCODE },
	{ "dimensions",    f_be_dimensions,    INVALID_FCODE },
	{ NULL,            NULL },
};

extern "C" Retcode
install_gui(Environ *e)
{
	Retcode ret;
	Entry *ent, *went;
	Package *savpkg = e->currpkg;
	Package *pkg;
	
	// first we setup the input package, which is a fake serial line
	pkg = e->currpkg = new_pkg_name(e->root, "keyboard");
	
	if (pkg == NULL)
		return E_OUT_OF_MEMORY;
	
	prop_set_str(pkg->props, "device_type", CSTR, "serial", CSTR);
	ret = init_entries(e, pkg->dict, be_in_methods);
	
	if (ret != NO_ERROR)
		return ret;
	
	// next we setup the output package, which is a fake display into a window
	pkg = e->currpkg = new_pkg_name(e->root, "screen");
	
	if (pkg == NULL)
		return E_OUT_OF_MEMORY;
	
	prop_set_str(pkg->props, "device_type", CSTR, "display", CSTR);
	ret = init_entries(e, pkg->dict, be_out_methods);

	// 16-color text extension property + 8-bit color extension props
	prop_set_str(pkg->props, "iso6429-1983-colors", CSTR, "", CSTR);
	prop_set_int(pkg->props, "width", CSTR, SCREEN_WIDTH);
	prop_set_int(pkg->props, "height", CSTR, SCREEN_HEIGHT);
	
	// save this for later
	went = find_table(pkg->dict, "write", CSTR);
	
	if (went == NULL)
		return E_NO_METHOD;
	
	drop_table(pkg->dict, went);		// drop it for now
	
	// need to pass in the xtok for our open routine to is-install
	ent = find_table(pkg->dict, "be-open", CSTR);
	
	if (ent == NULL)
		return E_NO_METHOD;
	
	PUSH(e, ent->xtok);
	// adds open, write, draw-logo, and restore
	ret = execute_word(e, "is-install");
	
	// adds open, write, draw-logo, and restore
	if (ret != NO_ERROR)
		return ret;
	
	// need to pass in the xtok for our close routine to is-remove
	ent = find_table(pkg->dict, "be-close", CSTR);
	
	if (ent == NULL)
		return E_NO_METHOD;
	
	PUSH(e, ent->xtok);
	ret = execute_word(e, "is-remove");		// adds a close method
	
	/* save is-package's write method */
	pkg->self = (struct pself*)find_table(pkg->dict, "write", CSTR);
	insert_table(pkg->dict, went);		// put our write before is-install's
	e->currpkg = savpkg;

	// setup our SmartFirmware display parameters
	ret = execute_word(e, "default-font");
	
	if (ret == NO_ERROR)
		ret = execute_word(e, "set-font");
	
	// tell SF what size our display really is
	Byte cmd[STR_SIZE];
	bprintf(cmd, "setenv screen-#rows %d\nsetenv screen-#columns %d",
			LINES, COLUMNS);
	ret = interp_text(e, cmd, CSTR);

	// and initialize our frame buffer
	e->framebuf = (uPtr)g_app->framebuf()->Bits();
	e->swidth = g_app->framebuf()->BytesPerRow();
	e->sheight = SCREEN_HEIGHT;
	e->pixsize = 8;
	e->fg = 0;
	e->bg = 15;
	memset((Byte*)(uPtr)e->framebuf, e->bg, e->sheight * e->swidth);

	/* make console device aliases */
	ret = make_devalias(e, "keyboard", CSTR, "/keyboard", CSTR);

	if (ret == NO_ERROR)
		ret = make_devalias(e, "screen", CSTR, "/screen", CSTR);

	return ret;
}

int
main()
{
	g_app = new App();
	g_app->Run();
	delete g_app;
	return 0;
}
