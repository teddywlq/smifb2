// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023, SiliconMotion Inc.


#ifndef _SMI_SYSFS_H_
#define _SMI_SYSFS_H_



#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
        void smi_debugfs_init(struct drm_minor *minor);
#else
        int smi_debugfs_init(struct drm_minor *minor);
#endif


#endif
