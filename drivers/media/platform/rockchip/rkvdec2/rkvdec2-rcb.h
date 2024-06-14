#include <linux/types.h>

struct rkvdec2_ctx;

enum rcb_axis {
	PIC_WIDTH = 0,
	PIC_HEIGHT = 1
};

struct rcb_size_info {
	u8 multiplier;
	enum rcb_axis axis;
};


int rkvdec_allocate_rcb(struct rkvdec2_ctx *ctx, const struct rcb_size_info *size_info, size_t rcb_count);
dma_addr_t rkvdec_rcb_buf_dma_addr(struct rkvdec2_ctx *ctx, int id);
size_t rkvdec_rcb_buf_size(struct rkvdec2_ctx *ctx, int id);
int rkvdec_rcb_buf_count(struct rkvdec2_ctx *ctx);
void rkvdec_free_rcb(struct rkvdec2_ctx *ctx);
