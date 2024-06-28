/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright 2021 Google LLC
 * Author: Daeho Jeong <daehojeong@google.com>
 */
#ifndef __F2FS_IOSTAT_H__
#define __F2FS_IOSTAT_H__

struct bio_post_read_ctx;

#ifdef CONFIG_F2FS_IOSTAT

#define DEFAULT_IOSTAT_PERIOD_MS	3000
#define MIN_IOSTAT_PERIOD_MS		100
/* maximum period of iostat tracing is 1 day */
#define MAX_IOSTAT_PERIOD_MS		8640000

enum {
	READ_IO,
	WRITE_SYNC_IO,
	WRITE_ASYNC_IO,
	MAX_IO_TYPE,
};

struct iostat_lat_info {
	unsigned long sum_lat[MAX_IO_TYPE][NR_PAGE_TYPE];	/* sum of io latencies */
	unsigned long peak_lat[MAX_IO_TYPE][NR_PAGE_TYPE];	/* peak io latency */
	unsigned int bio_cnt[MAX_IO_TYPE][NR_PAGE_TYPE];	/* bio count */
};

extern int __maybe_unused iostat_info_seq_show(struct seq_file *seq,
			void *offset);
extern void f2fs_reset_iostat(struct f2fs_sb_info *sbi);
extern void f2fs_update_iostat(struct f2fs_sb_info *sbi,
			enum iostat_type type, unsigned long long io_bytes);

struct bio_iostat_ctx {
	struct f2fs_sb_info *sbi;
	unsigned long submit_ts;
	enum page_type type;
	struct bio_post_read_ctx *post_read_ctx;
#ifdef CONFIG_BLK_DEV_ZONED
	struct bio_post_write_end_io_ctx *post_write_end_io_ctx;
#endif
};

static inline void iostat_update_submit_ctx(struct bio *bio,
			enum page_type type)
{
	struct bio_iostat_ctx *iostat_ctx = bio->bi_private;

	iostat_ctx->submit_ts = jiffies;
	iostat_ctx->type = type;
}

static inline struct bio_post_read_ctx *get_post_read_ctx(struct bio *bio)
{
	struct bio_iostat_ctx *iostat_ctx = bio->bi_private;

	return iostat_ctx->post_read_ctx;
}

extern void iostat_update_and_unbind_ctx(struct bio *bio, int rw);
#ifdef CONFIG_BLK_DEV_ZONED
extern void iostat_alloc_and_bind_ctx(struct f2fs_sb_info *sbi,
		struct bio *bio, struct bio_post_read_ctx *ctx, struct bio_post_write_end_io_ctx *w_ctx);
#else
extern void iostat_alloc_and_bind_ctx(struct f2fs_sb_info *sbi,
		struct bio *bio, struct bio_post_read_ctx *ctx);
#endif
extern int f2fs_init_iostat_processing(void);
extern void f2fs_destroy_iostat_processing(void);
extern int f2fs_init_iostat(struct f2fs_sb_info *sbi);
extern void f2fs_destroy_iostat(struct f2fs_sb_info *sbi);
#else
static inline void f2fs_update_iostat(struct f2fs_sb_info *sbi,
		enum iostat_type type, unsigned long long io_bytes) {}
static inline void iostat_update_and_unbind_ctx(struct bio *bio, int rw) {}
#ifdef CONFIG_BLK_DEV_ZONED
static inline void iostat_alloc_and_bind_ctx(struct f2fs_sb_info *sbi,
		struct bio *bio, struct bio_post_read_ctx *ctx, struct bio_post_write_end_io_ctx *w_ctx) {}
#else
static inline void iostat_alloc_and_bind_ctx(struct f2fs_sb_info *sbi,
		struct bio *bio, struct bio_post_read_ctx *ctx) {}
#endif
static inline void iostat_update_submit_ctx(struct bio *bio,
		enum page_type type) {}
static inline struct bio_post_read_ctx *get_post_read_ctx(struct bio *bio)
{
	return bio->bi_private;
}
static inline int f2fs_init_iostat_processing(void) { return 0; }
static inline void f2fs_destroy_iostat_processing(void) {}
static inline int f2fs_init_iostat(struct f2fs_sb_info *sbi) { return 0; }
static inline void f2fs_destroy_iostat(struct f2fs_sb_info *sbi) {}
#endif
#endif /* __F2FS_IOSTAT_H__ */
