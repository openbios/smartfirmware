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

/* (c) Copyright 1996-1997 by CodeGen, Inc.  All Rights Reserved. */

#ifdef STANDALONE
#	include <stdio.h>
#	include <stdlib.h>
#	include <stdarg.h>
#	if defined __MWERKS__ && defined macintosh
#		include <unix.h>
#	else	/* unix */
#		include <fcntl.h>
#		include <unistd.h>
#	endif
#else
#	include "stdlib.h"
#	include "stdarg.h"
#endif	/* STANDALONE */

#include "defs.h"

#ifdef STANDALONE_GUI
	extern Retcode init_gui(int argc, char **argv);
	extern void fini_gui(void);
#endif


Environ *g_e = NULL;
void *g_stack_bottom;


/* argc/argv are only used for STANDALONE builds.  They are typically not
   defined for most ports and are un-initialized.  Unfortunately some
   compilers complain unless they are defined for main().
 */
int
main(int argc, char **argv)
{
	Retcode r = NO_ERROR;
	Retcode nvret = NO_ERROR;
	Byte *str;
	Int len;
	Environ *e;
	
	g_stack_bottom = (void*)&argc;	/* for debugging use */

	if (machine_initialize() != NO_ERROR)
		return E_INIT_ERROR;

#ifdef STANDALONE

#   ifdef STANDALONE_GUI
#   ifdef unix
	if (argc >= 2 && argv[1][0] == '-' && argv[1][1] == 'g')	/* -gui */
	{
#   endif
		r = init_gui(argc, argv);

		if (r != NO_ERROR)
			return r;

#   ifdef unix
		argc--;
		argv++;
	}
#	endif
#	endif	/* STANDALONE_GUI */

#	ifdef unix
	if (isatty(fileno(stdin)) && isatty(fileno(stdout)))
	{
		system("stty eof '^A' -echo -icanon -isig");

#		ifdef O_NONBLOCK
			fcntl(fileno(stdin), F_SETFL,
					fcntl(fileno(stdin), F_GETFL) | O_NONBLOCK);
#		endif
	}

#	endif	/* unix */

#endif		/* STANDALONE */

	g_e = e = (Environ*)malloc(sizeof *e);
	DPRINTF(("allocated g_e=%p (len=%d)\n", g_e, sizeof *g_e));
	e = init_environ(e);
	DPRINTF(("after init_environ e=%p\n", e));

	if (e == NULL)
	{
		machine_led_write(NULL, 1);
		return E_INIT_ERROR;
	}

#ifdef STANDALONE
	(void)machine_init_args(e, argc, argv);
	DPRINTF(("after machine_init_args e=%p\n"));
#endif		/* STANDALONE */

	/* first run nvram script, if any */
	str = get_config(e, "use-nvramrc?", CSTR);
	
	if (str && (str[0] == 't' || str[0] == 'T'))
	{
		str = NULL;
		len = 0;
		(void)get_config_val(e, "nvramrc", CSTR, &str, &len);

		if (str && len)
		{
			DPRINTF(("running nvramrc...\n"));
			e->security = SM_NVRAMRC;
			nvret = interp_text(e, str, len);
			e->security = SM_NONE;

			if (nvret != NO_ERROR)
				machine_led_write(e, 2);
		}

		DPRINTF(("after running nvramrc\n"));
	}

	/* run the startup script to set everything up if it isn't already */
	if (!e->no_banner || r != NO_ERROR)
	{
		/* perform the default startup script by calling each word in order:
				"probe-all install-console banner"
		   we do not do anything with errors except report them in case
		   the failsafe I/O code is working - this lets the user get to a
		   prompt and figure out what went wrong
		 */
		DPRINTF(("no/bad nvramrc - performing default startup script\n"));
		r = execute_word(e, "probe-all");

		if (r != NO_ERROR)
		{
			cprintf(e, "probe-all ");
			error(r);
			machine_led_write(e, 3);
		}

		r = execute_word(e, "install-console");

		if (r != NO_ERROR)
		{
			cprintf(e, "install-console ");
			error(r);
			machine_led_write(e, 4);
		}

		r = execute_word(e, "banner");

		if (r != NO_ERROR)
		{
			cprintf(e, "banner ");
			error(r);
			machine_led_write(e, 5);
		}
	}

	if (nvret != NO_ERROR)
	{
		DPRINTF(("nvramrc error\n"));
		cprintf(e, "nvramrc script ");
		error(nvret);		/* in case we had an error in the nvramrc script */
		machine_led_write(e, 6);
	}

	/* if we do not have a screen or keyboard, hope failsafe works */
	if (e->screen == (uPtr)NULL)
	{
		DPRINTF(("no console!\n"));
		error(E_NO_CONSOLE);
	}
	else if (e->keyboard == (uPtr)NULL)
	{
		DPRINTF(("no keyboard!\n"));
		error(E_NO_KEYBOARD);
	}

#ifdef STANDALONE
	/* optionally run the tokenizer using command-line options */
	if (argc > 3 && argv[1][0] == '-' && argv[1][1] == 't')
	{
		r = tokenize(e, argv[2], argv[3]);
		error(r);
		goto done;
	}
	else if (argc > 2 && argv[1][0] == '-' && argv[1][1] == 'f')
	{
		r = load_file(e, argv[2]);
		error(r);
		goto done;
	}
	else if (argc > 2 && argv[1][0] == '-' && argv[1][1] == 'd')
	{
		r = detok_file(e, argv[2]);
		error(r);
		goto done;
	}
	else if (argc > 1 && (argv[1][0] != '-' || argv[1][1] != '-'))
	{
		fprintf(stderr, "%s: usage: (one of):\n"
			#ifdef STANDALONE_GUI
				"  -g[ui] <additional args>\n"
			#endif
				"  -t[okenize] forth-source-file fcode-output-file\n"
				"  -f[ile] forth-or-fcode-file\n"
				"  -d[etokenize] fcode-file\n",
				argv[0]);
		goto done;
	}
#endif		/* STANDALONE */

	/* there is no word specified in OpenFirmware to run secondary diagnostics, 
	   so we punt and create our own */
	if (get_config_bool(e, "secondary-diag?", CSTR))
	{
		DPRINTF(("running secondary diagnostics...\n"));
		r = execute_word(e, "secondary-diag");
		error(r);
	}

	/* initialize current security mode from NVRAM */
	e->security = security_mode(e);

	/* now try to auto-boot */
	str = get_config(e, "auto-boot?", CSTR);

	if (e->security != SM_FULL && str && (str[0] == 't' || str[0] == 'T'))
	{
		/* auto-boot-timeout is not a standard OpenFirmware parameter */
		Cell timeout = get_config_int(e, "auto-boot-timeout", CSTR);
		int key;

		DPRINTF(("auto-booting...\n"));

		if (timeout <= 0)
			timeout = 5000;

		e->flush_output = FO_NONE;
		cprintf(e,
			"Auto-boot in %d seconds - press ESC to abort, ENTER to boot: ",
				timeout / 1000);

		for (; timeout >= 0; timeout -= 10)
		{
			u_sleep(10000);

			if (key_down(e))
				break;

			if ((timeout % 1000) == 0)
				cprintf(e,
			"\rAuto-boot in %Cd seconds - press ESC to abort, ENTER to boot: "
						"       \b\b\b\b\b\b\b",
						timeout / 1000);
		}

		/* if a key is down, we may have to abort the auto-boot */
		key = (timeout >= 0) ? get_key(e) : '\r';
		
		/* user can press ENTER or RETURN to continue auto-boot */
		if (key == '\r' || key == '\n')
		{
			cprintf(e, "\n");
			str = NULL;
			len = 0;
			(void)get_config_val(e, "boot-command", CSTR, &str, &len);

			if (str && *str && len)
			{
				r = interp_text(e, str, len);
				error(r);				/* not supposed to get here */
			}
			else
				cprintf(e, "boot-command parameter is not set\n");
		}
		else
			cprintf(e, "aborted\n");
	}

	/* dunno if this is needed or not, but it can be useful to know */
	if (diagnostic_mode(e))
		cprintf(e, "Extended diagnostics are now switched on.\n");

#ifdef STANDALONE
	/* read-eval loop but "exit" will actually exit */
	r = interpret(e);

done:	;

	free(destroy_environ(e));

#	ifdef unix
	if (isatty(fileno(stdin)) && isatty(fileno(stdout)))
	{
#		ifdef O_NONBLOCK
			fcntl(fileno(stdin), F_SETFL,
					fcntl(fileno(stdin), F_GETFL) & ~O_NONBLOCK);
#		endif

		system("stty eof '^D' echo icanon isig");
	}
#	endif

#else /* !STANDALONE */

	/* either no auto-boot or bootup failed, so go into a read-eval loop */
	DPRINTF(("entering main read/eval loop...\n"));

	do
		r = interpret(e);
	while (r == R_END);

	DPRINTF(("exited main read/eval loop - freeing everything\n"));
	free(destroy_environ(e));

#endif /* !STANDALONE */

	machine_led_write(NULL, 0);

#ifdef STANDALONE_GUI
	fini_gui();
#endif

	DPRINTF(("main done - returning %d\n", r));
	return r;
}


/* display an error message when all else fails */
int
error(Retcode r)
{
	const char *err;
	
	if (r == NO_ERROR || r == R_END)
		return 0;
	
	err = err2str(r);
	
	if (err == NULL || strcmp(err, "unknown error") == 0)
		cprintf(g_e, "unknown error: %d\n", r);
	else
		cprintf(g_e, "error: %s\n", err);
	
	if (g_e->showstack)
	{
		display_stack(g_e);
		display_rstack(g_e);
		display_ftrace(g_e);
		display_exec_stack(g_e);
	}
	
	g_e->rsp = g_e->rstack;		/* clear the return stack */
	return -1;
}
