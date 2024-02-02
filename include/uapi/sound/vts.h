#ifndef _UAPI__SOUND_VTS_H
#define _UAPI__SOUND_VTS_H

#if defined(__KERNEL__) || defined(__linux__)
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#endif

#ifndef __KERNEL__
#include <stdlib.h>
#endif

#define VTSDRV_MISC_IOCTL_WRITE_SVOICE		 _IOW('V', 0x00, int)
#define VTSDRV_MISC_IOCTL_WRITE_GOOGLE		 _IOW('V', 0x01, int)
#define VTSDRV_MISC_IOCTL_READ_GOOGLE_VERSION	 _IOR('V', 0x02, int)
#define VTSDRV_MISC_IOCTL_READ_EVENT_TYPE 	 _IOR('V', 0x03, int)
#define VTSDRV_MISC_IOCTL_WRITE_EXIT_POLLING     _IOW('V', 0x04, int)
#define VTSDRV_MISC_IOCTL_READ_TRIGGERED_KEYWORD _IOR('V', 0x05, int)

#define VTSDRV_MISC_MODEL_BIN_MAXSZ  0x10800

#endif /* _UAPI__SOUND_VTS_H */

