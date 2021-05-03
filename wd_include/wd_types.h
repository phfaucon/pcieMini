#ifndef _WD_TYPES_H_
#define _WD_TYPES_H_

#if defined(__cplusplus)
    extern "C" {
#endif

    typedef unsigned char u8;
    typedef unsigned short u16;
    typedef unsigned int u32;

    #if defined(WINNT)
        typedef unsigned __int64 u64;
    #else
        typedef unsigned long long u64;
    #endif

#if defined(__cplusplus)
    }
#endif

#endif /* _WD_TYPES_H_ */

