/** Time zone handling lib as a part of r01lib/r01device
 * 	based on https://github.com/gitpan/Time-OlsonTZ-Data
 *  
 *	r01lib/r01device by Tedd OKANO
 *  Released under the MIT license License
 */

#ifdef __cplusplus
extern "C" {
#endif

/*	$OpenBSD: strptime.c,v 1.31 2023/03/02 16:21:51 millert Exp $ */
/*	$NetBSD: strptime.c,v 1.12 1998/01/20 21:39:40 mycroft Exp $	*/
/*-
 * Copyright (c) 1997, 1998, 2005, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was contributed to The NetBSD Foundation by Klaus Klein.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include	<stdint.h>
typedef		uint8_t	u_char;

#define _CurrentTimeLocale (&dummy_locale_info)

struct dummy_locale_info_t {
	char d_t_fmt[15];
	char t_fmt_ampm[12];
	char t_fmt[9];
	char d_fmt[9];
	char day[7][10];
	char abday[7][4];
	char mon[12][10];
	char abmon[12][4];
	char am_pm[2][3];
};


static const struct dummy_locale_info_t dummy_locale_info = {
	"%a %b %d %X %Y",		/* %c */
	"%I:%M:%S %p",		/* %r */
	"%H:%M:%S",			/* %X */
	"%m/%d/%y",			/* %x */
	{ "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
	  "Saturday" },
	{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" },
	{ "January", "February", "March", "April", "May", "June",
	  "July", "August", "September", "October", "November", "December" },
	{ "Jan", "Feb", "Mar", "Apr", "May", "Jun",
	  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" },
	{ "AM", "PM" },
};

char *strptime( const char *buf, const char *fmt, struct tm *tm );

#ifdef __cplusplus
}
#endif
