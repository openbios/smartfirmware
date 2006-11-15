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

/* Code for Cogent EDB7312 display using fb7312 package */

/*#define DEBUG*/

#include "defs.h"

#include "ep7312.h"
#include "lib7312.h"

#define LCD_X_SIZE		320
#define LCD_Y_SIZE		240

void LCDColorEnable(void)
{
    unsigned long * volatile pulPtr = (unsigned long *)EDB7312_VA_EP7312;
    unsigned long ulLcdConfig, ulLineLength, ulVideoBufferSize;

    //
    // Determine the value to be programmed into the LCD controller in the
    // EP7312 to properly drive the Color LCD panel at 320x3x240, 4 bits per
    // pixel, at 80Hz.
    //
    ulVideoBufferSize = (((320 * 3) * 240 * 4) / 128) - 1;
    ulLineLength = ((320 * 3) / 16) - 1;
    ulLcdConfig = HwLcdControlGreyEnable |
                  HwLcdControlGrey4Or2 |
                  (0x13 << HwLcdControlAcPrescaleShift) |
                  (1 << HwLcdControlPixelPrescaleShift) |
                  (ulLineLength << HwLcdControlLineLengthShift) |
                  (ulVideoBufferSize << HwLcdControlBufferSizeShift);

    //
    // Configure the palette to be a one-to-one mapping of the pixel values to
    // the available LCD pixel intensities.
    //
    pulPtr[HwPaletteLSW >> 2] = 0x76543210;
    pulPtr[HwPaletteMSW >> 2] = 0xFEDCBA98;

    //
    // Program the LCD controller with the previously determined configuration.
    //
    pulPtr[HwLcdControl >> 2] = ulLcdConfig;

    //
    // Set the LCD frame buffer start address.
    //
    pulPtr[HwLcdFrameBuffer >> 2] = HwLcdBaseAddress >> 28;
}

void LCDColorOn(void)
{																	 
	unsigned long * volatile pulPtr = (unsigned long *)EDB7312_VA_EP7312;
	int loop;

	pulPtr[HwControl >> 2] |= HwControlLcdEnable;
	((unsigned char *)pulPtr)[HwPortD] |= HwPortDLCDPower;

	//delay
	for(loop = 0; loop < 500; loop++)
	{
	}

	((unsigned char *)pulPtr)[HwPortD] |= HwPortDLCDDcDcPower;

}

void LCDColorOff(void)
{
    unsigned long * volatile pulPtr = (unsigned long *)EDB7312_VA_EP7312;
    int iDelay;

    //
    // Power off the LCD DC-DC converter.
    //
    ((unsigned char *)pulPtr)[HwPortD] &= ~HwPortDLCDDcDcPower;

    //
    // Delay for a little while.
    //
    for(iDelay = 0; iDelay < 500; iDelay++)
    {
    }

    //
    // Power off the LCD panel.
    //
    ((unsigned char *)pulPtr)[HwPortD] &= ~HwPortDLCDPower;

    //
    // Power off the LCD controller.
    //
    pulPtr[HwControl >> 2] &= ~HwControlLcdEnable;
}



void LCDColorBacklightOn(void)
{
	unsigned long * volatile pulPtr = (unsigned long *)EDB7312_VA_EP7312;

	((unsigned char *)pulPtr)[HwPortD] |= HwPortDLCDBacklightPower;
}

void LCDColorBacklightOff(void)
{
	unsigned long * volatile pulPtr = (unsigned long *)EDB7312_VA_EP7312;

	((unsigned char *)pulPtr)[HwPortD] &= ~HwPortDLCDBacklightPower;
}


void LCDColorContrastEnable(void)
{
	unsigned long * volatile pulPtr = (unsigned long *)EDB7312_VA_EP7312;

	pulPtr[HwPumpControl >> 2] |= 0x00000800; 
}


void LCDColorContrastDisable(void)
{
	unsigned long * volatile pulPtr = (unsigned long *)EDB7312_VA_EP7312;

	pulPtr[HwPumpControl >> 2] = 0x00000000; 
}


void LCDColorCls(void)
{
    char *pcPtr = (char *)SwLcdBaseAddress;
    int iIdx;

    //
    // Fill the frame buffer with zeros.  Remember, we are 320 x 240 x rgb
	// so our total xsize is actually 960.
    //
    for(iIdx = 0; iIdx < (((LCD_X_SIZE * 3) * LCD_Y_SIZE) / 2); iIdx++)
    {
        *pcPtr++ = 0x00;
    }
}

//void LCDColorSetPixel(long lX, long lY, CPixel color)
//{
//long cx;
//
//cx = lX * 3;
//LCDCSetPixel(cx, lY, color.r);
//LCDCSetPixel(cx+1, lY, color.g);
//LCDCSetPixel(cx+2, lY, color.b);
//}

void LCDColorSetPixel(long lX, long lY, CPixel color)
{
    unsigned char * volatile pucPtr;
	unsigned long pixAdd, pixRem;


//  |R|G|	one pixel = three subpixels
//  |B|R|
//  |G|B|
//  |R|G|
//  |B|R|
//  |G|B|
//  |R|G|
//  |B|R|
//  |G|B|


    //
    // Make sure the specified pixel is valid.
    //
    if((lX < 0) || (lX >= (LCD_X_SIZE)) || (lY < 0) || (lY >= LCD_Y_SIZE))
    {
        return;
    }

    //
    // Compute the pixel number
    //
	pixAdd = lX + (lY * LCD_X_SIZE);

	// convert the pixel number to a byte address.  Since we pack each pixel 
	// in 1.5 bytes we must multiply the pixel address by 1.5.  Any remainder is
	// saved flagged in pixRem

	pixRem = (pixAdd * 3) % 2;	// 1 if we had a remainder, 0 otherwise

	pucPtr = (unsigned char *)(SwLcdBaseAddress + ((pixAdd * 3)/2));

    //
    // Set the appropriate pixel based on the state of pixRem
    //
    if(pixRem)
    {
        *pucPtr = ((*pucPtr & 0x0F) | (color.r << 4)); 	// red is the higher nibble
		pucPtr++;
        *pucPtr = ((*pucPtr & 0x00) | (color.g | (color.b << 4))); 	// Green is lower, Blue is upper nibble
    }
    else
    {
        *pucPtr = ((*pucPtr & 0x00) | (color.r | (color.g << 4))); 	// Red is lower, Green is upper
		pucPtr++;
        *pucPtr = ((*pucPtr & 0xF0) | color.b); 			// Blue is the lower nibble in the next byte
    }
}

void LCDCSetPixel(long lX, long lY, char cColor)
{
    unsigned char * volatile pucPtr;

    //
    // Make sure the specified pixel is valid.
    //
    if((lX < 0) || (lX >= LCD_X_SIZE) || (lY < 0) || (lY >= LCD_Y_SIZE))
    {
        return;
    }

    //
    // Compute the address of the pixel.
    //
    pucPtr = (unsigned char *)(SwLcdBaseAddress + (lY * (LCD_X_SIZE/2)) + (lX >> 1) + (lX % 2));

    //
    // Set the appropriate pixel based on the value of lX.
    //
    if(lX & 1)
    {
        *pucPtr = (*pucPtr & 0x0F) | (cColor << 4);
    }
    else
    {
        *pucPtr = (*pucPtr & 0xF0) | cColor;
    }
}

//****************************************************************************
//
// LCD.C - General drawing routines.
//
// Copyright (c) 1998-1999 Cirrus Logic, Inc.
//
//****************************************************************************

//****************************************************************************
//
// LCDLine draws a line from (lX1, lY1) to (lX2, lY2).  This is an
// implementation of Bresenham's line drawing algorithm; for details, see any
// reputable graphics book.
//
//****************************************************************************
void
LCDColorLine(long lX1, long lY1, long lX2, long lY2, CPixel sColor)
{       
    long lX, lY, lXend, lYend, lDx, lDy, lD, lXinc, lYinc, lIncr1, lIncr2;

    lDx = lX2 > lX1 ? lX2 - lX1 : lX1 - lX2;
    lDy = lY2 > lY1 ? lY2 - lY1 : lY1 - lY2;
    
    if(lDx >= lDy)
    {
        if(lX1 > lX2)
        {
            lX = lX2;
            lY = lY2;
            lXend = lX1;

            if(lDy == 0)
            {
                lYinc = 0;
            }
            else
            {
                if(lY2 > lY1)
                {
                    lYinc = -1;
                }
                else
                {
                    lYinc = 1;
                }
            }
        }
        else
        {
            lX = lX1;
            lY = lY1;
            lXend = lX2;

            if(lDy == 0)
            {
                lYinc = 0;
            }
            else
            {
                if(lY2 > lY1)
                {
                    lYinc = 1;
                }
                else
                {
                    lYinc = -1;
                }
            }
        }

        lIncr1 = 2 * lDy;
        lD = lIncr1 - lDx;
        lIncr2 = 2 * (lDy - lDx);

        LCDColorSetPixel(lX, lY, sColor);

        while(lX < lXend)
        {
            lX++;

            if(lD < 0)
            {
                lD += lIncr1;
            }
            else
            {
                lY += lYinc;      
                lD += lIncr2;
            }

            LCDColorSetPixel(lX, lY, sColor);
        }
    }
    else
    {
        if(lY1 > lY2)
        {
            lX = lX2;
            lY = lY2;
            lYend = lY1;

            if(lDx == 0)
            {
                lXinc = 0;
            }
            else
            {
                if(lX2 > lX1)
                {
                    lXinc = -1;
                }
                else
                {
                    lXinc = 1;
                }
            }
        }
        else
        {
            lX = lX1;
            lY = lY1;
            lYend = lY2;

            if(lDx == 0)
            {
                lXinc = 0;
            }
            else
            {
                if(lX2 > lX1)
                {
                    lXinc = 1;
                }
                else
                {
                    lXinc = -1;
                }
            }
        }

        lIncr1 = 2 * lDx;
        lD = lIncr1 - lDy;
        lIncr2 = 2 * (lDx - lDy);

        LCDColorSetPixel(lX, lY, sColor);

        while(lY < lYend)
        {
            lY++;

            if(lD < 0)
            {
                lD += lIncr1;
            }
            else
            {
                lX += lXinc;
                lD += lIncr2;
            }

            LCDColorSetPixel(lX, lY, sColor);
        }
    }
}

//****************************************************************************
//
// LCDCircle draws a circle at the specified location.  This is an
// implementation of Bresenham's circle drawing algorithm; for details, see
// any reputable graphics book.
//
//****************************************************************************
void
LCDColorCircle(long lX, long lY, long lRadius, CPixel sColor)
{
    long lXpos, lYpos, lD;

    lXpos = 0;
    lYpos = lRadius;

    lD = 3 - (2 * lYpos);

    while(lYpos >= lXpos)
    {
        LCDColorSetPixel(lX + lXpos, lY + lYpos, sColor);
        LCDColorSetPixel(lX + lYpos, lY + lXpos, sColor);
        LCDColorSetPixel(lX + lXpos, lY - lYpos, sColor);
        LCDColorSetPixel(lX - lYpos, lY + lXpos, sColor);
        LCDColorSetPixel(lX - lXpos, lY - lYpos, sColor);
        LCDColorSetPixel(lX - lYpos, lY - lXpos, sColor);
        LCDColorSetPixel(lX - lXpos, lY + lYpos, sColor);
        LCDColorSetPixel(lX + lYpos, lY - lXpos, sColor);

        if(lD < 0)
        {
            lD += (4 * lXpos) + 6;
        }
        else
        {
            lD += 10 + (4 * (lXpos - lYpos));
            lYpos--;
        }

        lXpos++;
    }
}

//****************************************************************************
//
// LCDFillCircle draws a filled circle at the specified location.  This is an
// implementation of Bresenham's circle drawing algorithm; for details, see
// any reputable graphics book.
//
//****************************************************************************
void
LCDColorFillCircle(long lX, long lY, long lRadius, CPixel sColor)
{
    long lXpos, lYpos, lD;

    lXpos = 0;
    lYpos = lRadius;

    lD = 3 - (2 * lYpos);

    while(lYpos >= lXpos)
    {
        LCDColorLine(lX - lXpos, lY + lYpos, lX + lXpos, lY + lYpos, sColor);
        LCDColorLine(lX - lYpos, lY + lXpos, lX + lYpos, lY + lXpos, sColor);
        LCDColorLine(lX - lXpos, lY - lYpos, lX + lXpos, lY - lYpos, sColor);
        LCDColorLine(lX - lYpos, lY - lXpos, lX + lYpos, lY - lXpos, sColor);

        if(lD < 0)
        {
            lD += (4 * lXpos) + 6;
        }
        else
        {
            lD += 10 + (4 * (lXpos - lYpos));
            lYpos--;
        }

        lXpos++;
    }
}


//****************************************************************************
//
// This program draws on the LCD panel, using all available drawing functions
// from the library.  After each "picture" has been drawn, pressing a keyboard
// button will cause the program to continue (or terminate when at the last
// "picture").
//
//****************************************************************************
void
LCDInitDisplay(void)
{
#if 0
    int xPos, yPos;
    CPixel sColor;
#endif

    //
    // Enable the LCD controller, clear the frame buffer, and turn on the LCD
    // panel.
    //
    LCDColorEnable();
    LCDColorCls();
    LCDColorOn();
    LCDColorBacklightOn();
    LCDColorContrastEnable();

#if 0
    //
    // Set the initial pixel color
    //
    sColor.r = 15;
    sColor.g = 0;
    sColor.b = 0;

    // fill the display first with red, then green and blue
    for(yPos = 0; yPos < 240; yPos++)
	for(xPos = 0; xPos < 320; xPos++)
	    LCDColorSetPixel(xPos, yPos, sColor);

    sColor.r = 0;
    sColor.g = 15;
    sColor.b = 0;

    // fill the display first with red, then green and blue
    for(yPos = 0; yPos < 240; yPos++)
	for(xPos = 0; xPos < 320; xPos++)
	    LCDColorSetPixel(xPos, yPos, sColor);

    sColor.r = 0;
    sColor.g = 0;
    sColor.b = 15;

    // fill the display first with red, then green and blue
    for(yPos = 0; yPos < 240; yPos++)
	for(xPos = 0; xPos < 320; xPos++)
	    LCDColorSetPixel(xPos, yPos, sColor);
	    
    sColor.r = 15;
    sColor.g = 15;
    sColor.b = 15;

    // fill the display with white
    for(yPos = 0; yPos < 240; yPos++)
	for(xPos = 0; xPos < 320; xPos++)
	    LCDColorSetPixel(xPos, yPos, sColor);

    sColor.r = 15;
    sColor.g = 0;
    sColor.b = 0;

    //
    // Draw two horizontal lines across the top and bottom of the screen by
    // individually setting the appropriate pixels.
    //
    for(xPos = 0; xPos < 320; xPos++)
    {
        LCDColorSetPixel(xPos, 01, sColor);
        LCDColorSetPixel(xPos, 239, sColor);
    }

    //
    // Draw diagonal lines from the upper-left corner to the lower-right
    // corner, and from the upper-right corner to the lower-left corner.
    //
    LCDColorLine(0, 0, 319, 239, sColor);
    LCDColorLine(0, 239, 319, 0, sColor);

    sColor.r = 0;
    sColor.g = 0;
    sColor.b = 15;

    //
    // Draw a filled circle with a radius of 50 at location (160, 320).
    //
    LCDColorFillCircle(160, 120, 50, sColor);

    sColor.r = 0;
    sColor.g = 15;
    sColor.b = 0;
    //
    // Draw a row of side-by-side circles of radius 4 across the screen.
    //
    for(xPos = 8; xPos < 320; xPos += 8)
        LCDColorCircle(xPos, 100, 4, sColor);

    //
    // Draw two vertical lines along the left and right of the screen by
    // individually setting the appropriate pixels.
    //
    for(yPos = 0; yPos < 240; yPos++)
    {
        LCDColorSetPixel(0, yPos, sColor);
        LCDColorSetPixel(319, yPos, sColor);
    }
#endif
}

void
LCDClearDisplay(void)
{
    int xPos, yPos;
    CPixel sColor;

    sColor.r = 15;
    sColor.g = 15;
    sColor.b = 15;

    // fill the display with white
    for(yPos = 0; yPos < 240; yPos++)
	for(xPos = 0; xPos < 320; xPos++)
	    LCDColorSetPixel(xPos, yPos, sColor);
}

EC(f_fb7312_reset_screen);			/* fb7312-reset-screen ( -- ) */

C(f_screen_selftest)
{
	Bool diag = diagnostic_mode(e);

	if (diag)
		cprintf(e, "%s display selftest\n", MACHINE_TYPE);

	PUSH(e, 0);			/* successful */
	return NO_ERROR;
}

C(f_screen_open)			/* screen-open (--) */
{
	Retcode ret;
	int displaywide, displayhigh, lines, cols;
	EC(f_default_font);
	EC(f_set_font);
	EC(f_fb7312_install);
	DPRINTF(("screen_open: begin\n"));

	LCDColorContrastEnable();
	LCDColorBacklightOn();

	displaywide = 320;
	displayhigh = 240;

	/* initialize colors before setting the mask */
	e->fg = 15;
	e->bg = 0;

	/* see if we should invert fb/bg colors */
	if (get_config_bool(e, "inverse-video?", CSTR))
	{
		Int t = e->fg;
		e->fg = e->bg;
		e->bg = t;
	}

	cols = displaywide / 8;
	lines = displayhigh / 18;
	e->linesperpage = lines;		/* paging on by default */
	IFCKSP(e, 0, 4);
	PUSH(e, displaywide);			/* screen width */
	PUSH(e, displayhigh);			/* screen height */
	PUSH(e, cols);					/* columns */
	PUSH(e, lines);					/* lines */
	ret = f_fb7312_install(e);		/* fb7312-install */

	if (ret == NO_ERROR)
		ret = f_fb7312_reset_screen(e);		/* clear the screen */

	DPRINTF(("screen_open: end, ret=%d\n", ret));
	return ret;
}

C(f_screen_close)			/* screen-close (--) */
{
	LCDColorContrastDisable();
	LCDColorBacklightOff();
	DPRINTF(("screen_close\n"));
	return NO_ERROR;
}

C(f_screen_contrast)			/* contrast (enable --) */
{
	Cell enable;
	DPRINTF(("screen_contrast\n"));

	POP(e, enable);

	if (enable)
		LCDColorContrastEnable();
	else
		LCDColorContrastDisable();

	return NO_ERROR;
}

C(f_screen_backlight)			/* backlight (enable --) */
{
	Cell enable;
	DPRINTF(("screen_backlight\n"));

	POP(e, enable);

	if (enable)
		LCDColorBacklightOn();
	else
		LCDColorBacklightOff();

	return NO_ERROR;
}

static const Initentry screen_methods[] =
{
	{ "screen-open",   f_screen_open,          INVALID_FCODE },
	{ "screen-close",  f_screen_close,         INVALID_FCODE },
	{ "selftest",      f_screen_selftest,      INVALID_FCODE },
	{ "contrast",      f_screen_contrast,      INVALID_FCODE },
	{ "backlight",     f_screen_backlight,     INVALID_FCODE },
	{ NULL,            NULL },
};

CC(probe_ep7312_display)
{
	Retcode ret;
	Entry *ent;
	Package *savepkg = e->currpkg;
	Package *pkg = new_pkg_name(e->root, "display");
#if 0
	Package *savescreen = (Package *)(uPtr)e->screen;
	Instance *inst;
#endif
	EC(f_is_install);
	EC(f_is_remove);
	EC(f_reg);
	extern char *g_fb_base;

	DPRINTF(("probe_ep7312_display: about to call LCDInitDisplay\n"));

	LCDInitDisplay();

	DPRINTF(("probe_ep7312_display: LCDInitDisplay called\n"));

	LCDColorContrastDisable();
	LCDColorBacklightOff();

	if (pkg == NULL)
		return E_OUT_OF_MEMORY;

	e->currpkg = pkg;
	ret = init_entries(e, pkg->dict, screen_methods);

	if (ret != NO_ERROR)
	{
		e->currpkg = savepkg;
		return ret;
	}

	prop_set_str(pkg->props, "device_type", CSTR, "display", CSTR);
	PUSH(e, 0xC0000000UL);
	PUSH(e, 115200);		/* 320*240*1.5 */
	ret = f_reg(e);
	e->currpkg = savepkg;

	g_fb_base = (unsigned char *)0xF7000000;

	if (ret == NO_ERROR)
		ret = prop_set_str(pkg->props, "iso6429-1983-colors", CSTR, "", CSTR);

	/* need to pass in the xtok for our open routine to is-install */
	ent = find_table(pkg->dict, "screen-open", CSTR);

	if (ent == NULL)
		return E_NO_METHOD;

	e->currpkg = pkg;
	PUSH(e, ent->xtok);
	ret = f_is_install(e);		/* is-install adds open, write, draw-logo, and restore */

	if (ret != NO_ERROR)
		return ret;

	/* need to pass in the xtok for our close routine to is-remove */
	ent = find_table(pkg->dict, "screen-close", CSTR);
	
	if (ent == NULL)
		return E_NO_METHOD;
	
	PUSH(e, ent->xtok);
	ret = f_is_remove(e);		/* is-remove adds a close method */

#if 0
	PUSH(e, "/display");
	PUSH(e, 8);
	ret = execute_word(e, "open-dev");

	if (ret != NO_ERROR)
		return ret;

	POPT(e, inst, Instance *);

	if (inst == NULL)
		return E_NO_DEVICE;

	e->screen = (Cell)inst;
	ret = execute_word(e, "banner");

	if (ret != NO_ERROR)
		return ret;

	PUSH(e, inst);
	ret = execute_word(e, "close-dev");

	if (ret != NO_ERROR)
		return ret;

	e->logo_width = 0;
	e->logo_height = 0;
	e->curline = 0;
	e->curcol = 0;
	e->screen = (Cell)savescreen;
#endif
	e->currpkg = savepkg;
	return ret;
}
