/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef _LIBSSUSB_MATH_UTILITIES_H_
#define _LIBSSUSB_MATH_UTILITIES_H_

#include <sys/cdefs.h>

#ifndef min
#define min(a, b)                                                              \
        __extension__ ({ __typeof__ (a) _a = (a);                              \
           __typeof__ (b) _b = (b);                                            \
           (_a < _b) ? _a : _b;                                                \
        })
#endif /* !min */

#ifndef max
#define max(a, b)                                                              \
        __extension__ ({ __typeof__ (a) _a = (a);                              \
           __typeof__ (b) _b = (b);                                            \
           (_a > _b) ? _a : _b;                                                \
        })
#endif /* !max */

#endif /* !_LIBSSUSB_MATH_UTILITIES_H_ */
