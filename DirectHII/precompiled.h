
#define _WINSOCKAPI_    // stops windows.h including winsock.h

// disable certain warnings
#pragma warning(disable : 4305) // truncation from double to float
#pragma warning(disable : 4018) // <, >, signed/unsigned mismatch
#pragma warning(disable : 4389) // != signed/unsigned mismatch
#pragma warning(disable : 4244) // conversion from double to float
#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
#pragma warning(disable : 4706) // assignment within conditional expression
#pragma warning(disable : 4204) // nonstandard extension used : non-constant aggregate initializer

#include <windows.h>
#include <stddef.h>

#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <malloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <dsound.h>

#include <windowsx.h>
#include <mmsystem.h>
#include <assert.h>
#include <stdio.h>

#include <fcntl.h>

#include <windowsx.h>
#include <commctrl.h>
#include <memory.h>
#include <mmreg.h>

#include <time.h>

#include <wsipx.h>

#include <math.h>

// handful of defines that need to be shared across the renderer and the engine
#define	MAX_LIGHTSTYLES	64

