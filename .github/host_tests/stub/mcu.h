// Stand-in for r01lib's mcu.h when the core's portable files are built on
// the host (.github/host_tests): panic() reports and exits
#pragma once
#include <cstdio>
#include <cstdlib>
[[noreturn]] inline void panic( const char *s )
{
	std::printf( "panic: %s\n", s );
	std::exit( 2 );
}
