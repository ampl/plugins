/*	Copyright (c) 2010 AMPL Optimization LLC	*/
/*	  All Rights Reserved 	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AMPL Optimization LLC.	*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

/* Example encoding program that uses a trivial encoding:
 *	Let c[i] denote input character i.
 *	This input character is mapped to (i + key + c[i]) mod 256,
 *	where key (an integer) is specified on the command line.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

 static char *progname;

 static int
usage(int rc)
{
	fprintf(rc ? stderr : stdout, "Usage: %s [-d] [-k key] [file [file...]]]\n\
	to encode the concatenated input files (default stdin), replacing\n\
	c[i] = character i of the concatenated input files with\n\
	(c[i] + key + i) mod 256 in the output (written to stdout).\n\
	The default key is 0.  Option -d reverses the process, so\n\n\
		%s | %s -d\n\n\
	is idempotent.\n", progname, progname, progname);
	return rc;
	}

 int
main(int argc, char **argv)
{
	FILE *inf, *outf;
	char *s, *se;
	int c, decode;
	unsigned int key;

	progname = *argv;
	outf = stdout;
	decode = 0;
	key = 0;

	while ((s = *++argv) && *s == '-') {
 nextopt:
	  switch(s[1]) {
		case '?':
			return usage(s[2] != 0);
		case '-':
			if (!s[2]) {
				s = *++argv;
				goto break2;
				}
			return usage(strcmp(s,"--help") ? 1 : 0);
		case 'd':
			decode = 1;
			if (s[2]) {
				++s;
				goto nextopt;
				}
			break;
		case 'k':
			if (s[2])
				s += 2;
			else if (!(s = *++argv))
				return usage(1);
			key = (unsigned int)strtol(s,&se,10);
			if (*se) {
				fprintf(stderr,
					"%s: invalid key \"%s\"; expected a decimal integer.\n",
					progname, s);
				return 1;
				}
			break;
		default:
			return usage(1);
		}
	  }
 break2:
#ifdef _WIN32
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
#endif
	for(;;) {
		inf = stdin;
		if (s && !(inf = fopen(s, "rb"))) {
			fprintf(stderr, "%s: cannot open input file \"%s\"\n", progname, s);
			return 1;
			}
		if (decode)
			while((c = getc(inf)) != EOF)
				putc((char)(c - key++), outf);
		else
			while((c = getc(inf)) != EOF)
				putc((char)(c + key++), outf);
		if (!s)
			break;
		fclose(inf);
		if (!(s = *++argv))
			break;
		}
	return 0;
	}
