#ifndef __SMI_VERSION_H__
#define __SMI_VERSION_H__

#include <linux/version.h>
#ifdef RHEL_MAJOR
#undef LINUX_VERSION_CODE
#if RHEL_MAJOR==8

#if RHEL_MINOR==2
#define LINUX_VERSION_CODE 0x050300
#elif RHEL_MINOR==4
#define LINUX_VERSION_CODE 0x050900
#elif RHEL_MINOR==6
#define LINUX_VERSION_CODE 0x050e15//5.14.21
#endif

#endif//RHEL_MAJOR==8

#if RHEL_MAJOR==9
#if RHEL_MINOR==3
#define LINUX_VERSION_CODE 0x060300//6.3.0
#endif
#endif//RHEL_MAJOR==9

#endif

#endif
