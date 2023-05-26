// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023, SiliconMotion Inc.


#ifndef _SMI_SYSFS_H_
#define _SMI_SYSFS_H_

struct device;

void smi_sysfs_init(struct kobject *);
void smi_sysfs_deinit(struct kobject *);

#endif
