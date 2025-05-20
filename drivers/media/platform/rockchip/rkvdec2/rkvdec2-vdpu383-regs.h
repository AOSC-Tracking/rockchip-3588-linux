/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Rockchip Video Decoder 2 driver registers description
 *
 * Copyright (C) 2024 Collabora, Ltd.
 *  Detlev Casanova <detlev.casanova@collabora.com>
 */

#ifndef _RKVDEC_VDPU838_REGS_H_
#define _RKVDEC_VDPU838_REGS_H_

#include <linux/types.h>

#define VDPU383_OFFSET_COMMON_REGS		(8 * sizeof(u32))
#define VDPU383_OFFSET_CODEC_PARAMS_REGS	(64 * sizeof(u32))
#define VDPU383_OFFSET_COMMON_ADDR_REGS		(128 * sizeof(u32))
#define VDPU383_OFFSET_CODEC_ADDR_REGS		(168 * sizeof(u32))
#define VDPU383_OFFSET_POC_HIGHBIT_REGS		(200 * sizeof(u32))

#define RKVDEC2_MODE_HEVC	0
#define RKVDEC2_MODE_H264	1
#define RKVDEC2_MODE_VP9	2
#define RKVDEC2_MODE_AVS2	3
//TODO: Add AV1

#define MAX_SLICE_NUMBER	0x3fff

#define RKVDEC2_1080P_PIXELS		(1920 * 1080)
#define RKVDEC2_4K_PIXELS		(4096 * 2304)
#define RKVDEC2_8K_PIXELS		(7680 * 4320)
#define RKVDEC2_TIMEOUT_1080p		(0xffffff) //Check downstream values
#define RKVDEC2_TIMEOUT_4K		(0x2cfffff)
#define RKVDEC2_TIMEOUT_8K		(0x4ffffff)
#define RKVDEC2_TIMEOUT_MAX		(0xffffffff)

#define VDPU383_REG_DEC_E		0x100
#define VDPU383_DEC_E_BIT		BIT(0)

#define VDPU383_REG_IMPORTANT_EN	0x024
#define VDPU383_DEC_IRQ_DISABLE		0

#define VDPU383_REG_STA_INT		0x03c
#define VDPU383_STA_INT_DEC_RDY_STA	BIT(0)
#define VDPU383_STA_INT_SOFTRESET_RDY	BIT(10) // FIXME: Could be 11 or could be both, TBD

struct Vdpu383RegLlp {
    struct SWREG0_LINK_MODE {
        u32 llp_mmu_zap_cache_dis          : 1;
        u32 reserve0                       : 15;
        u32 core_work_mode                 : 1;
        u32 ccu_core_work_mode             : 1;
        u32 reserve1                       : 3;
        u32 ltb_pause_flag                 : 1;
        u32 reserve2                       : 10;
    } reg0;

    struct SWREG1_CFG_START_ADDR {
        u32 reserve0                       : 4;
        u32 reg_cfg_addr                   : 28;
    } reg1;

    struct SWREG2_LINK_MODE {
        u32 pre_frame_num                  : 30;
        u32 reserve0                       : 1;
        u32 link_mode                      : 1;
    } reg2;

    /* SWREG3_CONFIG_DONE */
    u32 reg3_done;

    /* SWREG4_DECODERED_NUM */
    u32 reg4_num;

    /* SWREG5_DEC_TOTAL_NUM */
    u32 reg5_total_num;

    /* SWREG6_LINK_MODE_EN */
    u32 reg6_mode_en;

    /* SWREG7_SKIP_NUM */
    u32 reg7_num;

    /* SWREG8_CUR_LTB_IDX */
    u32 reg8_ltb_idx;

    u32 reserve_reg9_15[7];

    /* SWREG16_DEC_E */
    u32 reg16_dec_e;

    /* SWREG17_SOFT_RST */
    u32 reg17_rkvdec_ip_rst_p;

    struct SWREG18_IRQ {
        u32 rkvdec_irq                     : 1;
        u32 rkvdec_line_irq                : 1;
        u32 reserve0                       : 14;
        u32 wmask                          : 2;
        u32 reserve1                       : 14;
    } reg18;

    struct SWREG19_STA {
        u32 rkvdec_frame_rdy_sta           : 1;
        u32 rkvdec_strm_error_sta          : 1;
        u32 rkvdec_core_timeout_sta        : 1;
        u32 rkvdec_ip_timeout_sta          : 1;
        u32 rkvdec_bus_error_sta           : 1;
        u32 rkvdec_buffer_empty_sta        : 1;
        u32 rkvdec_colmv_ref_error_sta     : 1;
        u32 rkvdec_error_spread_sta        : 1;
        u32 create_core_timeout_sta        : 1;
        u32 wlast_miss_match_sta           : 1;
        u32 rkvdec_core_rst_rdy_sta        : 1;
        u32 rkvdec_ip_rst_rdy_sta          : 1;
        u32 force_busidle_rdy_sta          : 1;
        u32 ltb_pause_rdy_sta              : 1;
        u32 ltb_end_flag                   : 1;
        u32 unsupport_decmode_error_sta    : 1;
        u32 wmask_bits                     : 15;
        u32 reserve0                       : 1;
    } reg19;

    u32 reserve_reg20;

    /* SWREG21_IP_TIMEOUT_THRESHOD */
    u32 reg21_ip_timeout_threshold;

    struct SWREG22_IP_EN {
        u32 ip_timeout_pause_flag          : 1;
        u32 reserve0                       : 3;
        u32 auto_reset_dis                 : 1;
        u32 reserve1                       : 3;
        u32 force_busidle_req_flag         : 1;
        u32 reserve2                       : 3;
        u32 bus_clkgate_dis                : 1;
        u32 ctrl_clkgate_dis               : 1;
        u32 reserve3                       : 1;
        u32 irq_dis                        : 1;
        u32 wid_reorder_dis                : 1;
        u32 reserve4                       : 7;
        u32 clk_cru_mode                   : 2;
        u32 reserve5                       : 5;
        u32 mmu_sel                        : 1;
    } reg22;

    struct SWREG23_IN_OUT {
        u32 endian                         : 1;
        u32 swap32_e                       : 1;
        u32 swap64_e                       : 1;
        u32 str_endian                     : 1;
        u32 str_swap32_e                   : 1;
        u32 str_swap64_e                   : 1;
        u32 reserve0                       : 26;
    } reg23;

    /* SWREG24_EXTRA_STRM_BASE */
    u32 reg24_extra_stream_base;

    /* SWREG25_EXTRA_STRM_LEN */
    u32 reg25_extra_stream_len;

    /* SWREG26_EXTRA_STRM_PARA_SET */
    u32 reg26_extra_strm_start_bit;

    /* SWREG27_BUF_EMPTY_RESTART */
    u32 reg27_buf_emtpy_restart_p;

    /* SWREG28_RCB_BASE */
    u32 reg28_rcb_base;

};

struct vdpu383_regs_common {
    /* SWREG8_DEC_MODE */
    u32 reg8_dec_mode;

    struct SWREG9_IMPORTANT_EN {
        u32 fbc_e                          : 1;
        u32 tile_e                         : 1;
        u32 reserve0                       : 2;
        u32 buf_empty_en                   : 1;
        u32 scale_down_en                  : 1;
        u32 reserve1                       : 1;
        u32 pix_range_det_e                : 1;
        u32 av1_fgs_en                     : 1;
        u32 reserve2                       : 7;
        u32 line_irq_en                    : 1;
        u32 out_cbcr_swap                  : 1;
        u32 fbc_force_uncompress           : 1;
        u32 fbc_sparse_mode                : 1;
        u32 reserve3                       : 12;
    } reg9;

    struct SWREG10_BLOCK_GATING_EN {
        u32 strmd_auto_gating_e            : 1;
        u32 inter_auto_gating_e            : 1;
        u32 intra_auto_gating_e            : 1;
        u32 transd_auto_gating_e           : 1;
        u32 recon_auto_gating_e            : 1;
        u32 filterd_auto_gating_e          : 1;
        u32 bus_auto_gating_e              : 1;
        u32 ctrl_auto_gating_e             : 1;
        u32 rcb_auto_gating_e              : 1;
        u32 err_prc_auto_gating_e          : 1;
        u32 reserve0                       : 22;
    } reg10;

    struct SWREG11_CFG_PARA {
        u32 reserve0                       : 9;
        u32 dec_timeout_dis                : 1;
        u32 reserve1                       : 22;
    } reg11;

    struct SWREG12_CACHE_HASH_MASK {
        u32 reserve0                       : 7;
        u32 cache_hash_mask                : 25;
    } reg12;

    /* SWREG13_CORE_TIMEOUT_THRESHOLD */
    u32 reg13_core_timeout_threshold;

    struct SWREG14_LINE_IRQ_CTRL {
        u32 dec_line_irq_step              : 16;
        u32 dec_line_offset_y_st           : 16;
    } reg14;

    /* copy from llp, media group add */
    struct SWREG15_IRQ_STA {
        u32 rkvdec_frame_rdy_sta           : 1;
        u32 rkvdec_strm_error_sta          : 1;
        u32 rkvdec_core_timeout_sta        : 1;
        u32 rkvdec_ip_timeout_sta          : 1;
        u32 rkvdec_bus_error_sta           : 1;
        u32 rkvdec_buffer_empty_sta        : 1;
        u32 rkvdec_colmv_ref_error_sta     : 1;
        u32 rkvdec_error_spread_sta        : 1;
        u32 create_core_timeout_sta        : 1;
        u32 wlast_miss_match_sta           : 1;
        u32 rkvdec_core_rst_rdy_sta        : 1;
        u32 rkvdec_ip_rst_rdy_sta          : 1;
        u32 force_busidle_rdy_sta          : 1;
        u32 ltb_pause_rdy_sta              : 1;
        u32 ltb_end_flag                   : 1;
        u32 unsupport_decmode_error_sta    : 1;
        u32 wmask_bits                     : 15;
        u32 reserve0                       : 1;
    } reg15;

    struct SWREG16_ERROR_CTRL_SET {
        u32 error_proc_disable             : 1;
        u32 reserve0                       : 7;
        u32 error_spread_disable           : 1;
        u32 reserve1                       : 15;
        u32 roi_error_ctu_cal_en           : 1;
        u32 reserve2                       : 7;
    } reg16;

    struct SWREG17_ERR_ROI_CTU_OFFSET_START {
        u32 roi_x_ctu_offset_st            : 12;
        u32 reserve0                       : 4;
        u32 roi_y_ctu_offset_st            : 12;
        u32 reserve1                       : 4;
    } reg17;

    struct SWREG18_ERR_ROI_CTU_OFFSET_END {
        u32 roi_x_ctu_offset_end           : 12;
        u32 reserve0                       : 4;
        u32 roi_y_ctu_offset_end           : 12;
        u32 reserve1                       : 4;
    } reg18;

    struct SWREG19_ERROR_REF_INFO {
        u32 avs2_ref_error_field           : 1;
        u32 avs2_ref_error_topfield        : 1;
        u32 ref_error_topfield_used        : 1;
        u32 ref_error_botfield_used        : 1;
        u32 reserve0                       : 28;
    } reg19;

    /* SWREG20_CABAC_ERROR_EN_LOWBITS */
    u32 reg20_cabac_error_en_lowbits;

    /* SWREG21_CABAC_ERROR_EN_HIGHBITS */
    u32 reg21_cabac_error_en_highbits;

    u32 reserve_reg22;

    struct SWREG23_INVALID_PIXEL_FILL {
        u32 fill_y                         : 10;
        u32 fill_u                         : 10;
        u32 fill_v                         : 10;
        u32 reserve0                       : 2;
    } reg23;

    u32 reserve_reg24_26[3];

    struct SWREG27_ALIGN_EN {
        u32 reserve0                       : 4;
        u32 ctu_align_wr_en                : 1;
        u32 reserve1                       : 27;
    } reg27;

    struct SWREG28_DEBUG_PERF_LATENCY_CTRL0 {
        u32 axi_perf_work_e                : 1;
        u32 reserve0                       : 2;
        u32 axi_cnt_type                   : 1;
        u32 rd_latency_id                  : 8;
        u32 reserve1                       : 4;
        u32 rd_latency_thr                 : 12;
        u32 reserve2                       : 4;
    } reg28;

    struct SWREG29_DEBUG_PERF_LATENCY_CTRL1 {
        u32 addr_align_type                : 2;
        u32 ar_cnt_id_type                 : 1;
        u32 aw_cnt_id_type                 : 1;
        u32 ar_count_id                    : 8;
        u32 reserve0                       : 4;
        u32 aw_count_id                    : 8;
        u32 rd_band_width_mode             : 1;
        u32 reserve1                       : 7;
    } reg29;

    struct SWREG30_QOS_CTRL {
        u32 axi_wr_qos_level               : 4;
        u32 reserve0                       : 4;
        u32 axi_wr_qos                     : 4;
        u32 reserve1                       : 4;
        u32 axi_rd_qos_level               : 4;
        u32 reserve2                       : 4;
        u32 axi_rd_qos                     : 4;
        u32 reserve3                       : 4;
    } reg30;
};

struct vdpu383_regs_common_addr {
    /* SWREG128_STRM_BASE */
    u32 reg128_strm_base;

    /* SWREG129_RPS_BASE */
    u32 reg129_rps_base;

    /* SWREG130_CABACTBL_BASE */
    u32 reg130_cabactbl_base;

    /* SWREG131_GBL_BASE */
    u32 reg131_gbl_base;

    /* SWREG132_SCANLIST_ADDR */
    u32 reg132_scanlist_addr;

    /* SWREG133_SCL_BASE */
    u32 reg133_scale_down_base;

    /* SWREG134_FGS_BASE */
    u32 reg134_fgs_base;

    u32 reserve_reg135_139[5];

    struct rcb_info {
	u32 offset;
	u32 size;
    } rcb_info[11];
};

struct vdpu383_regs_h264_params {
    /* SWREG64_H26X_PARA */
    u32 reg64_start_decoder;

    /* SWREG65_STREAM_PARAM_SET */
    u32 reg65_strm_start_bit;

    /* SWREG66_STREAM_LEN */
    u32 reg66_stream_len;

    /* SWREG67_GLOBAL_LEN */
    u32 reg67_global_len;

    /* SWREG68_HOR_STRIDE */
    u32 reg68_hor_virstride;

    /* SWREG69_RASTER_UV_HOR_STRIDE */
    u32 reg69_raster_uv_hor_virstride;

    /* SWREG70_Y_STRIDE */
    u32 reg70_y_virstride;

    /* SWREG71_SCL_Y_HOR_VIRSTRIDE */
    u32 reg71_scl_ref_hor_virstride;

    /* SWREG72_SCL_UV_HOR_VIRSTRIDE */
    u32 reg72_scl_ref_raster_uv_hor_virstride;

    /* SWREG73_SCL_Y_VIRSTRIDE */
    u32 reg73_scl_ref_virstride;

    /* SWREG74_FGS_Y_HOR_VIRSTRIDE */
    u32 reg74_fgs_ref_hor_virstride;

    u32 reserve_reg75_79[5];

    /* SWREG80_ERROR_REF_Y_HOR_VIRSTRIDE */
    u32 reg80_error_ref_hor_virstride;

    /* SWREG81_ERROR_REF_UV_HOR_VIRSTRIDE */
    u32 reg81_error_ref_raster_uv_hor_virstride;

    /* SWREG82_ERROR_REF_Y_VIRSTRIDE */
    u32 reg82_error_ref_virstride;

    /* SWREG83_REF0_Y_HOR_VIRSTRIDE */
    u32 reg83_ref0_hor_virstride;

    /* SWREG84_REF0_UV_HOR_VIRSTRIDE */
    u32 reg84_ref0_raster_uv_hor_virstride;

    /* SWREG85_REF0_Y_VIRSTRIDE */
    u32 reg85_ref0_virstride;

    /* SWREG86_REF1_Y_HOR_VIRSTRIDE */
    u32 reg86_ref1_hor_virstride;

    /* SWREG87_REF1_UV_HOR_VIRSTRIDE */
    u32 reg87_ref1_raster_uv_hor_virstride;

    /* SWREG88_REF1_Y_VIRSTRIDE */
    u32 reg88_ref1_virstride;

    /* SWREG89_REF2_Y_HOR_VIRSTRIDE */
    u32 reg89_ref2_hor_virstride;

    /* SWREG90_REF2_UV_HOR_VIRSTRIDE */
    u32 reg90_ref2_raster_uv_hor_virstride;

    /* SWREG91_REF2_Y_VIRSTRIDE */
    u32 reg91_ref2_virstride;

    /* SWREG92_REF3_Y_HOR_VIRSTRIDE */
    u32 reg92_ref3_hor_virstride;

    /* SWREG93_REF3_UV_HOR_VIRSTRIDE */
    u32 reg93_ref3_raster_uv_hor_virstride;

    /* SWREG94_REF3_Y_VIRSTRIDE */
    u32 reg94_ref3_virstride;

    /* SWREG95_REF4_Y_HOR_VIRSTRIDE */
    u32 reg95_ref4_hor_virstride;

    /* SWREG96_REF4_UV_HOR_VIRSTRIDE */
    u32 reg96_ref4_raster_uv_hor_virstride;

    /* SWREG97_REF4_Y_VIRSTRIDE */
    u32 reg97_ref4_virstride;

    /* SWREG98_REF5_Y_HOR_VIRSTRIDE */
    u32 reg98_ref5_hor_virstride;

    /* SWREG99_REF5_UV_HOR_VIRSTRIDE */
    u32 reg99_ref5_raster_uv_hor_virstride;

    /* SWREG100_REF5_Y_VIRSTRIDE */
    u32 reg100_ref5_virstride;

    /* SWREG101_REF6_Y_HOR_VIRSTRIDE */
    u32 reg101_ref6_hor_virstride;

    /* SWREG102_REF6_UV_HOR_VIRSTRIDE */
    u32 reg102_ref6_raster_uv_hor_virstride;

    /* SWREG103_REF6_Y_VIRSTRIDE */
    u32 reg103_ref6_virstride;

    /* SWREG104_REF7_Y_HOR_VIRSTRIDE */
    u32 reg104_ref7_hor_virstride;

    /* SWREG105_REF7_UV_HOR_VIRSTRIDE */
    u32 reg105_ref7_raster_uv_hor_virstride;

    /* SWREG106_REF7_Y_VIRSTRIDE */
    u32 reg106_ref7_virstride;
};

struct vdpu383_regs_h264_addr {
    /* SWREG168_DECOUT_BASE */
    u32 reg168_decout_base;

    /* SWREG169_ERROR_REF_BASE */
    u32 reg169_error_ref_base;

    /* SWREG170_185_REF0_BASE */
    u32 reg170_185_ref_base[16];

    u32 reserve_reg186_191[6];

    /* SWREG192_PAYLOAD_ST_CUR_BASE */
    u32 reg192_payload_st_cur_base;

    /* SWREG193_FBC_PAYLOAD_OFFSET */
    u32 reg193_fbc_payload_offset;

    /* SWREG194_PAYLOAD_ST_ERROR_REF_BASE */
    u32 reg194_payload_st_error_ref_base;

    /* SWREG195_PAYLOAD_ST_REF0_BASE */
    u32 reg195_210_payload_st_ref_base[16];

    u32 reserve_reg211_215[5];

    /* SWREG216_COLMV_CUR_BASE */
    u32 reg216_colmv_cur_base;

    /* SWREG217_232_COLMV_REF0_BASE */
    u32 reg217_232_colmv_ref_base[16];
};
//struct rkvdec2_regs_h264_highpoc_rk3576	{};

struct vdpu383_regs_h264 {
	struct vdpu383_regs_common		common;		/* 8-30 */
	struct vdpu383_regs_h264_params		h264_param;	/* 64-74, 80-106 */
	struct vdpu383_regs_common_addr		common_addr;	/* 128-134, 140-161 */
	struct vdpu383_regs_h264_addr		h264_addr;	/* 168-185, 192-210, 216-232 */
//	struct vdpu383_regs_h264_highpoc_rk3576		h264_highpoc;
} __packed;

#endif /* __RKVDEC_VDPU838_REGS_H__ */
