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

/* This file is only for the Macintosh to fake a gettimeofday() call.
   There is also a matching "sys/time.h" file for it.
 */

#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>

#include <Timer.h>

#include "sys/time.h"

static void pascal
timer_func(void)
{
    // do nothing
}


/* Starts and stops a timer every other time it is called. */

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

#ifdef VERSION_1
	/* Must be called only multiple-of-two times or the timer will be left primed. */
	static TMTask tm;
	static boolean primed = FALSE;
	long rt;

	if (!primed)
	{
		// start the timer for the next time that we are called
		tm.tmAddr = NewTimerProc(timer_func);	// we do not expect this to ever be called
		tm.tmCount = 0;		// initialize all fields
		tm.tmWakeUp = 0;
		tm.tmReserved = 0;
		InsTime((struct QElem*)&tm);
		PrimeTime((struct QElem*)&tm, -LONG_MAX);
		primed = TRUE;
		rt = 0;
	}
	else
	{
		RmvTime((struct QElem*)&tm);	// returns unused time in negative microseconds
		primed = FALSE;
		rt = LONG_MAX - -tm.tmCount;
	}
	
	// return the time in tv as expected
	if (tv != NULL)
	{
		tv->tv_sec = rt / 1000000;
		tv->tv_usec = rt % 1000000;
	}

	return 0;		// Unix value for successful
#endif // VERSION_1
}
