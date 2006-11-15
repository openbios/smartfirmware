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

/* (c) Copyright 1996-1999 by CodeGen, Inc.  All Rights Reserved. */

/* fb8 package - handles displays of 8, 16, and 32-bits per pixel
   - not incredibly fast, but easy to debug
 */

#include "defs.h"
#include "logo.h"

#if (LOGO_DEPTH != 1 && LOGO_DEPTH != 8) || LOGO_COLORS > 240
	#error Can only handle logo depth of 1 or 8 bits and colors <= 240.
#endif

#define TEXT_COLORS		16


static Int
get_pixel_color(int index, int pixelsize)
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
		
	if (pixelsize == 8 || inst == NULL || CKSP(g_e, 0, 1))
	{
		if (find_table(inst->package->props, "iso6429-1983-colors", CSTR))
			c = index;
		else
			c = (index) ? 0xFF : 0;
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
		}
		else
			r = g = b = index;		/* just replicate the pixel value */
		
		if (pixelsize == 16)
			c = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
		else
			c = (r << 16) | (g << 8) | b;
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
pixfill(int pixsize, void *dst, int stride, int x, int y,
	int w, int h, Int pixel, Bool lookup_color)
{
	/* The rectangle (x, y) .. (x + w - 1, y + h - 1) in the
	 * destination plane is filled with pixel.  No clipping is done.
	 */
	Byte *p;
	Short *sp;
	Int *ip;
	int i;
	
	if (lookup_color)
		pixel = get_pixel_color(pixel, pixsize);
	
	stride *= pixsize >> 3;
	p = ((Byte *)dst + stride * y) + x * (pixsize >> 3);
	
	/* fill each scan line */ 
	while (h--)
	{
		i = w;
		
		switch (pixsize)
		{
		case 8:
			memset(p, pixel, w);
			break;
			
		case 16:
			sp = (Short*)p;
			
			while (i--)
				*sp++ = pixel;
			
			break;
			
		case 32:
			ip = (Int*)p;
			
			while (i--)
				*ip++ = pixel;
			
			break;
					
		default:
			return E_BAD_PIXELSIZE;
		}
		
		p += stride;
	}
	
	return NO_ERROR;
}

static Retcode
pixinvert(int pixsize, void *dst, int stride, int x, int y,
	int w, int h, Int fg, Int bg)
{
	/* The rectangle (x, y) .. (x + w - 1, y + h - 1) in the
	 * destination plane is inverted.  No clipping is done.
	 */
	uByte *p;
	int i;
	uByte *p2;
	uShort *sp;
	uInt *ip;

	fg = get_pixel_color(fg, pixsize);
	bg = get_pixel_color(bg, pixsize);
	stride *= pixsize >> 3;
	p = (uByte *)dst + stride * y + x * (pixsize >> 3);

	/* invert each scan line */ 
	while (h--)
	{
		i = w;
		
		switch (pixsize)
		{
		case 8:
			p2 = p;

			while (i--)
			{
				/* only invert fg or bg value as per the color text spec */
				if (*p2 == fg)
					*p2 = bg;
				else if (*p2 == bg)
					*p2 = fg;

				p2++;
			}
			
			break;
			
		case 16:
			sp = (uShort*)p;

			while (i--)
			{
				/* only invert fg or bg value as per the color text spec */
				if (*sp == fg)
					*sp = bg;
				else if (*sp == bg)
					*sp = fg;

				sp++;
			}
			
			break;
			
		case 32:
			ip = (uInt*)p;

			while (i--)
			{
				/* only invert fg or bg value as per the color text spec */
				if (*ip == fg)
					*ip = bg;
				else if (*ip == bg)
					*ip = fg;

				ip++;
			}
			
			break;
					
		default:
			return E_BAD_PIXELSIZE;
		}

		p += stride;
	}
	
	return NO_ERROR;
}

static Retcode
pixcopy(int pixsize, void *src, int sstride, int sx, int sy, 
	void *dst, int dstride, int dx, int dy,
	int w, int h)
{
	/* copy pixels of pixsize bits from the rectangle (sx, sy) ..
	 * (sx + w - 1, sy + h - 1) in the source plane to the rectangle 
	 * (dx, dy) .. (dx + w - 1, dy + h - 1) in the destination plane.
	 * No clipping is done.
	 */

	Byte *ps, *pd;
	
	if (pixsize != 8 && pixsize != 16 && pixsize != 32)
		return E_BAD_PIXELSIZE;
	
	pixsize >>= 3;
	sstride *= pixsize;
	dstride *= pixsize;
	dx *= pixsize;
	sx *= pixsize;
	w *= pixsize;
	
	if (src == dst && sy < dy)
	{
		/* moving stuff down - do it bottom up to prevent trashing data */
		ps = (Byte*)src + sstride * (sy + h - 1);
		pd = (Byte*)dst + dstride * (dy + h - 1);

		while (h--)
		{
			/* copy a single scan line form source to dest */
			memmove(pd + dx, ps + sx, w);
			ps -= sstride;
			pd -= dstride;
		}
	}
	else
	{
		/* moving stuff up */
		ps = (Byte*)src + sstride * sy;
		pd = (Byte*)dst + dstride * dy;
	
		while (h--)
		{
			/* copy a single scan line form source to dest */
			memmove(pd + dx, ps + sx, w);
			ps += sstride;
			pd += dstride;
		}
	}
	
	return NO_ERROR;
}

static Retcode
pixexpand(int pixsize, void *src, int sstride, int sx, int sy, 
	void *dst, int dstride, int dx, int dy,
	int w, int h, Int fg, Int bg)
{
	/* copy 1-bit pixels from the rectangle (sx, sy) ..
	 * (sx + w - 1, sy + h - 1) in the source plane to the rectangle 
	 * (dx, dy) .. (dx + w - 1, dy + h - 1) in the destination plane.
	 * Single bit pixels in the source plane are expanded to either
	 * fg in the destination plane if the bit is one or bg if the bit is
	 * zero.  No clipping is done.
	 */
	Byte *ps, *pd;
	uByte *ps2, *pd2;
	uByte b;
	uShort *spd;
	uInt *ipd;
	int i;
	
	fg = get_pixel_color(fg, pixsize);
	bg = get_pixel_color(bg, pixsize);
	dstride *= pixsize >> 3;
	ps = (Byte *)src + sy * sstride;
	pd = (Byte *)dst + dy * dstride;

	while (h--)
	{
		ps2 = (uByte*)ps + sx;
		b = 0x80 >> (sx & 0x07);

		switch (pixsize)
		{
		case 8:
			pd2 = (uByte*)pd + dx;
			
			for (i = w; i; i--)
			{
				*pd2++ = (*ps2 & b) ? fg : bg;
				b >>= 1;

				if (b == 0)
				{
					b = 0x80;
					ps2++;
				}
			}
			
			break;
					
		case 16:
			spd = (uShort*)pd + dx;
			
			for (i = w ;i; i--)
			{
				*spd++ = (*ps2 & b) ? fg : bg;
				b >>= 1;

				if (b == 0)
				{
					b = 0x80;
					ps2++;
				}
			}
			
			break;
						
		case 32:
			ipd = (uInt*)pd + dx;
			
			for (i = w; i; i--)
			{
				*ipd++ = (*ps2 & b) ? fg : bg;
				b >>= 1;

				if (b == 0)
				{
					b = 0x80;
					ps2++;
				}
			}
		
			break;

		default:
			return E_BAD_PIXELSIZE;
		}
	
		ps += sstride;
		pd += dstride;
	}
	
	return NO_ERROR;
}


/* fb8-draw-character ( char -- ) 0x180 */
C(f_fb8_draw_character)
{
	Byte *charaddr;

	IFCKSP(e, 1, 0);
	(void)execute_word(e, ">font");
	POPT(e, charaddr, Byte*);

	if (charaddr)
		return pixexpand(e->pixsize, charaddr, e->fontbytes, 0, 0,
				(char*)(uPtr)e->framebuf, e->swidth,
				e->wleft + e->curcol * e->cwidth,
				e->wtop + e->curline * e->cheight,
				e->cwidth, e->cheight - 1,
				e->inverse ? e->bg : e->fg, e->inverse ? e->fg : e->bg);

	return NO_ERROR;
}

/* fb8-reset-screen ( -- ) 0x181 */
C(f_fb8_reset_screen)
{
	return pixfill(e->pixsize, (char*)(uPtr)e->framebuf, e->swidth, 0, 0,
			e->swidth, e->sheight, e->invscreen ? e->fg : e->bg, TRUE);
}

/* fb8-toggle-cursor ( -- ) 0x182 */
C(f_fb8_toggle_cursor)
{
	Int ulx = e->wleft + e->curcol * e->cwidth;
	Int uly = e->wtop + e->curline * e->cheight;
	
	e->cursor = !e->cursor;
	
	return pixinvert(e->pixsize, (char*)(uPtr)e->framebuf, e->swidth,
		ulx, uly, e->cwidth, e->cheight, e->fg, e->bg);
}

/* fb8-erase-screen ( -- ) 0x183 */
C(f_fb8_erase_screen)
{
	return pixfill(e->pixsize, (char*)(uPtr)e->framebuf, e->swidth, 0, 0,
			e->swidth, e->sheight, e->invscreen ? e->fg : e->bg, TRUE);
}

/* fb8-invert-screen ( -- ) 0x185 */
C(f_fb8_invert_screen)
{
	return pixinvert(e->pixsize, (char*)(uPtr)e->framebuf, e->swidth,
		e->wleft, e->wtop, e->cols * e->cwidth, e->lines * e->cheight,
		e->fg, e->bg);
}

/* fb8-blink-screen ( -- ) 0x184 */
C(f_fb8_blink_screen)
{
	IFCKSP(e, 0, 1);
	f_fb8_invert_screen(e);
	u_sleep(20000);
	f_fb8_invert_screen(e);
	return NO_ERROR;
}

/* fb8-insert-characters ( n -- ) 0x186 */
C(f_fb8_insert_characters)
{
	Int ulx = e->wleft + e->curcol * e->cwidth;
	Int uly = e->wtop + e->curline * e->cheight;
	Int w;
	Int n;
	Retcode ret;

	IFCKSP(e, 1, 0);
	POP(e, n);

	if (n < 1)
		return NO_ERROR;

	w = e->cols - e->curcol - n;

	if (w > 0)
	{
		ret = pixcopy(e->pixsize, (char*)(uPtr)e->framebuf, e->swidth, ulx, uly,
			(char*)(uPtr)e->framebuf, e->swidth, ulx + n * e->cwidth, uly,
			w * e->cwidth, e->cheight);
		
		if (ret != NO_ERROR)
			return ret;
	}
	
	return pixfill(e->pixsize, (char*)(uPtr)e->framebuf, e->swidth, ulx, uly,
		n * e->cwidth, e->cheight, e->invscreen ? e->fg : e->bg, TRUE);
}

/* fb8-delete-characters ( n -- ) 0x187 */
C(f_fb8_delete_characters)
{
	Int ulx = e->wleft + e->curcol * e->cwidth;
	Int uly = e->wtop + e->curline * e->cheight;
	Int n, w;
	Retcode ret;

	IFCKSP(e, 1, 0);
	POP(e, n);

	if (n < 1)
		return NO_ERROR;

	w = e->cols - e->curcol - n;

	if (w > 0)
	{
		ret = pixcopy(e->pixsize, (char*)(uPtr)e->framebuf, e->swidth,
			ulx + n * e->cwidth, uly, (char*)(uPtr)e->framebuf, e->swidth,
			ulx, uly, w * e->cwidth, e->cheight);
		
		if (ret != NO_ERROR)
			return ret;
	}

	return pixfill(e->pixsize, (char*)(uPtr)e->framebuf, e->swidth,
		ulx + w * e->cwidth, uly, n * e->cwidth, e->cheight,
		e->invscreen ? e->fg : e->bg, TRUE);
}

/* fb8-insert-lines ( n -- ) 0x188 */
C(f_fb8_insert_lines)
{
	Int uly = e->wtop + e->curline * e->cheight;
	Int h;
	Int n;
	Retcode ret;
	
	IFCKSP(e, 1, 0);
	POP(e, n);

	if (n < 1)
		return NO_ERROR;
		
	h = e->lines - e->curline - n;

	if (h > 0)
	{
		ret = pixcopy(e->pixsize,
			(char*)(uPtr)e->framebuf, e->swidth, 0, uly,
			(char*)(uPtr)e->framebuf, e->swidth, 0, uly + n * e->cheight,
			e->swidth, h * e->cheight);
		
		if (ret != NO_ERROR)
			return ret;
	}

	return pixfill(e->pixsize, (char*)(uPtr)e->framebuf, e->swidth, 0, uly,
		e->swidth, n * e->cheight, e->invscreen ? e->fg : e->bg, TRUE);
}

/* fb8-delete-lines ( n -- ) 0x189 */
C(f_fb8_delete_lines)
{
	Int uly = e->wtop + e->curline * e->cheight;
	Int h;
	Int n;
	Retcode ret;

	IFCKSP(e, 1, 0);
	POP(e, n);

	if (n < 1)
		return NO_ERROR;

	h = e->lines - e->curline - n;

	if (h > 0)
	{
		ret = pixcopy(e->pixsize,
			(char*)(uPtr)e->framebuf, e->swidth, 0, uly + n * e->cheight,
			(char*)(uPtr)e->framebuf, e->swidth, 0, uly,
			e->swidth, h * e->cheight);
		
		if (ret != NO_ERROR)
			return ret;
	}

	return pixfill(e->pixsize, (char*)(uPtr)e->framebuf, e->swidth, 0,
		uly + h * e->cheight, e->swidth, n * e->cheight,
		e->invscreen ? e->fg : e->bg, TRUE);
}

/* fb8-draw-logo ( line# addr width height -- ) 0x18A */
C(f_fb8_draw_logo)
{
	Int line, width, height;
	Byte *addr;
	Instance *inst = (Instance*)(uPtr)e->screen;
#if LOGO_DEPTH == 8
	int i, j, t, b;
	Short *saddr;
	Int *iaddr;
	Retcode ret;
#endif

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

	/* logo as defined in OF spec - black-n-white only */
	if (addr != NULL && addr != (Byte*)logo_pixmap)
	{
		pixfill(e->pixsize, (char*)(uPtr)e->framebuf, e->swidth,
				e->wleft, e->wtop + line * e->cheight,
				width, height, e->bg, TRUE);
		return pixexpand(e->pixsize, addr,
				(width + BYTE_SIZE - 1) / BYTE_SIZE, 0, 0,
				(char*)(uPtr)e->framebuf, e->swidth,
				e->wleft, e->wtop + line * e->cheight,
				width, height, e->fg, e->bg);
	}

#if LOGO_DEPTH == 8
	width = LOGO_WIDTH;
	height = LOGO_HEIGHT;
	pixfill(e->pixsize, (char*)(uPtr)e->framebuf, e->swidth,
			e->wleft, e->wtop + line * e->cheight,
			width, height, e->bg, TRUE);
	
	/* draw the logo pixel by pixel */
	switch (e->pixsize)
	{
	case 8:
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
		
		addr = (Byte*)(uPtr)e->framebuf +
				(e->wtop + line * e->cheight) * e->swidth + e->wleft;
		
		for (i = 0; i < height; i++)
		{
			t = i * width;
			
			for (j = 0; j < width; j++)
				addr[j] = logo_pixmap[t + j] + TEXT_COLORS;
			
			addr += (Int)e->swidth;
		}
		
		break;

	case 16:
		/* draw logo by converting 8-bit color map to direct pixels */
		saddr = (Short*)(uPtr)e->framebuf +
				(e->wtop + line * e->cheight) * e->swidth + e->wleft;
		
		for (i = 0; i < height; i++)
		{
			t = i * width;
			
			for (j = 0; j < width; j++)
			{
				b = logo_pixmap[t + j] * 3;
				
				saddr[j] = ((logo_colors[b] >> 3) << 10) |
						((logo_colors[b + 1] >> 3) << 5) |
						(logo_colors[b + 2] >> 3);
			}
			
			saddr += (Int)e->swidth;
		}
		
		break;

	case 32:
		/* draw logo by converting 8-bit color map to direct pixels */
		iaddr = (Int*)(uPtr)e->framebuf +
				(e->wtop + line * e->cheight) * e->swidth + e->wleft;
		
		for (i = 0; i < height; i++)
		{
			t = i * width;
			
			for (j = 0; j < width; j++)
			{
				b = logo_pixmap[t + j] * 3;
				
				iaddr[j] = (logo_colors[b] << 16) |
						(logo_colors[b + 1] << 8) |
						logo_colors[b + 2];
			}
			
			iaddr += (Int)e->swidth;
		}
		
		break;
			
	default:
		return E_BAD_PIXELSIZE;
	}
#endif /* LOGO_DEPTH == 8 */

	return NO_ERROR;
}

/* some simple 8-bit graphics extension words provided for convenience */

C(f_fb8_draw_rectangle)		/* draw-rectangle (addr x y w h --) */
{
	Byte *addr;
	Int x, y, w, h;
	
	IFCKSP(e, 5, 0);
	POP(e, h);
	POP(e, w);
	POP(e, y);
	POP(e, x);
	POPT(e, addr, Byte*);
	return pixcopy(e->pixsize,
			(char*)addr, w, 0, 0,
			(char*)(uPtr)e->framebuf, e->swidth, e->wleft + x, e->wtop + y,
			w, h);
}

static const Initentry draw_rect_list[] =
{
	{ "draw-rectangle", f_fb8_draw_rectangle, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(addr x y w h --)\n" \
				"\tcopy a rectangle from addr to the screen") },
	{ NULL, NULL }
};


C(f_fb8_fill_rectangle)		/* fill-rectangle (number x y w h --) */
{
	Int num, x, y, w, h;
	
	IFCKSP(e, 5, 0);
	POP(e, h);
	POP(e, w);
	POP(e, y);
	POP(e, x);
	POP(e, num);
	
	/* do not lookup color, to allow raw fills of arbitrary pixel values */
	return pixfill(e->pixsize, (char*)(uPtr)e->framebuf, e->swidth,
			x + e->wleft, y + e->wtop, w, h, num, FALSE);
}

static const Initentry fill_rect_list[] =
{
	{ "fill-rectangle", f_fb8_fill_rectangle, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(number x y w h --)\n" \
			"\fill a rectangle on the screen with the given pixel") },
	{ NULL, NULL }
};


C(f_fb8_read_rectangle)		/* read-rectangle (addr x y w h --) */
{
	Byte *addr;
	Int x, y, w, h;
	
	IFCKSP(e, 5, 0);
	POP(e, h);
	POP(e, w);
	POP(e, y);
	POP(e, x);
	POPT(e, addr, Byte*);
	return pixcopy(e->pixsize,
			(char*)(uPtr)e->framebuf, e->swidth, e->wleft + x, e->wtop + y,
			(char*)addr, w, 0, 0,
			w, h);
}

static const Initentry read_rect_list[] =
{
	{ "read-rectangle", f_fb8_read_rectangle, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(addr x y w h --)\n" \
					"\tcopy a rectangle from the screen to addr") },
	{ NULL, NULL }
};

/* fb8-install ( width height #columns #lines -- ) 0x18B */
CC(f_fb8_install)
{
	Retcode ret;
	Int val;
	Bool isocolor = FALSE;
	
	IFCKSP(e, 4, 0);
	
	/* could do these directly since all these words have pre-defined fcodes:
	   e.g. draw-character:   e->xtoks[0x157]->v.val = 0x180;  */
	ret = set_defer(e, "draw-character", "fb8-draw-character");
	ret = set_defer(e, "reset-screen", "fb8-reset-screen");
	ret = set_defer(e, "toggle-cursor", "fb8-toggle-cursor");
	ret = set_defer(e, "erase-screen", "fb8-erase-screen");
	ret = set_defer(e, "blink-screen", "fb8-blink-screen");
	ret = set_defer(e, "invert-screen", "fb8-invert-screen");
	ret = set_defer(e, "insert-characters", "fb8-insert-characters");
	ret = set_defer(e, "delete-characters", "fb8-delete-characters");
	ret = set_defer(e, "insert-lines", "fb8-insert-lines");
	ret = set_defer(e, "delete-lines", "fb8-delete-lines");
	ret = set_defer(e, "draw-logo", "fb8-draw-logo");
	
	/* create 3 of the 8-bit graphics extensions words if they are not there */
	if (e->currinst)
	{
		Instance *inst = (Instance*)(uPtr)e->currinst;
		Package *pkg = inst->package;
		
		if (find_table(pkg->props, "iso6429-1983-colors", CSTR))
			isocolor = TRUE;

		if (isocolor)
		{
			if (find_table(pkg->dict, "draw-rectangle", CSTR) == NULL)
				init_entries(e, pkg->dict, draw_rect_list);
		
			if (find_table(pkg->dict, "fill-rectangle", CSTR) == NULL)
				init_entries(e, pkg->dict, fill_rect_list);
		
			if (find_table(pkg->dict, "read-rectangle", CSTR) == NULL)
				init_entries(e, pkg->dict, read_rect_list);
		}
	}

	/* tell f_banner() in admin.c that we have a builtin logo */
	e->logo_width = LOGO_WIDTH;
	e->logo_height = LOGO_HEIGHT;

	e->scrollstep = 4;		/* slow scrolling, so jump-scroll a bit */
	POP(e, e->lines);
	POP(e, e->cols);
	POP(e, e->sheight);
	POP(e, e->swidth);

	/* initialize these fields if necessary */
	if (e->pixsize == 0)
		e->pixsize = 8;

	if (e->fg == 0 && e->bg == 0)
	{
		/* see if we should invert fb/bg colors */
		if (get_config_bool(e, "inverse-video?", CSTR))
			e->bg = 15;
		else
			e->fg = 15;
	}

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
	f_fb8_erase_screen(e);
	return f_fb8_toggle_cursor(e);		/* turn on the cursor */
}


const Initentry init_fb[] =
{
	{ "fb8-install", f_fb8_install, 0x18B, F_NONE, T_FUNC
			HELP("(width height #columns #lines --)\n"
		"\tinstall all built-in generic 8/16/32-bit frame-buffer routines") },
	{ "fb8-draw-character", f_fb8_draw_character, 0x180, F_NONE, T_FUNC
			HELP("(char --) fb* - draw character at current cursor position") },
	{ "fb8-reset-screen", f_fb8_reset_screen, 0x181, F_NONE, T_FUNC
			HELP("(--) fb* - perform frame-buffer device initialization") },
	{ "fb8-toggle-cursor", f_fb8_toggle_cursor, 0x182, F_NONE, T_FUNC
			HELP("(--) fb* - toggle the state of the text cursor") },
	{ "fb8-erase-screen", f_fb8_erase_screen, 0x183, F_NONE, T_FUNC
			HELP("(--) fb* - clear the screen") },
	{ "fb8-blink-screen", f_fb8_blink_screen, 0x184, F_NONE, T_FUNC
			HELP("(--) fb* - flash the screen") },
	{ "fb8-invert-screen", f_fb8_invert_screen, 0x185, F_NONE, T_FUNC
			HELP("(--) fb* - exchange foreground and background colors") },
	{ "fb8-insert-characters", f_fb8_insert_characters, 0x186, F_NONE, T_FUNC
			HELP("(n --) fb* - insert n spaces to the right of the cursor") },
	{ "fb8-delete-characters", f_fb8_delete_characters, 0x187, F_NONE, T_FUNC
			HELP("(n --) fb* - delete n characters to the right of cursor") },
	{ "fb8-insert-lines", f_fb8_insert_lines, 0x188, F_NONE, T_FUNC
			HELP("(n --) fb* - insert n lines at and below cursor line") },
	{ "fb8-delete-lines", f_fb8_delete_lines, 0x189, F_NONE, T_FUNC
			HELP("(n --) fb* - delete n lines at and below cursor line") },
	{ "fb8-draw-logo", f_fb8_draw_logo, 0x18A, F_NONE, T_FUNC
			HELP("(line# addr width height --)\n"
		"\tfb* - draw the 1-bit black&white logo stored at addr at line#\n" \
		"\tor draw the built-in 8-bit color logo if addr is NULL") },

	{ "fb8-draw-rectangle", f_fb8_draw_rectangle, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(addr x y w h --)\n" \
				"\tfb* - copy a rectangle from addr to the screen") },
	{ "fb8-fill-rectangle", f_fb8_fill_rectangle, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(number x y w h --)\n" \
			"\tfb* - fill a rectangle on the screen with the given pixel") },
	{ "fb8-read-rectangle", f_fb8_read_rectangle, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(addr x y w h --)\n" \
					"\tfb* - copy a rectangle from the screen to addr") },

	{ NULL, NULL }
};
