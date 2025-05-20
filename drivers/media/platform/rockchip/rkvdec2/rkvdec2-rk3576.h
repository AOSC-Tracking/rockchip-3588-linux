/* SPDX-License-Identifier: (GPL-2.0) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd
 *
 * author:
 *	Ding Wei <leo.ding@rock-chips.com>
 */

#ifndef __ROCKCHIP_MPP_HACK_RK3576_H__
#define __ROCKCHIP_MPP_HACK_RK3576_H__

#include "rkvdec2.h"

#define RK3576_HACK_MAGIC	(0x76543210)

int rk3576_workaround_init(struct rkvdec2_dev*);
int rk3576_workaround_exit(struct rkvdec2_dev*);
int rk3576_workaround_run(struct rkvdec2_dev*);

#endif
