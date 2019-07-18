#ifndef _TINYCC_CRASH_H
#define _TINYCC_CRASH_H

typedef int (*CrashHandlerCallback)( const char * rason, const char * stacktrace );	//Return 0 to attempt continuation of execution.

void InstallTCCCrashHandler( CrashHandlerCallback h );

#endif

