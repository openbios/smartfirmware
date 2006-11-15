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

/* (c) Copyright 2001 by CodeGen, Inc.  All Rights Reserved. */

/* fb7312 package - handles displays of 8, 16, and 32-bits per pixel
   - not incredibly fast, but easy to debug
 */

/*#define DEBUG*/
#include "defs.h"
#define logo_colors fb7312_logo_colors
#define logo_pixmap fb7312_logo_pixmap
#include "logo.h"

#if (LOGO_DEPTH != 1 && LOGO_DEPTH != 8) || LOGO_COLORS > 240
	#error Can only handle logo depth of 1 or 8 bits and colors <= 240.
#endif

#define TEXT_COLORS		16

unsigned char *g_fb_base;

#define DPY_WIDTH	320
#define DPY_STRIDE	(DPY_WIDTH + (DPY_WIDTH >> 1))
#define DPY_HEIGHT	240

static void
set_pixel(int x, int y, int c)
{
	char *p;

	if (x < 0 || y < 0 || x >= DPY_WIDTH || y >= DPY_HEIGHT)
		return;

	p = g_fb_base + ((((y * DPY_WIDTH) + x) * 3) / 2);

	if (x & 1)
	{
		p[0] = (p[0] & 0x0F) | ((c >> 4) & 0xF0);
		p[1] = ((c >> 4) & 0x0F) | ((c << 4) & 0xF0);
	}
	else
	{
		p[0] = ((c >> 8) & 0x0F) | (c & 0xF0);
		p[1] = (p[1] & 0xF0) | (c & 0x0F);
	}
}

static int
get_pixel(int x, int y)
{
	char *p;
	int v;

	if (x < 0 || y < 0 || x >= DPY_WIDTH || y >= DPY_HEIGHT)
		return -1;

	p = g_fb_base + ((((y * DPY_WIDTH) + x) * 3) / 2);

	if (x & 1)
		v = ((p[0] << 4) & 0xF00) | ((p[1] << 4) & 0x0F0) |
				((p[1] >> 4)& 0x00F);
	else
		v = ((p[0] << 8) & 0xF00) | (p[0] & 0x0F0) | (p[1] & 0x00F);

	return v;
}

static Int
get_pixel_color(int index)
{
	/* This routine maps 8-bit text colors to 16 and 32-bit direct pixels
	 * using the 8-bit color map if it can, and otherwise by replicating
	 * the pixel value for all three RGB components.  It then caches the
	 * color for faster lookup, as it is called rather frequently.
	 */
	Instance *inst = (Instance*)(uPtr)g_e->screen;
	Retcode ret;
	int r, g, b;
	Int c;
	static Int cache[TEXT_COLORS];		/* cache standard colors for text */
	static Instance *previnst;
	
	if (inst == NULL)
		inst = (Instance*)(uPtr)g_e->currinst;

	if (previnst == inst && index < TEXT_COLORS && cache[index])
		return cache[index] - 1;
		
	if (inst == NULL || CKSP(g_e, 0, 1))
	{
		if (find_table(inst->package->props, "iso6429-1983-colors", CSTR))
			c = index;
		else
			c = (index) ? 0xFF : 0;

		DPRINTF(("no inst: c %#x\n"));
	}
	else
	{
		PUSH(g_e, index);
		ret = execute_method_name(g_e, inst, "color@", CSTR);
		
		if (ret == NO_ERROR)
		{
			POP(g_e, b);
			POP(g_e, g);
			POP(g_e, r);
			DPRINTF(("dac: rgb (%#x,%#x,%#x)\n", r, g, b));
		}
		else
		{
			DROP(g_e);
			r = g = b = (index << 4);		/* just replicate the pixel value */
		}
		
		DPRINTF(("inst: rgb (%#x,%#x,%#x)\n", r, g, b));
		c = ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4);
		DPRINTF(("inst: c %#x\n", c));
	}

	/* clear the cache if the instance changed */
	if (previnst != inst)
	{
		memset(cache, 0, sizeof cache);
		previnst = inst;
	}
	
	if (index < TEXT_COLORS)
		cache[index] = c + 1;
	
	return c;
}

static Retcode
pixfill(int x, int y, int w, int h, Int pixel, Bool lookup_color)
{
	/* The rectangle (x, y) .. (x + w - 1, y + h - 1) in the
	 * destination plane is filled with pixel.  No clipping is done.
	 */
	int i;
	int j;
	char *p;
	char *pb;
	
	if (lookup_color)
		pixel = get_pixel_color(pixel);

	if (x < 0)
	{
		w += x;
		x = 0;
	}

	if (y < 0)
	{
		h += y;
		y = 0;
	}

	if (x >= DPY_WIDTH || y >= DPY_HEIGHT || w <= 0 || h <= 0)
		return NO_ERROR;

	if (x + w > DPY_WIDTH)
		w = DPY_WIDTH - x;

	if (y + h > DPY_HEIGHT)
		h = DPY_HEIGHT - y;

	DPRINTF(("pixfill: x %d, y %d, w %d, h %d, pixel %#x\n", x, y, w, h,
			pixel));

	pb = g_fb_base + ((((y * DPY_WIDTH) + x) * 3) / 2);

	while (h--)
	{
		p = pb;
		pb += DPY_STRIDE;
		j = x;

		for (i = 0; i < w; i++, j++)
		{
			if (j & 1)
			{
				p[0] = (p[0] & 0x0F) | ((pixel >> 4) & 0xF0);
				p[1] = ((pixel >> 4) & 0x0F) | ((pixel << 4) & 0xF0);
				p++;
			}
			else
			{
				p[0] = ((pixel >> 8) & 0x0F) | (pixel & 0xF0);
				p[1] = (p[1] & 0xF0) | (pixel & 0x0F);
			}

			p++;
		}

		y++;
	}

	return NO_ERROR;
}

static Retcode
pixinvert(int x, int y, int w, int h, Int fg, Int bg)
{
	/* The rectangle (x, y) .. (x + w - 1, y + h - 1) in the
	 * destination plane is inverted.  No clipping is done.
	 */
	int i;
	int j;
	Int pix;
	int npix;
	char *pb;
	char *p;

	fg = get_pixel_color(fg);
	bg = get_pixel_color(bg);

	if (x < 0)
	{
		w += x;
		x = 0;
	}

	if (y < 0)
	{
		h += y;
		y = 0;
	}

	if (x >= DPY_WIDTH || y >= DPY_HEIGHT || w <= 0 || h <= 0)
		return NO_ERROR;

	if (x + w > DPY_WIDTH)
		w = DPY_WIDTH - x;

	if (y + h > DPY_HEIGHT)
		h = DPY_HEIGHT - y;

	pb = g_fb_base + ((((y * DPY_WIDTH) + x) * 3) / 2);

	/* invert each scan line */ 
	while (h--)
	{
		p = pb;
		pb += DPY_STRIDE;
		j = x;

		for (i = 0; i < w; i++, j++)
		{
			if (j & 1)
				pix = ((p[0] << 4) & 0xF00) | ((p[1] << 4) & 0x0F0) |
						((p[1] >> 4)& 0x00F);
			else
				pix = ((p[0] << 8) & 0xF00) | (p[0] & 0x0F0) | (p[1] & 0x00F);

			/* only invert fg or bg value as per the color text spec */
			npix = pix;

			if (pix == fg)
				npix = bg;
			else if (pix == bg)
				npix = fg;

			if (npix != pix)
			{
				if (j & 1)
				{
					p[0] = (p[0] & 0x0F) | ((npix >> 4) & 0xF0);
					p[1] = ((npix >> 4) & 0x0F) | ((npix << 4) & 0xF0);
					p++;
				}
				else
				{
					p[0] = ((npix >> 8) & 0x0F) | (npix & 0xF0);
					p[1] = (p[1] & 0xF0) | (npix & 0x0F);
				}
			}
			else if (j & 1)
				p++;

			p++;
		}

		y++;
	}
	
	return NO_ERROR;
}

static Retcode
pixcopy(int sx, int sy, int dx, int dy, int w, int h)
{
	/* copy pixels of 12 bits from the rectangle (sx, sy) ..
	 * (sx + w - 1, sy + h - 1) in the source plane to the rectangle 
	 * (dx, dy) .. (dx + w - 1, dy + h - 1) in the destination plane.
	 * No clipping is done.
	 */
	int i;
	int aligned = 0;
	int stride = DPY_STRIDE;
	char *ps = (char *)0;
	char *pd = (char *)0;

	if ((sx & 1) == 0 && (dx & 1) == 0 && (w & 1) == 0)
	{
		aligned = 1;
		w += w >> 1;
		ps = g_fb_base + ((((sy * DPY_WIDTH) + sx) * 3) / 2);
		pd = g_fb_base + ((((dy * DPY_WIDTH) + dx) * 3) / 2);

		if (ps < pd)
		{
			ps += (h - 1) * DPY_STRIDE;
			pd += (h - 1) * DPY_STRIDE;
			stride = -DPY_STRIDE;
		}
	}

	while (h--)
	{
		if (!aligned)
		{
			for (i = 0; i < w; i++)
				set_pixel(dx + i, dy, get_pixel(sx + i, sy));

			sy++;
			dy++;
		}
		else
		{
			memmove(pd, ps, w);
			ps += stride;
			pd += stride;
		}
	}
	
	return NO_ERROR;
}

static Retcode
pixexpand(void *src, int sstride, int sx, int sy, int dx, int dy,
		int w, int h, Int fg, Int bg)
{
	/* copy 1-bit pixels from the rectangle (sx, sy) ..
	 * (sx + w - 1, sy + h - 1) in the source plane to the rectangle 
	 * (dx, dy) .. (dx + w - 1, dy + h - 1) in the destination plane.
	 * Single bit pixels in the source plane are expanded to either
	 * fg in the destination plane if the bit is one or bg if the bit is
	 * zero.  No clipping is done.
	 */
	uByte *psb;
	uByte *ps;
	uByte *pdb;
	uByte *pd;
	uByte b;
	uByte d;
	int i;
	int j;
	
	DPRINTF(("pixexpand: src %#x, stride %d, src (%d,%d), dst (%d,%d), %dx%d, fg %#x, bg %#x\n", src, sstride, sx, sy, dx, dy, w, h, fg, bg));
	fg = get_pixel_color(fg);
	bg = get_pixel_color(bg);
	DPRINTF(("pixexpand: fg %#x, bg %#x\n", fg, bg));

	if (dx < 0)
	{
		w += dx;
		sx -= dx;
		dx = 0;
	}

	if (dy < 0)
	{
		h += dy;
		sy -= dy;
		dy = 0;
	}

	if (sx < 0)
	{
		w += sx;
		dx -= sx;
		sx = 0;
	}

	if (sy < 0)
	{
		h += sy;
		dy -= sy;
		sy = 0;
	}

	if (sx >= sstride || dx >= DPY_WIDTH || dy >= DPY_HEIGHT ||
			w <= 0 || h <= 0)
		return NO_ERROR;

	if (dx + w > DPY_WIDTH)
		w = DPY_WIDTH - dx;

	if (dy + h > DPY_HEIGHT)
		h = DPY_HEIGHT - dy;

	if (sx + w > (sstride << 3))
		w = (sstride << 3) - sx;

	psb = (uByte *)src + sy * sstride + (sx >> 3);
	pdb = g_fb_base + ((((dy * DPY_WIDTH) + dx) * 3) / 2);

	while (h--)
	{
		ps = psb;
		pd = pdb;
		psb += sstride;
		pdb += DPY_STRIDE;
		d = *ps;
		b = (0x80 >> (sx & 0x07));
		j = dx;

		for (i = 0; i < w; i++, j++)
		{
			Int pix = ((d & b) ? fg : bg);

			if (j & 1)
			{
				pd[0] = (pd[0] & 0x0F) | ((pix >> 4) & 0xF0);
				pd[1] = ((pix >> 4) & 0x0F) | ((pix << 4) & 0xF0);
				pd++;
			}
			else
			{
				pd[0] = ((pix >> 8) & 0x0F) | (pix & 0xF0);
				pd[1] = (pd[1] & 0xF0) | (pix & 0x0F);
			}

			b >>= 1;
			pd++;

			if (b == 0)
			{
				b = 0x80;
				ps++;
				d = *ps;
			}
		}
	}
	
	return NO_ERROR;
}


/* fb7312-draw-character ( char -- ) */
C(f_fb7312_draw_character)
{
	Byte *charaddr;

	IFCKSP(e, 1, 0);
	DPRINTF(("f_fb7312_draw_character\n"));
	(void)execute_word(e, ">font");
	POPT(e, charaddr, Byte*);
	DPRINTF(("f_fb7312_draw_character: font char addr %#x\n", charaddr));

	if (charaddr)
		return pixexpand(charaddr, e->fontbytes, 0, 0,
				e->wleft + e->curcol * e->cwidth,
				e->wtop + e->curline * e->cheight,
				e->cwidth, e->cheight - 1,
				e->inverse ? e->bg : e->fg, e->inverse ? e->fg : e->bg);

	return NO_ERROR;
}

/* fb7312-reset-screen ( -- ) */
C(f_fb7312_reset_screen)
{
	DPRINTF(("f_fb7312_reset_screen\n"));
	return pixfill(0, 0, e->swidth, e->sheight, e->invscreen ? e->fg : e->bg,
			TRUE);
}

/* fb7312-toggle-cursor ( -- ) */
C(f_fb7312_toggle_cursor)
{
	Int ulx = e->wleft + e->curcol * e->cwidth;
	Int uly = e->wtop + e->curline * e->cheight;
	DPRINTF(("f_fb7312_toggle_cursor\n"));
	DPRINTF(("wleft %d, wtop %d, curcol %d, curline %d, cwidth %d, cheight %d\n", e->wleft, e->wtop, e->curcol, e->curline, e->cwidth, e->cheight));
	DPRINTF(("swidth %d, sheight %d\n", e->swidth, e->sheight));
	
	e->cursor = !e->cursor;
	
	return pixinvert(ulx, uly, e->cwidth, e->cheight, e->fg, e->bg);
}

/* fb7312-erase-screen ( -- ) */
C(f_fb7312_erase_screen)
{
	DPRINTF(("f_fb7312_erase_screen\n"));
	return pixfill(0, 0, e->swidth, e->sheight, e->invscreen ? e->fg : e->bg,
			TRUE);
}

/* fb7312-invert-screen ( -- ) */
C(f_fb7312_invert_screen)
{
	DPRINTF(("f_fb7312_invert_screen\n"));
	return pixinvert(e->wleft, e->wtop, e->cols * e->cwidth,
			e->lines * e->cheight, e->fg, e->bg);
}

/* fb7312-blink-screen ( -- ) */
C(f_fb7312_blink_screen)
{
	IFCKSP(e, 0, 1);
	DPRINTF(("f_fb7312_blink_screen\n"));
	f_fb7312_invert_screen(e);
	u_sleep(20000);
	f_fb7312_invert_screen(e);
	return NO_ERROR;
}

/* fb7312-insert-characters ( n -- ) */
C(f_fb7312_insert_characters)
{
	Int ulx = e->wleft + e->curcol * e->cwidth;
	Int uly = e->wtop + e->curline * e->cheight;
	Int w;
	Int n;
	Retcode ret;

	DPRINTF(("f_fb7312_insert_characters\n"));
	IFCKSP(e, 1, 0);
	POP(e, n);

	if (n < 1)
		return NO_ERROR;

	w = e->cols - e->curcol - n;

	if (w > 0)
	{
		ret = pixcopy(ulx, uly, ulx + n * e->cwidth, uly,
				w * e->cwidth, e->cheight);
		
		if (ret != NO_ERROR)
			return ret;
	}
	
	return pixfill(ulx, uly, n * e->cwidth, e->cheight,
			e->invscreen ? e->fg : e->bg, TRUE);
}

/* fb7312-delete-characters ( n -- ) */
C(f_fb7312_delete_characters)
{
	Int ulx = e->wleft + e->curcol * e->cwidth;
	Int uly = e->wtop + e->curline * e->cheight;
	Int n, w;
	Retcode ret;

	DPRINTF(("f_fb7312_delete_characters\n"));
	IFCKSP(e, 1, 0);
	POP(e, n);

	if (n < 1)
		return NO_ERROR;

	w = e->cols - e->curcol - n;

	if (w > 0)
	{
		ret = pixcopy(ulx + n * e->cwidth, uly, ulx, uly,
				w * e->cwidth, e->cheight);
		
		if (ret != NO_ERROR)
			return ret;
	}

	return pixfill(ulx + w * e->cwidth, uly, n * e->cwidth, e->cheight,
		e->invscreen ? e->fg : e->bg, TRUE);
}

/* fb7312-insert-lines ( n -- ) */
C(f_fb7312_insert_lines)
{
	Int uly = e->wtop + e->curline * e->cheight;
	Int h;
	Int n;
	Retcode ret;
	
	DPRINTF(("f_fb7312_insert_lines\n"));
	IFCKSP(e, 1, 0);
	POP(e, n);

	if (n < 1)
		return NO_ERROR;
		
	h = e->lines - e->curline - n;

	if (h > 0)
	{
		ret = pixcopy(0, uly, 0, uly + n * e->cheight,
				e->swidth, h * e->cheight);
		
		if (ret != NO_ERROR)
			return ret;
	}

	return pixfill(0, uly, e->swidth, n * e->cheight,
			e->invscreen ? e->fg : e->bg, TRUE);
}

/* fb7312-delete-lines ( n -- ) */
C(f_fb7312_delete_lines)
{
	Int uly = e->wtop + e->curline * e->cheight;
	Int h;
	Int n;
	Retcode ret;

	DPRINTF(("f_fb7312_delete_lines\n"));
	IFCKSP(e, 1, 0);
	POP(e, n);

	if (n < 1)
		return NO_ERROR;

	h = e->lines - e->curline - n;

	if (h > 0)
	{
		ret = pixcopy(0, uly + n * e->cheight, 0, uly,
				e->swidth, h * e->cheight);
		
		if (ret != NO_ERROR)
			return ret;
	}

	return pixfill(0, uly + h * e->cheight, e->swidth, n * e->cheight,
			e->invscreen ? e->fg : e->bg, TRUE);
}

/* fb7312-draw-logo ( line# addr width height -- ) */
C(f_fb7312_draw_logo)
{
	Int line, width, height;
	Byte *addr;
	Instance *inst = (Instance*)(uPtr)e->screen;
#if LOGO_DEPTH == 8
	int i, j, t;
	Retcode ret;
#endif

	DPRINTF(("f_fb7312_draw_logo\n"));
	IFCKSP(e, 4, 4);
	POP(e, height);
	POP(e, width);
	POPT(e, addr, Byte*);
	POP(e, line);
	
	if (inst == NULL)
		return E_NULL_INSTANCE;

#if LOGO_DEPTH == 1
	/* use built-in black-and-white pixmap */
	if (addr == NULL || addr == (Byte*)logo_pixmap)
	{
		addr = (Byte*)logo_pixmap;
		width = LOGO_WIDTH;
		height = LOGO_HEIGHT;
	}
#endif
	DPRINTF(("draw-logo: %dx%d (%d)\n", width, height, LOGO_DEPTH));

	/* logo as defined in OF spec - black-n-white only */
	if (addr != NULL && addr != (Byte*)logo_pixmap)
	{
		pixfill(e->wleft, e->wtop + line * e->cheight, width, height, e->bg,
				TRUE);
		return pixexpand(addr,
				(width + BYTE_SIZE - 1) / BYTE_SIZE, 0, 0,
				e->wleft, e->wtop + line * e->cheight,
				width, height, e->fg, e->bg);
	}

#if LOGO_DEPTH == 8
	width = LOGO_WIDTH;
	height = LOGO_HEIGHT;
	pixfill(e->wleft, e->wtop + line * e->cheight, width, height, e->bg, TRUE);
	
	/* draw the logo pixel by pixel */
	/* draw 8-bit color logo using 8-bit color extension words */
	if (find_table(inst->package->dict, "color@", CSTR))
	{
		/* set colors for logo, but leave first TEXT_COLORS alone */
		PUSHP(e, logo_colors);
		PUSH(e, TEXT_COLORS);
		PUSH(e, LOGO_COLORS);
		ret = execute_method_name(e, inst, "set-colors", CSTR);
		
		if (ret != NO_ERROR)
			return ret;
	}
		
	for (i = 0; i < height; i++)
	{
		t = i * width;
		
		for (j = 0; j < width; j++)
		{
			Int pix;
			int r, g, b;
			pix = logo_pixmap[t + j];

			if (pix >= LOGO_COLORS)
				r = g = b = 0xFF;
			else
			{
				r = logo_colors[pix * 3];
				g = logo_colors[pix * 3 + 1];
				b = logo_colors[pix * 3 + 2];
			}

			pix = ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4);
			set_pixel(e->wleft + j, e->wtop + line * e->cheight + i, pix);
		}
	}
		
#endif /* LOGO_DEPTH == 8 */

	return NO_ERROR;
}

/* fb7312-install ( width height #columns #lines -- ) */
CC(f_fb7312_install)
{
	Retcode ret;
	Int val;
	
	DPRINTF(("f_fb7312_install: begin\n"));
	IFCKSP(e, 4, 0);
	
	/* could do these directly since all these words have pre-defined fcodes:
	   e.g. draw-character:   e->xtoks[0x157]->v.val = 0x180;  */
	ret = set_defer(e, "draw-character", "fb7312-draw-character");
	ret = set_defer(e, "reset-screen", "fb7312-reset-screen");
	ret = set_defer(e, "toggle-cursor", "fb7312-toggle-cursor");
	ret = set_defer(e, "erase-screen", "fb7312-erase-screen");
	ret = set_defer(e, "blink-screen", "fb7312-blink-screen");
	ret = set_defer(e, "invert-screen", "fb7312-invert-screen");
	ret = set_defer(e, "insert-characters", "fb7312-insert-characters");
	ret = set_defer(e, "delete-characters", "fb7312-delete-characters");
	ret = set_defer(e, "insert-lines", "fb7312-insert-lines");
	ret = set_defer(e, "delete-lines", "fb7312-delete-lines");
	ret = set_defer(e, "draw-logo", "fb7312-draw-logo");
	DPRINTF(("f_fb7312_install: defers set\n"));
	
	/* tell f_banner() in admin.c that we have a builtin logo */
	e->logo_width = LOGO_WIDTH;
	e->logo_height = LOGO_HEIGHT;

	e->scrollstep = 1;		/* fast scrolling, so don't jump-scroll */
	POP(e, e->lines);
	POP(e, e->cols);
	POP(e, e->sheight);
	POP(e, e->swidth);
	DPRINTF(("f_fb7312_install: lines %d, cols %d, swidth %d, sheight %d\n", e->lines, e->cols, e->swidth, e->sheight));

	/* initialize these fields if necessary */
	if (e->pixsize == 0)
		e->pixsize = 12;

	e->fg = 0;
	e->bg = 0;

	/* see if we should invert fb/bg colors */
	if (get_config_bool(e, "inverse-video?", CSTR))
		e->bg = 15;
	else
		e->fg = 15;

	/* sanity check in case font is not initialized */
	if (e->font == NULL)
	{
		ret = execute_word(e, "default-font");

		if (ret == NO_ERROR)
			ret = execute_word(e, "set-font");
		
		if (ret != NO_ERROR)
			return ret;
	}

	/* e->cols is the minimum of the argument or "screen-#columns" */
	if (execute_word(e, "screen-#columns") == NO_ERROR)
	{
		POP(e, val);
		
		if (val && val < e->cols)
			e->cols = val;
	}

	/* e->lines is the minimum of the argument or "screen-#rows" */
	if (execute_word(e, "screen-#rows") == NO_ERROR)
	{
		POP(e, val);
		
		if (val && val < e->lines)
			e->lines = val;
	}

	/* make sure that lines/cols are reasonable for the font/screen size */
	if (e->lines * e->cheight > e->sheight)
		e->lines = e->sheight / e->cheight;

	if (e->cols * e->cwidth > e->swidth)
		e->cols = e->swidth / e->cwidth;
	
	/* now calculate window-top position */
	val = e->sheight - (e->lines * e->cheight);
	e->wtop = (val > 0) ? val / 2 : 0;
	
	/* and the window-left position */
	val = e->swidth - (e->cols * e->cwidth);
	e->wleft = (val > 0) ? val / 2 : 0;
	f_fb7312_erase_screen(e);
	DPRINTF(("f_fb7312_install: screen erased: %dx%d, %dx%d\n", e->cols, e->lines, e->swidth, e->sheight));
	return f_fb7312_toggle_cursor(e);		/* turn on the cursor */
}

const Initentry init_fb7312[] =
{
	{ "fb7312-install", f_fb7312_install, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(width height #columns #lines --)\n"
		"\tinstall all built-in generic 8/16/32-bit frame-buffer routines") },
	{ "fb7312-draw-character", f_fb7312_draw_character, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(char --) fb* - draw character at current cursor position") },
	{ "fb7312-reset-screen", f_fb7312_reset_screen, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) fb* - perform frame-buffer device initialization") },
	{ "fb7312-toggle-cursor", f_fb7312_toggle_cursor, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) fb* - toggle the state of the text cursor") },
	{ "fb7312-erase-screen", f_fb7312_erase_screen, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) fb* - clear the screen") },
	{ "fb7312-blink-screen", f_fb7312_blink_screen, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) fb* - flash the screen") },
	{ "fb7312-invert-screen", f_fb7312_invert_screen, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) fb* - exchange foreground and background colors") },
	{ "fb7312-insert-characters", f_fb7312_insert_characters, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(n --) fb* - insert n spaces to the right of the cursor") },
	{ "fb7312-delete-characters", f_fb7312_delete_characters, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(n --) fb* - delete n characters to the right of cursor") },
	{ "fb7312-insert-lines", f_fb7312_insert_lines, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(n --) fb* - insert n lines at and below cursor line") },
	{ "fb7312-delete-lines", f_fb7312_delete_lines, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(n --) fb* - delete n lines at and below cursor line") },
	{ "fb7312-draw-logo", f_fb7312_draw_logo, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(line# addr width height --)\n"
		"\tfb* - draw the 1-bit black&white logo stored at addr at line#\n" \
		"\tor draw the built-in 8-bit color logo if addr is NULL") },

	{ NULL, NULL }
};
