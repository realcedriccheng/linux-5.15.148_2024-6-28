/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __BLK_NULL_BLK_H
#define __BLK_NULL_BLK_H

#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/blkdev.h>
#include <linux/slab.h>
#include <linux/blk-mq.h>
#include <linux/hrtimer.h>
#include <linux/configfs.h>
#include <linux/badblocks.h>
#include <linux/fault-inject.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>

// #define PREALLOC_SPACE
#define ZNS_FTL
#define CONV_FTL
// #define SLC_CACHE

#define USER_SET_LAT

#ifdef ZNS_FTL
#ifndef MUST_USE
#define MUST_USE
#endif
#endif

#ifdef CONV_FTL
#ifndef MUST_USE
#define MUST_USE
#endif
#endif

struct nullb_cmd {
	struct request *rq;
	struct bio *bio;
	unsigned int tag;
	blk_status_t error;
	struct nullb_queue *nq;
	struct hrtimer timer;
	bool fake_timeout;
	unsigned int queue_id;
#ifdef MUST_USE
	uint64_t nsecs_start;
	uint64_t nsecs_target;
#ifdef SLC_CACHE
	uint64_t nsecs_start_slc;
	int zno_map;
#endif
#endif	
};

struct nullb_queue {
	unsigned long *tag_map;
	wait_queue_head_t wait;
	unsigned int queue_depth;
	struct nullb_device *dev;
	unsigned int requeue_selection;
	unsigned int queue_id;//FG - BG

	struct nullb_cmd *cmds;
};

struct nullb_zone {
	/*
	 * Zone lock to prevent concurrent modification of a zone write
	 * pointer position and condition: with memory backing, a write
	 * command execution may sleep on memory allocation. For this case,
	 * use mutex as the zone lock. Otherwise, use the spinlock for
	 * locking the zone.
	 */
	union {
		spinlock_t spinlock;
		struct mutex mutex;
	};
	enum blk_zone_type type;
	enum blk_zone_cond cond;
	sector_t start;
	sector_t wp;
#ifdef SLC_CACHE
	sector_t wp_tlc;
#endif
	unsigned int len;
	unsigned int capacity;
};

struct nullb_device {
	struct nullb *nullb;
	struct config_item item;
	struct radix_tree_root data; /* data stored in the disk */
	struct radix_tree_root cache; /* disk cache data */
	unsigned long flags; /* device flags */
	unsigned int curr_cache;
	struct badblocks badblocks;

	unsigned int nr_zones;
	unsigned int nr_zones_imp_open;
	unsigned int nr_zones_exp_open;
	unsigned int nr_zones_closed;
	unsigned int imp_close_zone_no;
	struct nullb_zone *zones;
	sector_t zone_size_sects;
	bool need_zone_res_mgmt;
	spinlock_t zone_res_lock;

	unsigned long size; /* device size in MB */
	unsigned long completion_nsec; /* time in ns to complete a request */
#ifdef USER_SET_LAT
	unsigned long read_nsec; /* time in ns to complete a read NAND request */
	unsigned long write_nsec; /* time in ns to complete a write NAND request */
	unsigned long erase_nsec; /* time in ns to complete a erase NAND request */
#endif
	unsigned long cache_size; /* disk cache size in MB */
	unsigned long zone_size; /* zone size in MB if device is zoned */
	unsigned long zone_capacity; /* zone capacity in MB if device is zoned */
	unsigned int zone_nr_conv; /* number of conventional zones */
	unsigned int zone_max_open; /* max number of open zones */
	unsigned int zone_max_active; /* max number of active zones */
	unsigned int submit_queues; /* number of submission queues */
	unsigned int home_node; /* home node for the device */
	unsigned int queue_mode; /* block interface */
	unsigned int blocksize; /* block size */
	unsigned int max_sectors; /* Max sectors per command */
	unsigned int irqmode; /* IRQ completion handler */
	unsigned int hw_queue_depth; /* queue depth */
	unsigned int index; /* index of the disk, only valid with a disk */
	unsigned int mbps; /* Bandwidth throttle cap (in MB/s) */
	bool blocking; /* blocking blk-mq device */
	bool use_per_node_hctx; /* use per-node allocation for hardware context */
	bool power; /* power on/off the device */
	bool memory_backed; /* if data is stored in memory */
	bool discard; /* if support discard */
	bool zoned; /* if device is zoned */
	bool no_zone_write_lock;/* unlock FG - BG */
	bool virt_boundary; /* virtual boundary on/off for the device */

	bool zufs;	/* if device is zufs */
	bool ufs;	/* if device is ufs */
	unsigned long map_entry_nr_per_pg;
	unsigned long ufs_op_area_pcent;
#ifdef SLC_CACHE
	unsigned long slc_zone_number;
#endif

	unsigned long gc_calls;//for gc count
	unsigned long gc_move_pages;//for gc count

#ifdef ZNS_FTL
	struct zns_ftl *zns_ftl;
#endif
#ifdef CONV_FTL
	struct conv_ftl *conv_ftls;
#endif
#ifdef SLC_CACHE
	struct slc_cache *slc_cache;
#endif
};

struct nullb {
	struct nullb_device *dev;
	struct list_head list;
	unsigned int index;
	struct request_queue *q;
	struct gendisk *disk;
	struct blk_mq_tag_set *tag_set;
	struct blk_mq_tag_set __tag_set;
	unsigned int queue_depth;
	atomic_long_t cur_bytes;
	struct hrtimer bw_timer;
	unsigned long cache_flush_pos;
	spinlock_t lock;

	struct nullb_queue *queues;
	unsigned int nr_queues;
	char disk_name[DISK_NAME_LEN];
};

blk_status_t null_handle_discard(struct nullb_device *dev, sector_t sector,
				 sector_t nr_sectors);
blk_status_t null_process_cmd(struct nullb_cmd *cmd,
			      enum req_opf op, sector_t sector,
			      unsigned int nr_sectors);
#ifdef SLC_CACHE
void null_cmd_end_timer(struct nullb_cmd *cmd);
enum hrtimer_restart null_cmd_timer_expired_slc(struct hrtimer *timer);
inline void null_lock_zone(struct nullb_device *dev,
				  struct nullb_zone *zone);
inline void null_unlock_zone(struct nullb_device *dev,
				    struct nullb_zone *zone);
#endif

#ifdef CONFIG_BLK_DEV_ZONED
int null_init_zoned_dev(struct nullb_device *dev, struct request_queue *q);
int null_register_zoned_dev(struct nullb *nullb);
void null_free_zoned_dev(struct nullb_device *dev);
int null_report_zones(struct gendisk *disk, sector_t sector,
		      unsigned int nr_zones, report_zones_cb cb, void *data);
blk_status_t null_process_zoned_cmd(struct nullb_cmd *cmd,
				    enum req_opf op, sector_t sector,
				    sector_t nr_sectors);
size_t null_zone_valid_read_len(struct nullb *nullb,
				sector_t sector, unsigned int len);
#else
static inline int null_init_zoned_dev(struct nullb_device *dev,
				      struct request_queue *q)
{
	pr_err("CONFIG_BLK_DEV_ZONED not enabled\n");
	return -EINVAL;
}
static inline int null_register_zoned_dev(struct nullb *nullb)
{
	return -ENODEV;
}
static inline void null_free_zoned_dev(struct nullb_device *dev) {}
static inline blk_status_t null_process_zoned_cmd(struct nullb_cmd *cmd,
			enum req_opf op, sector_t sector, sector_t nr_sectors)
{
	return BLK_STS_NOTSUPP;
}
static inline size_t null_zone_valid_read_len(struct nullb *nullb,
					      sector_t sector,
					      unsigned int len)
{
	return len;
}
#define null_report_zones	NULL
#endif /* CONFIG_BLK_DEV_ZONED */
#endif /* __NULL_BLK_H */
