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

// (c) Copyright 1996-1998 by CodeGen, Inc.  All Rights Reserved.

// Macintosh-specific I/O + other stuff so we can test the display package.

#ifndef macintosh
#error This file is Macintosh-specific.
#endif

#include "defs.h"

// some words are defined in defs.h and in Mac header files, so rename them
#define Byte MacByte
#define Ptr MacPtr
#define OLDROUTINENAMES 1
#include <Types.h>
#include <AppleEvents.h>
#include <Devices.h>
#include <Dialogs.h>
#include <Events.h>
#include <FixMath.h>
#include <Fonts.h>
#include <Gestalt.h>
#include <Memory.h>
#include <Menus.h>
#include <QuickDraw.h>
#include <QDOffscreen.h>
#include <SegLoad.h>
#include <Timer.h>
#include <ToolUtils.h>
#undef Byte
#undef Ptr


#define NIL 0

#define MAIN_MENU_BAR_ID	128
#define APPLE_MENU_ID		128
#define FILE_MENU_ID		129
#define EDIT_MENU_ID		130

#define ISO_6429_1983_ID	128

#define ABOUT_MENU_ITEM		1
#define CLOSE_MENU_ITEM		2
#define QUIT_MENU_ITEM		4

#define LINES		24
#define COLUMNS		80
#define SCREEN_WIDTH	(FONT_WIDTH * (COLUMNS + 1))
#define SCREEN_HEIGHT	(FONT_HEIGHT * (LINES + 1))


// these are for a Mac version of gettimeofday()
struct timeval
{
	long tv_sec;
	long tv_usec;
};

struct timezone
{
	int tz_minuteswest;
	int tz_dsttime;
};

extern int gettimeofday(struct timeval *tv, struct timezone *tz);


// file globals
static MenuHandle g_apple_menu, g_file_menu, g_edit_menu;
static WindowPtr g_window;
static PixMap g_pixmap;


int
gettimeofday(struct timeval *tv, struct timezone */*tz*/)
{
	// return the time in tv as expected
	if (tv != NULL)
	{
		UnsignedWide tm;
		
		Microseconds(&tm);
		tv->tv_sec = tm.lo / 1000000;
		tv->tv_usec = tm.lo % 1000000;
	}
	
	return 0;		// Unix value for successful
}

// needs to be done first thing upon entry to main() or stuff may not work
extern Retcode init_gui(int argc, char **argv);

Retcode
init_gui(int argc, char **argv)
{
	Handle menubar;
	Rect rect;
	int depth;
	
	InitGraf(&qd.thePort);
	InitFonts();
	FlushEvents(everyEvent, 0);
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(NIL);
	InitCursor();
	
	if ((menubar = GetNewMBar(MAIN_MENU_BAR_ID)) == NIL)
		return E_MAC_NO_MBAR;
	
	SetMenuBar(menubar);

	if ((g_apple_menu = GetMHandle(APPLE_MENU_ID)) == NIL)
		return E_MAC_NO_MENU;
	
	AddResMenu(g_apple_menu, 'DRVR');
	DrawMenuBar();

	// clear and set size of pixmap
	memset(&g_pixmap, 0, sizeof g_pixmap);
	g_pixmap.bounds.top = 0;
	g_pixmap.bounds.bottom = SCREEN_HEIGHT;
	g_pixmap.bounds.left = 0;
	// round width to nearest long-word boundary
	g_pixmap.bounds.right = (SCREEN_WIDTH & 0x03) ? (SCREEN_WIDTH & ~0x03) + 0x04 : SCREEN_WIDTH;

	// use size to create a window
	rect = g_pixmap.bounds;
	rect.top += 50;
	rect.bottom += 50;
	rect.left += 50;
	rect.right += 50;
	g_window = NewCWindow(NIL, &rect, "\pSmartFirmwareª", TRUE, noGrowDocProc,
			(WindowPtr)-1, TRUE, 0);	

	if (g_window == NIL)
		return E_MAC_NO_WINDOW;
	
	// extract current pixel depth from window
	depth = (*((CWindowPeek)g_window)->port.portPixMap)->pixelSize;
	
	if (depth != 8 && depth != 16 && depth != 32)
		return E_ABORT;
	
	// now we can allocate the pixmap
	g_pixmap.baseAddr = NewPtr(g_pixmap.bounds.right * SCREEN_HEIGHT * (depth >> 3));
	
	if (g_pixmap.baseAddr == NIL)
		return E_OUT_OF_MEMORY;
	
	// set the magic bit that tells QD that this is a PixMap and not a BitMap
	g_pixmap.rowBytes = (g_pixmap.bounds.right * (depth >> 3)) | 0x8000;
	g_pixmap.pmVersion = baseAddr32;		// 32-bit clean baseAddr pointer
	g_pixmap.packType= 0;
	g_pixmap.packSize = 0;
	g_pixmap.hRes = Long2Fix(72);
	g_pixmap.vRes = Long2Fix(72);
	g_pixmap.pixelType = (depth == 8) ? 0 : RGBDirect;
	g_pixmap.pixelSize = depth;
	g_pixmap.cmpCount = (depth == 8) ? 1 : 3;
	g_pixmap.cmpSize = (depth == 16) ? 5 : 8;
	g_pixmap.planeBytes = 0;
	g_pixmap.pmReserved = 0;

	g_pixmap.pmTable = GetCTable(ISO_6429_1983_ID);	// get the 8-bit color table
	
	if (g_pixmap.pmTable == NIL)
		return E_MAC_NO_CLUT;
	
	return NO_ERROR;
}

extern void fini_gui(void);

void
fini_gui(void)
{
	if (g_window)
		DisposeWindow(g_window);

	if (g_pixmap.pmTable)
		DisposCTable(g_pixmap.pmTable);

	if (g_pixmap.baseAddr)
		DisposPtr(g_pixmap.baseAddr);
}

C(f_mac_open)			/* mac-open (--) */
{
	Retcode ret;
	Byte cmd[STR_SIZE];
	
	ret = execute_word(e, "default-font");
	
	if (ret != NO_ERROR)
		return ret;
	
	ret = execute_word(e, "set-font");
	
	if (ret != NO_ERROR)
		return ret;
	
	// only our frame-buffer var needs to be initialized here - all others
	// are handled by fb8-install
	e->framebuf = (uPtr)g_pixmap.baseAddr;

	// init foreground & background colors as per the OF spec for 8-bit displays
	e->pixsize = g_pixmap.pixelSize;
	e->bg = e->fg = 0;
	
	/* see if we should invert fb/bg colors */
	if (get_config_bool(e, "inverse-video?", CSTR))
		e->bg = 15;
	else
		e->fg = 15;
	
	// force nv-vars to whatever is defined in this file
	bprintf(cmd, "setenv screen-#rows %d\nsetenv screen-#columns %d",
			LINES, COLUMNS);
	ret = interp_text(e, cmd, CSTR);
	
	IFCKSP(e, 0, 4);
	PUSH(e, g_pixmap.bounds.right - g_pixmap.bounds.left);	// screen width
	PUSH(e, g_pixmap.bounds.bottom - g_pixmap.bounds.top);	// screen height
	PUSH(e, COLUMNS);			// columns
	PUSH(e, LINES);				// lines
	return execute_word(e, "fb8-install");
}

C(f_mac_close)			/* mac-close (--) */
{
	return NO_ERROR;
}

C(f_mac_write)			/* write (buffer len -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Retcode ret;
	WindowPtr oldport;
	EventRecord event;
	
	// call routine that would have been installed as "write" by is-install
	ret = execute_xtok(e, ((Entry*)inst->package->self)->xtok);
	
	if (ret != NO_ERROR)
		return ret;
	
	// now blit our framebuffer out to the window to update it
	GetPort(&oldport);
	SetPort(g_window);
	CopyBits((BitMap*)&g_pixmap, &g_window->portBits,
			&g_pixmap.bounds, &g_window->portRect, srcCopy, NIL);
	SetPort(oldport);
	
	// see if the user typed Command-dot to kill us
	if (EventAvail(keyDownMask | keyUpMask | autoKeyMask, &event) &&
			(event.modifiers & cmdKey) &&
			(event.message & charCodeMask) == '.')
	{
		// force us outta here
		fini_gui();
		ExitToShell();
	}
	
	//WaitNextEvent(0, &event, 0, NIL);		// give some time to other processes
	return NO_ERROR;
}

// additional words for 8-bit color extension

C(f_mac_color_set)		/* color! (r g b number --) */
{
	Int r, g, b, number;
	
	IFCKSP(e, 4, 0);
	POP(e, number);
	POP(e, b);
	POP(e, g);
	POP(e, r);
	
	if (number < 0 || number > (*g_pixmap.pmTable)->ctSize + 1)
		return E_BAD_COLOR;
	
	(*g_pixmap.pmTable)->ctTable[number].rgb.red = (r << 8) | r;
	(*g_pixmap.pmTable)->ctTable[number].rgb.green = (g << 8) | g;
	(*g_pixmap.pmTable)->ctTable[number].rgb.blue = (b << 8) | b;
	CTabChanged(g_pixmap.pmTable);
	return NO_ERROR;
}

C(f_mac_color_get)		/* color@ (number -- r g b) */
{
	Int r, g, b, number;
	
	IFCKSP(e, 1, 3);
	POP(e, number);
	
	if (number < 0 || number > (*g_pixmap.pmTable)->ctSize + 1)
		return E_BAD_COLOR;
	
	r = (*g_pixmap.pmTable)->ctTable[number].rgb.red & 0xFF;
	g = (*g_pixmap.pmTable)->ctTable[number].rgb.green & 0xFF;
	b = (*g_pixmap.pmTable)->ctTable[number].rgb.blue & 0xFF;
	
	PUSH(e, r);
	PUSH(e, g);
	PUSH(e, b);
	return NO_ERROR;
}

C(f_mac_set_colors)		/* set-colors (addr number #numbers --) */
{
	uByte *addr;
	Int start, count, i;
	
	IFCKSP(e, 3, 0);
	POP(e, count);
	POP(e, start);
	POPT(e, addr, uByte*);
	
	if (start < 0 || start + count > (*g_pixmap.pmTable)->ctSize + 1)
		return E_BAD_COLOR;
	
	for (i = start; i < start + count; i++)
	{
		(*g_pixmap.pmTable)->ctTable[i].rgb.red = (addr[0] << 8) | addr[0];
		(*g_pixmap.pmTable)->ctTable[i].rgb.green = (addr[1] << 8) | addr[1];
		(*g_pixmap.pmTable)->ctTable[i].rgb.blue = (addr[2] << 8) | addr[2];
		addr += 3;
	}
	
	CTabChanged(g_pixmap.pmTable);
	return NO_ERROR;
}

C(f_mac_get_colors)		/* get-colors (addr number #numbers --) */
{
	uByte *addr;
	Int start, count, i;
	
	IFCKSP(e, 3, 0);
	POP(e, count);
	POP(e, start);
	POPT(e, addr, uByte*);
	
	if (start < 0 || start + count > (*g_pixmap.pmTable)->ctSize + 1)
		return E_BAD_COLOR;
	
	for (i = start; i < count; i++)
	{
		*addr++ = (*g_pixmap.pmTable)->ctTable[i].rgb.red & 0xFF;
		*addr++ = (*g_pixmap.pmTable)->ctTable[i].rgb.green & 0xFF;
		*addr++ = (*g_pixmap.pmTable)->ctTable[i].rgb.blue & 0xFF;
	}
	
	return NO_ERROR;
}

C(f_mac_dimensions)		/* dimensions (-- width height) */
{
	IFCKSP(e, 0, 2);
	PUSH(e, g_pixmap.bounds.right - g_pixmap.bounds.left);	// screen width
	PUSH(e, g_pixmap.bounds.bottom - g_pixmap.bounds.top);	// screen height
	return NO_ERROR;
}


static Retcode
do_menus(WindowPtr window, long m)
{
	Str255 accName;
	int accNumber;
	int menu = HiWord(m);
	int item = LoWord(m);
	

	if (m == 0)
		return NO_ERROR;
		
	switch (menu)
	{
	case APPLE_MENU_ID:
		if (item == ABOUT_MENU_ITEM)
		{
			interp_text(g_e, "cr banner", CSTR);
			PostEvent(keyDown, CTRL('L'));		// force an update
			break;
		}
		
		GetItem(g_apple_menu, item, accName);
		accNumber = OpenDeskAcc(accName);
		break;
		
	case FILE_MENU_ID:
		if (item == QUIT_MENU_ITEM ||
				item == CLOSE_MENU_ITEM)
			return R_END;
			
		break;
		
	case EDIT_MENU_ID:
		SystemEdit(item - 1);
		break;
	}

	HiliteMenu(0);
	return NO_ERROR;
}

static Retcode
do_mouse_down(EventRecord *event)
{
	WindowPtr window;
	short thePart;
	long menu;
	Rect dragrect;

	dragrect = qd.screenBits.bounds;
	dragrect.left += 4;
	dragrect.right -= 4;
	dragrect.bottom -= 4;
	
	thePart = FindWindow(event->where, &window);

	switch (thePart)
	{
	case inMenuBar:
		//adjust_menus();
		menu = MenuSelect(event->where);
		return do_menus(FrontWindow(), menu);

	case inSysWindow:
		SystemClick(event, window);
		break;

	case inDrag:
	case inContent:		// in window but not on a button
		DragWindow(window, event->where, &dragrect);
		break;

	case inGoAway:
		return R_END;
	}
	
	return NO_ERROR;
}

C(f_mac_read)			/* read (buffer len -- actual) */
{
	Int len;
	Byte *addr;
	Int actual = -2;		// no input pending
	Retcode ret = NO_ERROR;
	int c;
	EventRecord event;
	WindowPtr oldport;
	
	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
	
	while (WaitNextEvent(everyEvent, &event, 0, NIL) && len > 0 && ret == NO_ERROR)
	{
		switch (event.what)
		{
		case mouseDown:
			ret = do_mouse_down(&event);
			break;

		case keyDown:
		case autoKey:
			c = (char)(event.message & charCodeMask);
			
			if (event.modifiers & cmdKey)
			{
				//adjust_menus();
				if (c == '.')		// command-dot
				{
					actual = 0;		// return EOF
					len = 0;
				}
				else
					ret = do_menus(FrontWindow(), MenuKey(c));
			}
			else
			{
				*addr++ = c;
				len--;
				
				if (actual < 0)
					actual = 1;
				else
					actual++;
			}

			break;

		case updateEvt:
			GetPort(&oldport);
			SetPort(g_window);
			BeginUpdate(g_window);
			CopyBits((BitMap*)&g_pixmap, &g_window->portBits,
					&g_pixmap.bounds, &g_window->portRect, srcCopy, NIL);
			EndUpdate(g_window);
			SetPort(oldport);
			continue;
		}
	}
	
	PUSH(e, actual);
	return ret;
}

C(f_mac_install_abort)			/* install-abort (--) */
{
	return NO_ERROR;
}

C(f_mac_remove_abort)			/* remove-abort (--) */
{
	return NO_ERROR;
}

C(f_mac_in_open)				/* open (-- ok?) */
{
	IFCKSP(e, 0, 1);
	PUSH(e, FTRUE);
	return NO_ERROR;
}


static const Initentry mac_keyboard_methods[] =
{
	{ "open",          f_mac_in_open,       INVALID_FCODE },
	{ "close",         f_mac_close,         INVALID_FCODE },
	{ "read",          f_mac_read,          INVALID_FCODE },
	{ "install-abort", f_mac_install_abort, INVALID_FCODE },
	{ "remove-abort",  f_mac_remove_abort,  INVALID_FCODE },
	{ NULL,            NULL },
};

static const Initentry mac_screen_methods[] =
{
	{ "mac-open",      f_mac_open,          INVALID_FCODE },
	{ "mac-close",     f_mac_close,         INVALID_FCODE },
	{ "write",         f_mac_write,         INVALID_FCODE },
	{ "dimensions",    f_mac_dimensions,    INVALID_FCODE },
	{ "color!",        f_mac_color_set,     INVALID_FCODE },
	{ "color@",        f_mac_color_get,     INVALID_FCODE },
	{ "set-colors",    f_mac_set_colors,    INVALID_FCODE },
	{ "get-colors",    f_mac_get_colors,    INVALID_FCODE },
	{ NULL,            NULL },
};


CC(install_gui)
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
	ret = init_entries(e, pkg->dict, mac_keyboard_methods);
	
	if (ret != NO_ERROR)
		return ret;
	
	// next we setup the output package, which is a fake display into a window
	pkg = e->currpkg = new_pkg_name(e->root, "screen");
	
	if (pkg == NULL)
		return E_OUT_OF_MEMORY;
	
	prop_set_str(pkg->props, "device_type", CSTR, "display", CSTR);
	ret = init_entries(e, pkg->dict, mac_screen_methods);

	// 16-color text extension property
	prop_set_str(pkg->props, "iso6429-1983-colors", CSTR, "", CSTR);

	// 8-bit color extension properties?
	prop_set_int(pkg->props, "width", CSTR, g_pixmap.bounds.right - g_pixmap.bounds.left);
	prop_set_int(pkg->props, "height", CSTR, g_pixmap.bounds.bottom - g_pixmap.bounds.top);
	
	// save this for later
	went = find_table(pkg->dict, "write", CSTR);
	
	if (went == NULL)
		return E_NO_METHOD;
	
	drop_table(pkg->dict, went);		// drop it for now
	
	// need to pass in the xtok for our open routine to is-install
	ent = find_table(pkg->dict, "mac-open", CSTR);
	
	if (ent == NULL)
		return E_NO_METHOD;
	
	PUSH(e, ent->xtok);
	ret = execute_word(e, "is-install");	// adds open, write, draw-logo, and restore
	
	if (ret != NO_ERROR)
		return ret;
	
	// need to pass in the xtok for our close routine to is-remove
	ent = find_table(pkg->dict, "mac-close", CSTR);
	
	if (ent == NULL)
		return E_NO_METHOD;
	
	PUSH(e, ent->xtok);
	ret = execute_word(e, "is-remove");		// adds a close method
	
	// save is-package's write method
	pkg->self = (struct pself*)find_table(pkg->dict, "write", CSTR);
	insert_table(pkg->dict, went);		// put our write before is-install's

	// make console device aliases
	ret = make_devalias(e, "keyboard", CSTR, "/keyboard", CSTR);

	if (ret == NO_ERROR)
		ret = make_devalias(e, "screen", CSTR, "/screen", CSTR);

	e->currpkg = savpkg;
	return ret;
}
