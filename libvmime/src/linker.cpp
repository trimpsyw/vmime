
//
// Functionality common to all protocols 
// Copyright (C) Elmue
//
#include "vmime/vmime.hpp"
#include <windows.h>  // NEVER include windows.h in a header file in a Managed C++ project !!

// ######################################################################################
//                                  Linker directives
// ######################################################################################

// Required Dll: 
// libgsasl-7.dll
#ifdef WIN64
    #pragma comment(lib, "../libvmime/src/gsasl/libgsasl-7_64.lib") // 64 Bit, Release
#else
    #pragma comment(lib, "../libvmime/src/gsasl/libgsasl-7_32.lib") // 32 Bit, Release
#endif

// Required Dll's: 
// libgnutls-28.dll, 
// libnettle-4-7.dll, 
// libhogweed-2-5.dll, 
// libgmp-10.dll, 
// libp11-kit-0.dll 
#if VMIME_TLS_SUPPORT_LIB_IS_GNUTLS
    #error GnuTLS does not run correctly on Windows.
    #error Additionally a 64 bit version does not exist.
#endif

// The following LIB's may be:
// -- dynamic -> require libeay32.dll and ssleay32.dll
// -- static  -> no Dlls required
// IMPORTANT: Read documentation about the required VC++ runtime!
#if VMIME_TLS_SUPPORT_LIB_IS_OPENSSL

    // You can compile against the Debug version of openssl by setting the following definition = 1
    // But these Lib files are not included, because their total size is 24 MB
    #define DEBUG_LIBS_PRESENT   0

    #ifdef WIN64
        #if _DEBUG && DEBUG_LIBS_PRESENT
            #pragma comment(lib, "../libvmime/src/openssl/libeay64MDd.lib") // MultiThreadedDll, 64 Bit, Debug
            #pragma comment(lib, "../libvmime/src/openssl/ssleay64MDd.lib")
        #else
            #pragma comment(lib, "../libvmime/src/openssl/libeay64MD.lib")  // MultiThreadedDll, 64 Bit, Release
            #pragma comment(lib, "../libvmime/src/openssl/ssleay64MD.lib")
        #endif
    #else
        #if _DEBUG && DEBUG_LIBS_PRESENT
            #pragma comment(lib, "../libvmime/src/openssl/libeay32MDd.lib") // MultiThreadedDll, 32 Bit, Debug
            #pragma comment(lib, "../libvmime/src/openssl/ssleay32MDd.lib")
        #else
            #pragma comment(lib, "../libvmime/src/openssl/libeay32MD.lib")  // MultiThreadedDll, 32 Bit, Release
            #pragma comment(lib, "../libvmime/src/openssl/ssleay32MD.lib")
        #endif
    #endif
    #pragma comment(lib, "Crypt32.lib") // Microsoft Crypt32.dll
#endif