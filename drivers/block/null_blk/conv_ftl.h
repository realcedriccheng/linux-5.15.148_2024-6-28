// SPDX-License-Identifier: GPL-2.0-only

#ifndef _NVMEVIRT_CONV_FTL_H
#define _NVMEVIRT_CONV_FTL_H

#include <linux/types.h>
#include "pqueue.h"
#include "ssd_config.h"
#include "ssd.h"
#include "null_blk.h"

#define MAP_ENTRY_LRU

struct convparams {
	uint32_t gc_thres_lines;
	uint32_t gc_thres_lines_high;
	bool enable_gc_delay;

	double op_area_pcent;
	int pba_pcent; /* (physical space / logical space) * 100*/
};

struct line {
	int id; /* line id, the same as corresponding block id */
	int ipc; /* invalid page count in this line */
	int vpc; /* valid page count in this line */
	struct list_head entry;
	/* position in the priority queue for victim lines */
	size_t pos;
};

/* wp: record next write addr */
struct write_pointer {
	struct line *curline;
	uint32_t ch;
	uint32_t lun;
	uint32_t pg;
	uint32_t blk;
	uint32_t pl;
};

struct line_mgmt {
	struct line *lines;

	/* free line list, we only need to maintain a list of blk numbers */
	struct list_head free_line_list;
	pqueue_t *victim_line_pq;
	struct list_head full_line_list;

	uint32_t tt_lines;
	uint32_t free_line_cnt;
	uint32_t victim_line_cnt;
	uint32_t full_line_cnt;
};

struct write_flow_control {
	uint32_t write_credits;
	uint32_t credits_to_refill;
};

#ifdef MAP_ENTRY_LRU

#define CACHE_SIZE 64

struct LRU_node {
	uint64_t lpn;
	struct LRU_node *prev;
	struct LRU_node *next;
};

struct LRU_cache {
	int size;
	int capacity;
	struct LRU_node *head;
	struct LRU_node *tail;
};

#endif

struct conv_ftl {
	struct ssd *ssd;

	struct convparams cp;
	struct ppa *maptbl; /* page level mapping table */
	uint64_t *rmap; /* reverse mapptbl, assume it's stored in OOB */
	struct write_pointer wp;
	struct write_pointer gc_wp;
	struct line_mgmt lm;
	struct write_flow_control wfc;
	struct mutex write_lock;

#ifdef MAP_ENTRY_LRU
	struct LRU_node **maptbl_cache;
	struct LRU_cache *cache;
	unsigned long map_entry_nr_per_pg;
	struct mutex cache_lock;
#endif
	struct nullb_device *dev;//for gc count
};

// void conv_init_namespace(struct nvmev_ns *ns, uint32_t id, uint64_t size, void *mapped_addr,
// 			 uint32_t cpu_nr_dispatcher);
void conv_init_namespace(struct nullb_device *dev, uint64_t size);

// void conv_remove_namespace(struct nvmev_ns *ns);
void conv_remove_namespace(struct nullb_device *dev);

// bool conv_proc_nvme_io_cmd(struct nvmev_ns *ns, struct nvmev_request *req,
// 			   struct nvmev_result *ret);

bool conv_write(struct nullb_cmd *cmd, sector_t sector,
				    unsigned int nr_sectors);

bool conv_read(struct nullb_cmd *cmd, sector_t sector,
				    unsigned int nr_sectors);

#endif
