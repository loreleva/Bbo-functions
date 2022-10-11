
#pragma once


// cross OS stuff


#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
	#include "unistd_win.h"
#else
	#include <unistd.h>
	#include <sys/param.h>
#endif
