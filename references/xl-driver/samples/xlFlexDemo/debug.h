/*-------------------------------------------------------------------------------------------
| File        : debug.h
| Project     : Vector FlexRay Example
|
| Description : Shows the basic FlexRay functionality for the XL Driver Library
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2012 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/

/* defines for debugging */
#define DEBUG_OFF           99
#define DEBUG_BASIC         03 
#define DEBUG_ADV           02
#define DEBUG_EXP           01

static int most_debug = DEBUG_EXP;

// debug version
#ifdef DEBUG

    #define XLDEBUG(x, outp)        ( dbg_p(x, outp ) )

    _inline void dbg_p (char i, char *fmt, ...)
    {
      char tmp[500];
      if (i>=most_debug){

        sprintf (tmp, "xlFlexDemo: %s", fmt);

        OutputDebugString (tmp);
      }
    } 
    
// release version.
#else
    #define XLDEBUG(x, outp)     
#endif