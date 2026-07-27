/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PLIC_H
#define _LINUX_PLIC_H

#include <linux/types.h>

int plic_set_irq_priority(unsigned int irq, u32 priority);

#endif /* _LINUX_PLIC_H */
