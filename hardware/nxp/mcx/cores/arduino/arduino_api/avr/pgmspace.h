/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  avr-libc's <avr/pgmspace.h> for this core, so AVR-era sketches and
 *  libraries that put constants in flash build unchanged. Arduino.h
 *  includes it, as AVR's does.
 *
 *  AVR is a Harvard-architecture chip: flash and RAM are separate address
 *  spaces, so data tagged PROGMEM needs special instructions
 *  (pgm_read_byte() and the _P string functions) to read back. On this
 *  Cortex-M, flash is ordinary memory, readable like anything else, so
 *  PROGMEM is empty, the reads are plain pointer reads and each _P function
 *  is the ordinary one. The "near" and "far" forms are the same thing too:
 *  there is only one 32-bit address space.
 *
 *  Original implementation written to avr-libc's documented interface
 *  (avr-libc is under the Modified BSD License; none of its source is
 *  copied). Everything is a macro, as in avr-libc, so the common
 *  "#ifndef pgm_read_byte_near" checks in libraries see them.
 *  Usable from C as well as C++.
 */

#ifndef R01LIB_ARDUINO_AVR_PGMSPACE_H
#define R01LIB_ARDUINO_AVR_PGMSPACE_H

#include	<stdint.h>
#include	<stddef.h>
#include	<string.h>

/** @name Placing data in flash
 *  Nothing to do here: a `const` object already goes to flash.
 */
///@{
#define	PROGMEM
#define	PGM_P				const char *
#define	PGM_VOID_P			const void *
/** Identity -- string literals need no special flash placement here */
#define	PSTR( s )			( s )
///@}

/** @name avr-libc's old PROGMEM types (deprecated there, still in old code) */
///@{
typedef void			prog_void;
typedef char			prog_char;
typedef unsigned char	prog_uchar;
typedef int8_t			prog_int8_t;
typedef uint8_t			prog_uint8_t;
typedef int16_t			prog_int16_t;
typedef uint16_t		prog_uint16_t;
typedef int32_t			prog_int32_t;
typedef uint32_t		prog_uint32_t;
typedef int64_t			prog_int64_t;
typedef uint64_t		prog_uint64_t;
///@}

/** A "far" flash address, as an integer (any address, here) */
typedef uint32_t		uint_farptr_t;
/** @param var an object in flash @return its address as a uint_farptr_t */
#define	pgm_get_far_address( var )	( (uint_farptr_t)(uintptr_t)&( var ) )

/*  The multi-byte reads go through memcpy(), so they work at any address,
 *  as on AVR, where a word is read a byte at a time: AVR code reads words
 *  out of byte tables at odd offsets. A Cortex-M33 reads an unaligned word
 *  anyway, and the compiler turns these into the same single load there,
 *  but a Cortex-M0+ would fault. */
static inline uint16_t	mcx_pgm_read_word( const void *p )	{ uint16_t v; memcpy( &v, p, sizeof( v ) ); return v; }
static inline uint32_t	mcx_pgm_read_dword( const void *p )	{ uint32_t v; memcpy( &v, p, sizeof( v ) ); return v; }
static inline uint64_t	mcx_pgm_read_qword( const void *p )	{ uint64_t v; memcpy( &v, p, sizeof( v ) ); return v; }
static inline float		mcx_pgm_read_float( const void *p )	{ float v; memcpy( &v, p, sizeof( v ) ); return v; }
static inline void		*mcx_pgm_read_ptr( const void *p )	{ void *v; memcpy( &v, p, sizeof( v ) ); return v; }

/** @name Reading from flash: plain reads, at any alignment
 *  addr may be a pointer, or for the _far forms a uint_farptr_t.
 */
///@{
#define	pgm_read_byte( addr )		( *(const uint8_t *)(uintptr_t)(addr) )
#define	pgm_read_word( addr )		mcx_pgm_read_word( (const void *)(uintptr_t)(addr) )
#define	pgm_read_dword( addr )		mcx_pgm_read_dword( (const void *)(uintptr_t)(addr) )
#define	pgm_read_qword( addr )		mcx_pgm_read_qword( (const void *)(uintptr_t)(addr) )
#define	pgm_read_float( addr )		mcx_pgm_read_float( (const void *)(uintptr_t)(addr) )
#define	pgm_read_ptr( addr )		mcx_pgm_read_ptr( (const void *)(uintptr_t)(addr) )

#define	pgm_read_byte_near( addr )	pgm_read_byte( addr )
#define	pgm_read_word_near( addr )	pgm_read_word( addr )
#define	pgm_read_dword_near( addr )	pgm_read_dword( addr )
#define	pgm_read_qword_near( addr )	pgm_read_qword( addr )
#define	pgm_read_float_near( addr )	pgm_read_float( addr )
#define	pgm_read_ptr_near( addr )	pgm_read_ptr( addr )

#define	pgm_read_byte_far( addr )	pgm_read_byte( addr )
#define	pgm_read_word_far( addr )	pgm_read_word( addr )
#define	pgm_read_dword_far( addr )	pgm_read_dword( addr )
#define	pgm_read_qword_far( addr )	pgm_read_qword( addr )
#define	pgm_read_float_far( addr )	pgm_read_float( addr )
#define	pgm_read_ptr_far( addr )	pgm_read_ptr( addr )
///@}

// newlib has these but leaves them undeclared under -std=c++20, as they
// are BSD/GNU rather than ISO C. Declared here as newlib declares them,
// rather than by including <strings.h> or switching to -std=gnu++20,
// which would also declare index() and clash with a sketch's global
// `int index` (see Arduino.h).
#ifdef __cplusplus
extern "C" {
#endif
int		strcasecmp( const char *s1, const char *s2 );
int		strncasecmp( const char *s1, const char *s2, size_t n );
void	*memrchr( const void *s, int c, size_t n );
char	*strchrnul( const char *s, int c );
char	*strcasestr( const char *haystack, const char *needle );
size_t	strlcpy( char *dst, const char *src, size_t size );
size_t	strlcat( char *dst, const char *src, size_t size );
size_t	strnlen( const char *s, size_t maxlen );
char	*strsep( char **stringp, const char *delim );
char	*strtok_r( char *__restrict str, const char *__restrict delim, char **__restrict saveptr );
void	*memccpy( void *__restrict dst, const void *__restrict src, int c, size_t n );
void	*memmem( const void *haystack, size_t hlen, const void *needle, size_t nlen );
#ifdef __cplusplus
}
#endif

/** @name The _P string and memory functions: the ordinary ones
 *  Each takes its flash-side argument (the one avr-libc reads from flash)
 *  as an ordinary pointer.
 */
///@{
#define	memchr_P( s, c, n )					memchr( (s), (c), (n) )
#define	memcmp_P( s1, s2, n )				memcmp( (s1), (s2), (n) )
#define	memccpy_P( dst, src, c, n )			memccpy( (dst), (src), (c), (n) )
#define	memcpy_P( dst, src, n )				memcpy( (dst), (src), (n) )
#define	memmem_P( s1, n1, s2, n2 )			memmem( (s1), (n1), (s2), (n2) )
#define	memrchr_P( s, c, n )				memrchr( (s), (c), (n) )
#define	strcat_P( dst, src )				strcat( (dst), (src) )
#define	strchr_P( s, c )					strchr( (s), (c) )
#define	strchrnul_P( s, c )					strchrnul( (s), (c) )
#define	strcmp_P( s1, s2 )					strcmp( (s1), (s2) )
#define	strcpy_P( dst, src )				strcpy( (dst), (src) )
#define	strcasecmp_P( s1, s2 )				strcasecmp( (s1), (s2) )
#define	strcasestr_P( s1, s2 )				strcasestr( (s1), (s2) )
#define	strcspn_P( s, reject )				strcspn( (s), (reject) )
#define	strlcat_P( dst, src, size )			strlcat( (dst), (src), (size) )
#define	strlcpy_P( dst, src, size )			strlcpy( (dst), (src), (size) )
#define	strlen_P( s )						strlen( (s) )
#define	strnlen_P( s, n )					strnlen( (s), (n) )
#define	strncmp_P( s1, s2, n )				strncmp( (s1), (s2), (n) )
#define	strncasecmp_P( s1, s2, n )			strncasecmp( (s1), (s2), (n) )
#define	strncat_P( dst, src, n )			strncat( (dst), (src), (n) )
#define	strncpy_P( dst, src, n )			strncpy( (dst), (src), (n) )
#define	strpbrk_P( s, accept )				strpbrk( (s), (accept) )
#define	strrchr_P( s, c )					strrchr( (s), (c) )
#define	strsep_P( sp, delim )				strsep( (sp), (delim) )
#define	strspn_P( s, accept )				strspn( (s), (accept) )
#define	strstr_P( s1, s2 )					strstr( (s1), (s2) )
#define	strtok_P( s, delim )				strtok( (s), (delim) )
#define	strtok_rP( s, delim, last )			strtok_r( (s), (delim), (last) )
///@}

/** @name The _PF forms: flash side given as a uint_farptr_t */
///@{
#define	memcmp_PF( s1, s2, n )				memcmp( (s1), (const void *)(uintptr_t)(s2), (n) )
#define	memcpy_PF( dst, src, n )			memcpy( (dst), (const void *)(uintptr_t)(src), (n) )
#define	strcat_PF( dst, src )				strcat( (dst), (const char *)(uintptr_t)(src) )
#define	strcmp_PF( s1, s2 )					strcmp( (s1), (const char *)(uintptr_t)(s2) )
#define	strcpy_PF( dst, src )				strcpy( (dst), (const char *)(uintptr_t)(src) )
#define	strcasecmp_PF( s1, s2 )				strcasecmp( (s1), (const char *)(uintptr_t)(s2) )
#define	strlcat_PF( dst, src, size )		strlcat( (dst), (const char *)(uintptr_t)(src), (size) )
#define	strlcpy_PF( dst, src, size )		strlcpy( (dst), (const char *)(uintptr_t)(src), (size) )
#define	strlen_PF( s )						strlen( (const char *)(uintptr_t)(s) )
#define	strnlen_PF( s, n )					strnlen( (const char *)(uintptr_t)(s), (n) )
#define	strncmp_PF( s1, s2, n )				strncmp( (s1), (const char *)(uintptr_t)(s2), (n) )
#define	strncasecmp_PF( s1, s2, n )			strncasecmp( (s1), (const char *)(uintptr_t)(s2), (n) )
#define	strncat_PF( dst, src, n )			strncat( (dst), (const char *)(uintptr_t)(src), (n) )
#define	strncpy_PF( dst, src, n )			strncpy( (dst), (const char *)(uintptr_t)(src), (n) )
#define	strstr_PF( s1, s2 )					strstr( (s1), (const char *)(uintptr_t)(s2) )
///@}

/** @name The _P forms of the printf/scanf family: the format string is the ordinary one
 *  As on AVR, a sketch using these includes <stdio.h> itself; this header
 *  doesn't, to keep its names out of every sketch.
 */
///@{
#define	printf_P( ... )						printf( __VA_ARGS__ )
#define	sprintf_P( dst, ... )				sprintf( (dst), __VA_ARGS__ )
#define	snprintf_P( dst, n, ... )			snprintf( (dst), (n), __VA_ARGS__ )
#define	vsprintf_P( dst, fmt, ap )			vsprintf( (dst), (fmt), (ap) )
#define	vsnprintf_P( dst, n, fmt, ap )		vsnprintf( (dst), (n), (fmt), (ap) )
#define	fprintf_P( stream, ... )			fprintf( (stream), __VA_ARGS__ )
#define	vfprintf_P( stream, fmt, ap )		vfprintf( (stream), (fmt), (ap) )
#define	puts_P( s )							puts( (s) )
#define	fputs_P( s, stream )				fputs( (s), (stream) )
#define	sscanf_P( s, ... )					sscanf( (s), __VA_ARGS__ )
#define	vsscanf_P( s, fmt, ap )				vsscanf( (s), (fmt), (ap) )
///@}

#endif // !R01LIB_ARDUINO_AVR_PGMSPACE_H
