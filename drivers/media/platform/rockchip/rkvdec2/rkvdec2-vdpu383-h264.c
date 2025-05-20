// SPDX-License-Identifier: GPL-2.0
/*
 * Rockchip Video Decoder VDPU383 H264 backend
 *
 * Copyright (C) 2024 Collabora, Ltd.
 *  Detlev Casanova <detlev.casanova@collabora.com>
 */

#include "linux/dev_printk.h"
#include <media/v4l2-h264.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-cabac/rkvdec-cabac.h>

#include <linux/iopoll.h>

#include "rkvdec2.h"
#include "rkvdec2-vdpu383-regs.h"

#define RKVDEC_NUM_REFLIST		3

struct rkvdec2_h264_scaling_list {
	u8 scaling_list_4x4[6][16];
	u8 scaling_list_8x8[6][64];
	u8 padding[128];
};

struct rkvdec2_sps {
	u16 seq_parameter_set_id:			4;
	u16 profile_idc:				8;
	u16 constraint_set3_flag:			1;
	u16 chroma_format_idc:				2;
	u16 bit_depth_luma:				3;
	u16 bit_depth_chroma:				3;
	u16 qpprime_y_zero_transform_bypass_flag:	1;
	u16 log2_max_frame_num_minus4:			4;
	u16 max_num_ref_frames:				5; //31
	u16 pic_order_cnt_type:				2;
	u16 log2_max_pic_order_cnt_lsb_minus4:		4;
	u16 delta_pic_order_always_zero_flag:		1; //38

	u16 pic_width_in_mbs:				16; //38 - 54
	u16 pic_height_in_mbs:				16; //54 - 70

	u16 frame_mbs_only_flag:			1; //1 1
	u16 mb_adaptive_frame_field_flag:		1; //1 0
	u16 direct_8x8_inference_flag:			1; //0 1
	u16 mvc_extension_enable:			1; //0 1
	u16 num_views:					2; //0 1
	u16 view_id0:                                   10; //0
	u16 view_id1:                                   10; //0
} __packed; // 96 bits

struct rkvdec2_pps {
	u32 pic_parameter_set_id:				8;
	u32 pps_seq_parameter_set_id:				5;
	u32 entropy_coding_mode_flag:				1;
	u32 bottom_field_pic_order_in_frame_present_flag:	1;
	u32 num_ref_idx_l0_default_active_minus1:		5;
	u32 num_ref_idx_l1_default_active_minus1:		5;
	u32 weighted_pred_flag:					1;
	u32 weighted_bipred_idc:				2;
	u32 pic_init_qp_minus26:				7;
	u32 pic_init_qs_minus26:				6;
	u32 chroma_qp_index_offset:				5;
	u32 deblocking_filter_control_present_flag:		1;
	u32 constrained_intra_pred_flag:			1;
	u32 redundant_pic_cnt_present:				1;
	u32 transform_8x8_mode_flag:				1;
	u32 second_chroma_qp_index_offset:			5;
	u32 scaling_list_enable_flag:				1;//56
	u32 is_longterm:					16;
	u32 voidx:						16;

	// dpb
	u32 pic_field_flag:                                     1; // 88 (184)
	u32 pic_associated_flag:                                1; // 89 (185)
	u32 cur_top_field:					32; // 90  - 122 (186 - 218)
	u32 cur_bot_field:					32; // 122 - 154 (218 - 250)

	u32 top_field_order_cnt0:				32;
	u32 bot_field_order_cnt0:				32;
	u32 top_field_order_cnt1:				32;
	u32 bot_field_order_cnt1:				32;
	u32 top_field_order_cnt2:				32;
	u32 bot_field_order_cnt2:				32;
	u32 top_field_order_cnt3:				32;
	u32 bot_field_order_cnt3:				32;
	u32 top_field_order_cnt4:				32;
	u32 bot_field_order_cnt4:				32;
	u32 top_field_order_cnt5:				32;
	u32 bot_field_order_cnt5:				32;
	u32 top_field_order_cnt6:				32;
	u32 bot_field_order_cnt6:				32;
	u32 top_field_order_cnt7:				32;
	u32 bot_field_order_cnt7:				32;
	u32 top_field_order_cnt8:				32;
	u32 bot_field_order_cnt8:				32;
	u32 top_field_order_cnt9:				32;
	u32 bot_field_order_cnt9:				32;
	u32 top_field_order_cnt10:				32;
	u32 bot_field_order_cnt10:				32;
	u32 top_field_order_cnt11:				32;
	u32 bot_field_order_cnt11:				32;
	u32 top_field_order_cnt12:				32;
	u32 bot_field_order_cnt12:				32;
	u32 top_field_order_cnt13:				32;
	u32 bot_field_order_cnt13:				32;
	u32 top_field_order_cnt14:				32;
	u32 bot_field_order_cnt14:				32;
	u32 top_field_order_cnt15:				32;
	u32 bot_field_order_cnt15:				32;

	u32 ref_field_flags:					16;
	u32 ref_topfield_used:					16;
	u32 ref_botfield_used:					16;
	u32 ref_colmv_use_flag:					16; //1186

	u32 :							30;
	u32 reserved[3];
} __packed;

struct rkvdec2_rps_entry {
	u32 dpb_info0:		5;
	u32 bottom_flag0:	1;
	u32 view_index_off0:	1;
	u32 dpb_info1:		5;
	u32 bottom_flag1:	1;
	u32 view_index_off1:	1;
	u32 dpb_info2:		5;
	u32 bottom_flag2:	1;
	u32 view_index_off2:	1;
	u32 dpb_info3:		5;
	u32 bottom_flag3:	1;
	u32 view_index_off3:	1;
	u32 dpb_info4:		5;
	u32 bottom_flag4:	1;
	u32 view_index_off4:	1;
	u32 dpb_info5:		5;
	u32 bottom_flag5:	1;
	u32 view_index_off5:	1;
	u32 dpb_info6:		5;
	u32 bottom_flag6:	1;
	u32 view_index_off6:	1;
	u32 dpb_info7:		5;
	u32 bottom_flag7:	1;
	u32 view_index_off7:	1;
} __packed;

struct rkvdec2_rps {
	u16 frame_num[16];
	u32 reserved0;
	struct rkvdec2_rps_entry entries[12];
	u32 reserved1[2];
} __packed;

struct rkvdec2_sps_pps {
	struct rkvdec2_sps sps;
	struct rkvdec2_pps pps;
} __packed;

/* Data structure describing auxiliary buffer format. */
struct rkvdec2_h264_priv_tbl {
	s8 cabac_table[4][464][2];
	struct rkvdec2_h264_scaling_list scaling_list;
	u8 reserved[32];
	struct rkvdec2_sps_pps param_set[256];
	struct rkvdec2_rps rps;
} __packed;

struct rkvdec2_h264_reflists {
	struct v4l2_h264_reference p[V4L2_H264_REF_LIST_LEN];
	struct v4l2_h264_reference b0[V4L2_H264_REF_LIST_LEN];
	struct v4l2_h264_reference b1[V4L2_H264_REF_LIST_LEN];
};

struct rkvdec2_h264_run {
	struct rkvdec2_run base;
	const struct v4l2_ctrl_h264_decode_params *decode_params;
	const struct v4l2_ctrl_h264_sps *sps;
	const struct v4l2_ctrl_h264_pps *pps;
	const struct v4l2_ctrl_h264_scaling_matrix *scaling_matrix;
	struct vb2_buffer *ref_buf[V4L2_H264_NUM_DPB_ENTRIES];
};

struct rkvdec2_h264_ctx {
	struct rkvdec2_aux_buf priv_tbl;
	struct rkvdec2_h264_reflists reflists;
	struct vdpu383_regs_h264 regs;
};

/*
static void dump_sps(struct rkvdec2_ctx *ctx, u32 *buf, size_t count)
{
	for (int i = 0; i < count; i++) {
		dev_warn(ctx->dev->dev, "SPS[%03d] %03x = 0x%08x\n", i, i*4, buf[i]);
	}
}

static void dump_rps(struct rkvdec2_ctx *ctx, u32 *buf, size_t count)
{
	for (int i = 0; i < count; i++) {
		dev_warn(ctx->dev->dev, "RPS[%03d] %03x = 0x%08x\n", i, i*4, buf[i]);
	}
}
*/

static void set_field_order_cnt(struct rkvdec2_sps_pps *hw_ps, int id, u32 top, u32 bottom)
{
	switch (id) {
	case 0:
		hw_ps->pps.top_field_order_cnt0 = top;
		hw_ps->pps.bot_field_order_cnt0 = bottom;
		break;
	case 1:
		hw_ps->pps.top_field_order_cnt1 = top;
		hw_ps->pps.bot_field_order_cnt1 = bottom;
		break;
	case 2:
		hw_ps->pps.top_field_order_cnt2 = top;
		hw_ps->pps.bot_field_order_cnt2 = bottom;
		break;
	case 3:
		hw_ps->pps.top_field_order_cnt3 = top;
		hw_ps->pps.bot_field_order_cnt3 = bottom;
		break;
	case 4:
		hw_ps->pps.top_field_order_cnt4 = top;
		hw_ps->pps.bot_field_order_cnt4 = bottom;
		break;
	case 5:
		hw_ps->pps.top_field_order_cnt5 = top;
		hw_ps->pps.bot_field_order_cnt5 = bottom;
		break;
	case 6:
		hw_ps->pps.top_field_order_cnt6 = top;
		hw_ps->pps.bot_field_order_cnt6 = bottom;
		break;
	case 7:
		hw_ps->pps.top_field_order_cnt7 = top;
		hw_ps->pps.bot_field_order_cnt7 = bottom;
		break;
	case 8:
		hw_ps->pps.top_field_order_cnt8 = top;
		hw_ps->pps.bot_field_order_cnt8 = bottom;
		break;
	case 9:
		hw_ps->pps.top_field_order_cnt9 = top;
		hw_ps->pps.bot_field_order_cnt9 = bottom;
		break;
	case 10:
		hw_ps->pps.top_field_order_cnt10 = top;
		hw_ps->pps.bot_field_order_cnt10 = bottom;
		break;
	case 11:
		hw_ps->pps.top_field_order_cnt11 = top;
		hw_ps->pps.bot_field_order_cnt11 = bottom;
		break;
	case 12:
		hw_ps->pps.top_field_order_cnt12 = top;
		hw_ps->pps.bot_field_order_cnt12 = bottom;
		break;
	case 13:
		hw_ps->pps.top_field_order_cnt13 = top;
		hw_ps->pps.bot_field_order_cnt13 = bottom;
		break;
	case 14:
		hw_ps->pps.top_field_order_cnt14 = top;
		hw_ps->pps.bot_field_order_cnt14 = bottom;
		break;
	case 15:
		hw_ps->pps.top_field_order_cnt15 = top;
		hw_ps->pps.bot_field_order_cnt15 = bottom;
		break;
	}
}

static void assemble_hw_pps(struct rkvdec2_ctx *ctx,
			    struct rkvdec2_h264_run *run)
{
	struct rkvdec2_h264_ctx *h264_ctx = ctx->priv;
	const struct v4l2_ctrl_h264_sps *sps = run->sps;
	const struct v4l2_ctrl_h264_pps *pps = run->pps;
	const struct v4l2_ctrl_h264_decode_params *dec_params = run->decode_params;
	const struct v4l2_h264_dpb_entry *dpb = dec_params->dpb;
	struct rkvdec2_h264_priv_tbl *priv_tbl = h264_ctx->priv_tbl.cpu;
	struct rkvdec2_sps_pps *hw_ps;
	u32 i;

	/*
	 * HW read the SPS/PPS information from PPS packet index by PPS id.
	 * offset from the base can be calculated by PPS_id * 32 (size per PPS
	 * packet unit). so the driver copy SPS/PPS information to the exact PPS
	 * packet unit for HW accessing.
	 */
	hw_ps = &priv_tbl->param_set[pps->pic_parameter_set_id];
	memset(hw_ps, 0, sizeof(*hw_ps));

	/* write sps */
	hw_ps->sps.seq_parameter_set_id = sps->seq_parameter_set_id;
	hw_ps->sps.profile_idc = sps->profile_idc;
	hw_ps->sps.constraint_set3_flag = !!(sps->constraint_set_flags & (1 << 3));
	hw_ps->sps.chroma_format_idc = sps->chroma_format_idc;
	hw_ps->sps.bit_depth_luma = sps->bit_depth_luma_minus8;
	hw_ps->sps.bit_depth_chroma = sps->bit_depth_chroma_minus8;
	hw_ps->sps.qpprime_y_zero_transform_bypass_flag =
		!!(sps->flags & V4L2_H264_SPS_FLAG_QPPRIME_Y_ZERO_TRANSFORM_BYPASS);
	hw_ps->sps.log2_max_frame_num_minus4 = sps->log2_max_frame_num_minus4;
	hw_ps->sps.max_num_ref_frames = sps->max_num_ref_frames;
	hw_ps->sps.pic_order_cnt_type = sps->pic_order_cnt_type;
	hw_ps->sps.log2_max_pic_order_cnt_lsb_minus4 =
		sps->log2_max_pic_order_cnt_lsb_minus4;
	hw_ps->sps.delta_pic_order_always_zero_flag =
		!!(sps->flags & V4L2_H264_SPS_FLAG_DELTA_PIC_ORDER_ALWAYS_ZERO);
	hw_ps->sps.mvc_extension_enable = 0;
	hw_ps->sps.num_views = 0;

	/*
	 * Use the SPS values since they are already in macroblocks
	 * dimensions, height can be field height (halved) if
	 * V4L2_H264_SPS_FLAG_FRAME_MBS_ONLY is not set and also it allows
	 * decoding smaller images into larger allocation which can be used
	 * to implementing SVC spatial layer support.
	 */
	u32 pic_width = 16 * (sps->pic_width_in_mbs_minus1 + 1);
	u32 pic_height = 16 * (sps->pic_height_in_map_units_minus1 + 1);
	if (!(sps->flags & V4L2_H264_SPS_FLAG_FRAME_MBS_ONLY))
		pic_height *= 2;
	if (!!(dec_params->flags & V4L2_H264_DECODE_PARAM_FLAG_FIELD_PIC))
		pic_height /= 2;

	hw_ps->sps.pic_width_in_mbs = pic_width;
	hw_ps->sps.pic_height_in_mbs = pic_height;

	hw_ps->sps.frame_mbs_only_flag =
		!!(sps->flags & V4L2_H264_SPS_FLAG_FRAME_MBS_ONLY);
	hw_ps->sps.mb_adaptive_frame_field_flag =
		!!(sps->flags & V4L2_H264_SPS_FLAG_MB_ADAPTIVE_FRAME_FIELD);
	hw_ps->sps.direct_8x8_inference_flag =
		!!(sps->flags & V4L2_H264_SPS_FLAG_DIRECT_8X8_INFERENCE);

	/* write pps */
	hw_ps->pps.pic_parameter_set_id = pps->pic_parameter_set_id;
	hw_ps->pps.pps_seq_parameter_set_id = pps->seq_parameter_set_id;
	hw_ps->pps.entropy_coding_mode_flag =
		!!(pps->flags & V4L2_H264_PPS_FLAG_ENTROPY_CODING_MODE);
	hw_ps->pps.bottom_field_pic_order_in_frame_present_flag =
		!!(pps->flags & V4L2_H264_PPS_FLAG_BOTTOM_FIELD_PIC_ORDER_IN_FRAME_PRESENT);
	hw_ps->pps.num_ref_idx_l0_default_active_minus1 =
		pps->num_ref_idx_l0_default_active_minus1;
	hw_ps->pps.num_ref_idx_l1_default_active_minus1 =
		pps->num_ref_idx_l1_default_active_minus1;
	hw_ps->pps.weighted_pred_flag =
		!!(pps->flags & V4L2_H264_PPS_FLAG_WEIGHTED_PRED);
	hw_ps->pps.weighted_bipred_idc = pps->weighted_bipred_idc;
	hw_ps->pps.pic_init_qp_minus26 = pps->pic_init_qp_minus26;
	hw_ps->pps.pic_init_qs_minus26 = pps->pic_init_qs_minus26;
	hw_ps->pps.chroma_qp_index_offset = pps->chroma_qp_index_offset;
	hw_ps->pps.deblocking_filter_control_present_flag =
		!!(pps->flags & V4L2_H264_PPS_FLAG_DEBLOCKING_FILTER_CONTROL_PRESENT);
	hw_ps->pps.constrained_intra_pred_flag =
		!!(pps->flags & V4L2_H264_PPS_FLAG_CONSTRAINED_INTRA_PRED);
	hw_ps->pps.redundant_pic_cnt_present =
		!!(pps->flags & V4L2_H264_PPS_FLAG_REDUNDANT_PIC_CNT_PRESENT);
	hw_ps->pps.transform_8x8_mode_flag =
		!!(pps->flags & V4L2_H264_PPS_FLAG_TRANSFORM_8X8_MODE);
	hw_ps->pps.second_chroma_qp_index_offset = pps->second_chroma_qp_index_offset;
	hw_ps->pps.scaling_list_enable_flag =
		!!(pps->flags & V4L2_H264_PPS_FLAG_SCALING_MATRIX_PRESENT);

	for (i = 0; i < ARRAY_SIZE(dec_params->dpb); i++) {
		if (dpb[i].flags & V4L2_H264_DPB_ENTRY_FLAG_LONG_TERM)
			hw_ps->pps.is_longterm |= (1 << i);
		set_field_order_cnt(hw_ps, i, dpb[i].top_field_order_cnt, dpb[i].bottom_field_order_cnt);
		hw_ps->pps.ref_field_flags |=
			(!!(dpb[i].flags & V4L2_H264_DPB_ENTRY_FLAG_FIELD)) << i;
		hw_ps->pps.ref_colmv_use_flag |=
			(!!(dpb[i].flags & V4L2_H264_DPB_ENTRY_FLAG_ACTIVE)) << i;
		hw_ps->pps.ref_topfield_used |=
			(!!(dpb[i].fields & V4L2_H264_TOP_FIELD_REF)) << i;
		hw_ps->pps.ref_botfield_used |=
			(!!(dpb[i].fields & V4L2_H264_BOTTOM_FIELD_REF)) << i;
	}

	hw_ps->pps.pic_field_flag =
		!!(dec_params->flags & V4L2_H264_DECODE_PARAM_FLAG_FIELD_PIC);
	hw_ps->pps.pic_associated_flag =
		!!(dec_params->flags & V4L2_H264_DECODE_PARAM_FLAG_BOTTOM_FIELD);

	// MPP only seems to set this when hw_ps->pps.pic_associated_flag is one.
	// It doesn't seem to be needed, so I don't do it here and reg values are not identical
	// with mpp.
	hw_ps->pps.cur_top_field = dec_params->top_field_order_cnt;
	hw_ps->pps.cur_bot_field = dec_params->bottom_field_order_cnt;

	//dump_sps(ctx, (u32*)hw_ps, sizeof(*hw_ps)/4);
}

static void lookup_ref_buf_idx(struct rkvdec2_ctx *ctx,
			       struct rkvdec2_h264_run *run)
{
	const struct v4l2_ctrl_h264_decode_params *dec_params = run->decode_params;
	u32 i;

	for (i = 0; i < ARRAY_SIZE(dec_params->dpb); i++) {
		struct v4l2_m2m_ctx *m2m_ctx = ctx->fh.m2m_ctx;
		const struct v4l2_h264_dpb_entry *dpb = run->decode_params->dpb;
		struct vb2_queue *cap_q = &m2m_ctx->cap_q_ctx.q;
		struct vb2_buffer *buf = NULL;

		if (dpb[i].flags & V4L2_H264_DPB_ENTRY_FLAG_ACTIVE) {
			buf = vb2_find_buffer(cap_q, dpb[i].reference_ts);
			if (!buf) {
				dev_dbg(ctx->dev->dev, "No buffer for reference_ts %llu",
					dpb[i].reference_ts);
			}
		}

		run->ref_buf[i] = buf;
	}
}

static void set_dpb_info(struct rkvdec2_rps_entry *entries,
			 u8 reflist,
			 u8 refnum,
			 u8 info,
			 bool bottom)
{
	struct rkvdec2_rps_entry *entry = &entries[(reflist * 4) + refnum / 8];
	u8 idx = refnum % 8;

	switch (idx) {
	case 0:
		entry->dpb_info0 = info;
		entry->bottom_flag0 = bottom;
		break;
	case 1:
		entry->dpb_info1 = info;
		entry->bottom_flag1 = bottom;
		break;
	case 2:
		entry->dpb_info2 = info;
		entry->bottom_flag2 = bottom;
		break;
	case 3:
		entry->dpb_info3 = info;
		entry->bottom_flag3 = bottom;
		break;
	case 4:
		entry->dpb_info4 = info;
		entry->bottom_flag4 = bottom;
		break;
	case 5:
		entry->dpb_info5 = info;
		entry->bottom_flag5 = bottom;
		break;
	case 6:
		entry->dpb_info6 = info;
		entry->bottom_flag6 = bottom;
		break;
	case 7:
		entry->dpb_info7 = info;
		entry->bottom_flag7 = bottom;
		break;
	}
}

static void assemble_hw_rps(struct rkvdec2_ctx *ctx,
			    struct v4l2_h264_reflist_builder *builder,
			    struct rkvdec2_h264_run *run)
{
	const struct v4l2_ctrl_h264_decode_params *dec_params = run->decode_params;
	const struct v4l2_h264_dpb_entry *dpb = dec_params->dpb;
	struct rkvdec2_h264_ctx *h264_ctx = ctx->priv;
	struct rkvdec2_h264_priv_tbl *priv_tbl = h264_ctx->priv_tbl.cpu;

	struct rkvdec2_rps *hw_rps = &priv_tbl->rps;
	u32 i, j;

	memset(hw_rps, 0, sizeof(priv_tbl->rps));

	/*
	 * Assign an invalid pic_num if DPB entry at that position is inactive.
	 * If we assign 0 in that position hardware will treat that as a real
	 * reference picture with pic_num 0, triggering output picture
	 * corruption.
	 */
	for (i = 0; i < ARRAY_SIZE(dec_params->dpb); i++) {
		if (!(dpb[i].flags & V4L2_H264_DPB_ENTRY_FLAG_ACTIVE))
			continue;

		hw_rps->frame_num[i] = builder->refs[i].frame_num;
	}

	for (j = 0; j < RKVDEC_NUM_REFLIST; j++) {
		for (i = 0; i < builder->num_valid; i++) {
			struct v4l2_h264_reference *ref;
			bool dpb_valid;
			bool bottom;

			switch (j) {
			case 0:
				ref = &h264_ctx->reflists.p[i];
				break;
			case 1:
				ref = &h264_ctx->reflists.b0[i];
				break;
			case 2:
				ref = &h264_ctx->reflists.b1[i];
				break;
			}

			if (WARN_ON(ref->index >= ARRAY_SIZE(dec_params->dpb)))
				continue;

			dpb_valid = !!(run->ref_buf[ref->index]);
			bottom = ref->fields == V4L2_H264_BOTTOM_FIELD_REF;

			set_dpb_info(hw_rps->entries, j, i, ref->index | (dpb_valid << 4), bottom);
		}
	}
	//dump_rps(ctx, (u32*)hw_rps, sizeof(*hw_rps)/4);
}

static void assemble_hw_scaling_list(struct rkvdec2_ctx *ctx,
				     struct rkvdec2_h264_run *run)
{
	const struct v4l2_ctrl_h264_scaling_matrix *scaling = run->scaling_matrix;
	const struct v4l2_ctrl_h264_pps *pps = run->pps;
	struct rkvdec2_h264_ctx *h264_ctx = ctx->priv;
	struct rkvdec2_h264_priv_tbl *tbl = h264_ctx->priv_tbl.cpu;

	if (!(pps->flags & V4L2_H264_PPS_FLAG_SCALING_MATRIX_PRESENT))
		return;

	BUILD_BUG_ON(sizeof(tbl->scaling_list.scaling_list_4x4) !=
		     sizeof(scaling->scaling_list_4x4));
	BUILD_BUG_ON(sizeof(tbl->scaling_list.scaling_list_8x8) !=
		     sizeof(scaling->scaling_list_8x8));

	memcpy(tbl->scaling_list.scaling_list_4x4,
	       scaling->scaling_list_4x4,
	       sizeof(scaling->scaling_list_4x4));

	memcpy(tbl->scaling_list.scaling_list_8x8,
	       scaling->scaling_list_8x8,
	       sizeof(scaling->scaling_list_8x8));
}

static void dump_regs(struct rkvdec2_ctx *ctx, u32 *buf, size_t count, u32 offset)
{
	//for (int i = 0; i < count; i++) {
	//	dev_warn(ctx->dev->dev, "reg[%03d] %03x = 0x%08x\n", i + offset/4, offset + i*4, buf[i]);
	//}
}


static inline void rkvdec2_memcpy_toio(void __iomem *dst, void *src, size_t len)
{
#ifdef CONFIG_ARM64
	__iowrite32_copy(dst, src, len/4);
#else
	memcpy_toio(dst, src, len);
#endif
}

static void rkvdec2_write_regs(struct rkvdec2_ctx *ctx)
{
	struct rkvdec2_dev *rkvdec = ctx->dev;
	struct rkvdec2_h264_ctx *h264_ctx = ctx->priv;

	rkvdec2_memcpy_toio(rkvdec->regs + VDPU383_OFFSET_COMMON_REGS,
			    &h264_ctx->regs.common,
			    sizeof(h264_ctx->regs.common));
	dump_regs(ctx, (u32*)&h264_ctx->regs.common, sizeof(h264_ctx->regs.common)/4, VDPU383_OFFSET_COMMON_REGS);
	rkvdec2_memcpy_toio(rkvdec->regs + VDPU383_OFFSET_COMMON_ADDR_REGS,
			    &h264_ctx->regs.common_addr,
			    sizeof(h264_ctx->regs.common_addr));
	dump_regs(ctx, (u32*)&h264_ctx->regs.common_addr, sizeof(h264_ctx->regs.common_addr)/4, VDPU383_OFFSET_COMMON_ADDR_REGS);
	rkvdec2_memcpy_toio(rkvdec->regs + VDPU383_OFFSET_CODEC_PARAMS_REGS,
			    &h264_ctx->regs.h264_param,
			    sizeof(h264_ctx->regs.h264_param));
	dump_regs(ctx, (u32*)&h264_ctx->regs.h264_param, sizeof(h264_ctx->regs.h264_param)/4, VDPU383_OFFSET_CODEC_PARAMS_REGS);
	rkvdec2_memcpy_toio(rkvdec->regs + VDPU383_OFFSET_CODEC_ADDR_REGS,
			    &h264_ctx->regs.h264_addr,
			    sizeof(h264_ctx->regs.h264_addr));
	dump_regs(ctx, (u32*)&h264_ctx->regs.h264_addr, sizeof(h264_ctx->regs.h264_addr)/4, VDPU383_OFFSET_CODEC_ADDR_REGS);
//	rkvdec2_memcpy_toio(rkvdec->regs + VDPU383_OFFSET_POC_HIGHBIT_REGS,
//			    &h264_ctx->regs.h264_highpoc,
//			    sizeof(h264_ctx->regs.h264_highpoc));
}

static void config_registers(struct rkvdec2_ctx *ctx,
			     struct rkvdec2_h264_run *run)
{
	const struct v4l2_ctrl_h264_decode_params *dec_params = run->decode_params;
	struct rkvdec2_h264_ctx *h264_ctx = ctx->priv;
	dma_addr_t priv_start_addr = h264_ctx->priv_tbl.dma;
	const struct v4l2_pix_format_mplane *dst_fmt;
	struct vb2_v4l2_buffer *src_buf = run->base.bufs.src;
	struct vb2_v4l2_buffer *dst_buf = run->base.bufs.dst;
	struct vdpu383_regs_h264 *regs = &h264_ctx->regs;
	const struct v4l2_format *f;
	dma_addr_t rlc_addr;
	dma_addr_t dst_addr;
	u32 hor_virstride;
	u32 ver_virstride;
	u32 y_virstride;
	u32 offset;
	u32 pixels;
	u32 i;

	memset(regs, 0, sizeof(*regs));

	/* Set H264 mode */
	regs->common.reg8_dec_mode = RKVDEC2_MODE_H264;

	/* Set input stream length */
	regs->h264_param.reg66_stream_len = vb2_get_plane_payload(&src_buf->vb2_buf, 0);

	/* Set strides */
	f = &ctx->decoded_fmt;
	dst_fmt = &f->fmt.pix_mp;
	hor_virstride = dst_fmt->plane_fmt[0].bytesperline;
	ver_virstride = dst_fmt->height;
	y_virstride = hor_virstride * ver_virstride;

	pixels = dst_fmt->height * dst_fmt->width;

	regs->h264_param.reg68_hor_virstride = hor_virstride / 16;
	regs->h264_param.reg69_raster_uv_hor_virstride = hor_virstride / 16;
	regs->h264_param.reg70_y_virstride = y_virstride / 16;

	/* Activate block gating */
	regs->common.reg10.strmd_auto_gating_e      = 1;
	regs->common.reg10.inter_auto_gating_e      = 1;
	regs->common.reg10.intra_auto_gating_e      = 1;
	regs->common.reg10.transd_auto_gating_e     = 1;
	regs->common.reg10.recon_auto_gating_e      = 1;
	regs->common.reg10.filterd_auto_gating_e    = 1;
	regs->common.reg10.bus_auto_gating_e        = 1;
	regs->common.reg10.ctrl_auto_gating_e       = 1;
	regs->common.reg10.rcb_auto_gating_e        = 1;
	regs->common.reg10.err_prc_auto_gating_e    = 1;

	/* Set timeout threshold */
	if (pixels < RKVDEC2_1080P_PIXELS)
		regs->common.reg13_core_timeout_threshold = RKVDEC2_TIMEOUT_1080p;
	else if (pixels < RKVDEC2_4K_PIXELS)
		regs->common.reg13_core_timeout_threshold = RKVDEC2_TIMEOUT_4K;
	else if (pixels < RKVDEC2_8K_PIXELS)
		regs->common.reg13_core_timeout_threshold = RKVDEC2_TIMEOUT_8K;
	else
		regs->common.reg13_core_timeout_threshold = RKVDEC2_TIMEOUT_MAX;

	regs->common.reg16.error_proc_disable = 1;

	/* Set ref pic address & poc */
	for (i = 0; i < ARRAY_SIZE(dec_params->dpb); i++) {
		struct vb2_buffer *vb_buf = run->ref_buf[i];
		dma_addr_t buf_dma;

		/*
		 * If a DPB entry is unused or invalid, address of current destination
		 * buffer is returned.
		 */
		if (!vb_buf)
			vb_buf = &dst_buf->vb2_buf;

		buf_dma = vb2_dma_contig_plane_dma_addr(vb_buf, 0);

		/* Set reference addresses */
		regs->h264_addr.reg170_185_ref_base[i] = buf_dma;
		regs->h264_addr.reg195_210_payload_st_ref_base[i] = buf_dma;

		/* Set COLMV addresses */
		regs->h264_addr.reg217_232_colmv_ref_base[i] = buf_dma + ctx->colmv_offset;
	}

	/* Set rlc base address (input stream) */
	rlc_addr = vb2_dma_contig_plane_dma_addr(&src_buf->vb2_buf, 0);
	regs->common_addr.reg128_strm_base = rlc_addr;

	/* Set output base address */
	dst_addr = vb2_dma_contig_plane_dma_addr(&dst_buf->vb2_buf, 0);
	regs->h264_addr.reg168_decout_base = dst_addr;
	regs->h264_addr.reg169_error_ref_base = dst_addr;
	regs->h264_addr.reg192_payload_st_cur_base = dst_addr; // FIXME: This is probably not correct.

	/* Set colmv address */
	regs->h264_addr.reg216_colmv_cur_base = dst_addr + ctx->colmv_offset;

	/* Set RCB addresses */
	for (i = 0; i < RKVDEC2_RCB_COUNT; i++) {
		regs->common_addr.rcb_info[i].offset = ctx->rcb_bufs[i].dma;
		regs->common_addr.rcb_info[i].size = ctx->rcb_bufs[i].size;
	}

	/* Set hw pps address */
	offset = offsetof(struct rkvdec2_h264_priv_tbl, param_set);
	regs->common_addr.reg131_gbl_base = priv_start_addr + offset;
	regs->h264_param.reg67_global_len = sizeof(struct rkvdec2_sps_pps) / 16;

	/* Set hw rps address */
	offset = offsetof(struct rkvdec2_h264_priv_tbl, rps);
	regs->common_addr.reg129_rps_base = priv_start_addr + offset;

	/* Set cabac table */
	offset = offsetof(struct rkvdec2_h264_priv_tbl, cabac_table);
	regs->common_addr.reg130_cabactbl_base = priv_start_addr + offset;

	rkvdec2_write_regs(ctx);
}

#define RKVDEC_H264_MAX_DEPTH_IN_BYTES		2

static int rkvdec2_h264_adjust_fmt(struct rkvdec2_ctx *ctx,
				   struct v4l2_format *f)
{
	struct v4l2_pix_format_mplane *fmt = &f->fmt.pix_mp;

	fmt->num_planes = 1;
	if (!fmt->plane_fmt[0].sizeimage)
		fmt->plane_fmt[0].sizeimage = fmt->width * fmt->height *
					      RKVDEC_H264_MAX_DEPTH_IN_BYTES;
	return 0;
}

static enum rkvdec2_image_fmt rkvdec2_h264_get_image_fmt(struct rkvdec2_ctx *ctx,
							 struct v4l2_ctrl *ctrl)
{
	const struct v4l2_ctrl_h264_sps *sps = ctrl->p_new.p_h264_sps;

	if (ctrl->id != V4L2_CID_STATELESS_H264_SPS)
		return RKVDEC2_IMG_FMT_ANY;

	if (sps->bit_depth_luma_minus8 == 0) {
		if (sps->chroma_format_idc == 2)
			return RKVDEC2_IMG_FMT_422_8BIT;
		else
			return RKVDEC2_IMG_FMT_420_8BIT;
	} else if (sps->bit_depth_luma_minus8 == 2) {
		if (sps->chroma_format_idc == 2)
			return RKVDEC2_IMG_FMT_422_10BIT;
		else
			return RKVDEC2_IMG_FMT_420_10BIT;
	}

	return RKVDEC2_IMG_FMT_ANY;
}

static int rkvdec2_h264_validate_sps(struct rkvdec2_ctx *ctx,
				     const struct v4l2_ctrl_h264_sps *sps)
{
	unsigned int width, height;

	/* Only 4:0:0, 4:2:0 and 4:2:2 are supported */
	if (sps->chroma_format_idc > 2)
		return -EINVAL;

	/* Luma and chroma bit depth mismatch */
	if (sps->bit_depth_luma_minus8 != sps->bit_depth_chroma_minus8)
		return -EINVAL;

	/* Only 8-bit and 10-bit are supported */
	if (sps->bit_depth_luma_minus8 != 0 && sps->bit_depth_luma_minus8 != 2)
		return -EINVAL;

	width = (sps->pic_width_in_mbs_minus1 + 1) * 16;
	height = (sps->pic_height_in_map_units_minus1 + 1) * 16;

	/*
	 * When frame_mbs_only_flag is not set, this is field height,
	 * which is half the final height (see (7-8) in the
	 * specification)
	 */
	if (!(sps->flags & V4L2_H264_SPS_FLAG_FRAME_MBS_ONLY))
		height *= 2;

	if (width > ctx->coded_fmt.fmt.pix_mp.width ||
	    height > ctx->coded_fmt.fmt.pix_mp.height)
		return -EINVAL;

	return 0;
}

static int rkvdec2_h264_start(struct rkvdec2_ctx *ctx)
{
	struct rkvdec2_dev *rkvdec = ctx->dev;
	struct rkvdec2_h264_priv_tbl *priv_tbl;
	struct rkvdec2_h264_ctx *h264_ctx;
	struct v4l2_ctrl *ctrl;
	int ret;

	ctrl = v4l2_ctrl_find(&ctx->ctrl_hdl,
			      V4L2_CID_STATELESS_H264_SPS);
	if (!ctrl)
		return -EINVAL;

	ret = rkvdec2_h264_validate_sps(ctx, ctrl->p_new.p_h264_sps);
	if (ret)
		return ret;

	h264_ctx = kzalloc(sizeof(*h264_ctx), GFP_KERNEL);
	if (!h264_ctx)
		return -ENOMEM;

	priv_tbl = dma_alloc_coherent(rkvdec->dev, sizeof(*priv_tbl),
				      &h264_ctx->priv_tbl.dma, GFP_KERNEL);
	if (!priv_tbl) {
		ret = -ENOMEM;
		goto err_free_ctx;
	}

	h264_ctx->priv_tbl.size = sizeof(*priv_tbl);
	h264_ctx->priv_tbl.cpu = priv_tbl;
	memcpy(priv_tbl->cabac_table, rkvdec_h264_cabac_table,
	       sizeof(rkvdec_h264_cabac_table));

	ctx->priv = h264_ctx;

	return 0;

err_free_ctx:
	kfree(h264_ctx);
	return ret;
}

static void rkvdec2_h264_stop(struct rkvdec2_ctx *ctx)
{
	struct rkvdec2_h264_ctx *h264_ctx = ctx->priv;
	struct rkvdec2_dev *rkvdec = ctx->dev;

	dma_free_coherent(rkvdec->dev, h264_ctx->priv_tbl.size,
			  h264_ctx->priv_tbl.cpu, h264_ctx->priv_tbl.dma);
	kfree(h264_ctx);
}

static void rkvdec2_h264_run_preamble(struct rkvdec2_ctx *ctx,
				      struct rkvdec2_h264_run *run)
{
	struct v4l2_ctrl *ctrl;

	ctrl = v4l2_ctrl_find(&ctx->ctrl_hdl,
			      V4L2_CID_STATELESS_H264_DECODE_PARAMS);
	run->decode_params = ctrl ? ctrl->p_cur.p : NULL;
	ctrl = v4l2_ctrl_find(&ctx->ctrl_hdl,
			      V4L2_CID_STATELESS_H264_SPS);
	run->sps = ctrl ? ctrl->p_cur.p : NULL;
	ctrl = v4l2_ctrl_find(&ctx->ctrl_hdl,
			      V4L2_CID_STATELESS_H264_PPS);
	run->pps = ctrl ? ctrl->p_cur.p : NULL;
	ctrl = v4l2_ctrl_find(&ctx->ctrl_hdl,
			      V4L2_CID_STATELESS_H264_SCALING_MATRIX);
	run->scaling_matrix = ctrl ? ctrl->p_cur.p : NULL;

	rkvdec2_run_preamble(ctx, &run->base);
}

static int rkvdec2_h264_run(struct rkvdec2_ctx *ctx)
{
	struct v4l2_h264_reflist_builder reflist_builder;
	struct rkvdec2_dev *rkvdec = ctx->dev;
	struct rkvdec2_h264_ctx *h264_ctx = ctx->priv;
	struct rkvdec2_h264_run run;
	uint32_t watchdog_time;

	rkvdec2_h264_run_preamble(ctx, &run);

	/* Build the P/B{0,1} ref lists. */
	v4l2_h264_init_reflist_builder(&reflist_builder, run.decode_params,
				       run.sps, run.decode_params->dpb);
	v4l2_h264_build_p_ref_list(&reflist_builder, h264_ctx->reflists.p);
	v4l2_h264_build_b_ref_lists(&reflist_builder, h264_ctx->reflists.b0,
				    h264_ctx->reflists.b1);

	assemble_hw_scaling_list(ctx, &run);
	assemble_hw_pps(ctx, &run);
	lookup_ref_buf_idx(ctx, &run);
	assemble_hw_rps(ctx, &reflist_builder, &run);

	config_registers(ctx, &run);

	rkvdec2_run_postamble(ctx, &run.base);

	/* Set watchdog at 2 times the hardware timeout threshold */
	u64 timeout_threshold = h264_ctx->regs.common.reg13_core_timeout_threshold;
	unsigned long axi_rate = clk_get_rate(rkvdec->axi_clk);

	if (axi_rate)
		watchdog_time = 2 * (1000 * timeout_threshold) / axi_rate;
	else
		watchdog_time = 2000;
	schedule_delayed_work(&rkvdec->watchdog_work,
			      msecs_to_jiffies(watchdog_time));


	/* Start decoding! */
	writel(0x007fffff, rkvdec->link + 0x54);
	writel(0x00000000, rkvdec->link + 0x58);
	writel(0x00000001, rkvdec->link + 0x40);

	return 0;
}

static int rkvdec2_h264_try_ctrl(struct rkvdec2_ctx *ctx, struct v4l2_ctrl *ctrl)
{
	if (ctrl->id == V4L2_CID_STATELESS_H264_SPS)
		return rkvdec2_h264_validate_sps(ctx, ctrl->p_new.p_h264_sps);

	return 0;
}

const struct rkvdec2_coded_fmt_ops rkvdec2_vdpu383_h264_fmt_ops = {
	.adjust_fmt = rkvdec2_h264_adjust_fmt,
	.get_image_fmt = rkvdec2_h264_get_image_fmt,
	.start = rkvdec2_h264_start,
	.stop = rkvdec2_h264_stop,
	.run = rkvdec2_h264_run,
	.try_ctrl = rkvdec2_h264_try_ctrl,
};
