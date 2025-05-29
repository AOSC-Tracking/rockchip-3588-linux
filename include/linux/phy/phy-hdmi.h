/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright 2022,2024 NXP
 */

#ifndef __PHY_HDMI_H_
#define __PHY_HDMI_H_

#define PHY_SUBMODE_TMDS	0
#define PHY_SUBMODE_FRL		1

/**
 * struct phy_configure_opts_hdmi - HDMI configuration set
 * @tmds_char_rate: HDMI TMDS Character Rate in Hertz.
 * @bpc: Bits per color channel.
 *
 * This structure is used to represent the configuration state of a HDMI phy.
 */
struct phy_configure_opts_hdmi {
	//TODO: rename to "rate" so that it applies to FRL as well
	unsigned long long tmds_char_rate;
	unsigned int bpc;
};

#endif /* __PHY_HDMI_H_ */
