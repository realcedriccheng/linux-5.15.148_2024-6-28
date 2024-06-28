// SPDX-License-Identifier: GPL-2.0-only

// #include "nvmev.h"
#include "ssd.h"
#include "zns_ftl.h"

// void schedule_internal_operation(int sqid, unsigned long long nsecs_target,
// 				 struct buffer *write_buffer, size_t buffs_to_release);

// static inline uint32_t __nr_lbas_from_rw_cmd(struct nvme_rw_command *cmd)
// {
// 	return cmd->length + 1;
// }

// static bool __check_boundary_error(struct zns_ftl *zns_ftl, uint64_t slba, uint32_t nr_lba)
// {
// 	return lba_to_zone(zns_ftl, slba) == lba_to_zone(zns_ftl, slba + nr_lba - 1);
// }

// static void __increase_write_ptr(struct zns_ftl *zns_ftl, uint32_t zid, uint32_t nr_lba)
// {
// 	struct zone_descriptor *zone_descs = zns_ftl->zone_descs;
// 	uint64_t cur_write_ptr = zone_descs[zid].wp;
// 	uint64_t zone_capacity = zone_descs[zid].zone_capacity;

// 	cur_write_ptr += nr_lba;

// 	zone_descs[zid].wp = cur_write_ptr;

// 	if (cur_write_ptr == (zone_to_slba(zns_ftl, zid) + zone_capacity)) {
// 		//change state to ZSF
// 		release_zone_resource(zns_ftl, OPEN_ZONE);
// 		release_zone_resource(zns_ftl, ACTIVE_ZONE);

// 		if (zone_descs[zid].zrwav)
// 			ASSERT(0);

// 		change_zone_state(zns_ftl, zid, ZONE_STATE_FULL);
// 	} else if (cur_write_ptr > (zone_to_slba(zns_ftl, zid) + zone_capacity)) {
// 		NVMEV_ERROR("[%s] Write Boundary error!!\n", __func__);
// 	}
// }

static inline struct ppa __lpn_to_ppa(struct zns_ftl *zns_ftl, uint64_t lpn)
{
	struct ssdparams *spp = &zns_ftl->ssd->sp;
	struct znsparams *zpp = &zns_ftl->zp;
	uint64_t zone = lpn_to_zone(zns_ftl, lpn); // find corresponding zone
	uint64_t off = lpn - zone_to_slpn(zns_ftl, zone);

	uint32_t sdie = (zone * zpp->dies_per_zone) % spp->tt_luns;
	uint32_t die = sdie + ((off / spp->pgs_per_oneshotpg) % zpp->dies_per_zone);

	uint32_t channel = die_to_channel(zns_ftl, die);
	uint32_t lun = die_to_lun(zns_ftl, die);
	struct ppa ppa = {
		.g = {
			.lun = lun,
			.ch = channel,
			.pg = off % spp->pgs_per_oneshotpg,
		},
	};

	return ppa;
}

static bool __zns_write(struct nullb_cmd *cmd, sector_t sector,
				    unsigned int nr_sectors)
{
	struct zns_ftl *zns_ftl = cmd->nq->dev->zns_ftl;
	// struct zone_descriptor *zone_descs = zns_ftl->zone_descs;
	struct ssdparams *spp;
	// struct nvme_rw_command *cmd = &(req->cmd->rw);

	uint64_t slba = sector;
	uint64_t nr_lba = nr_sectors;
	uint64_t slpn, elpn, lpn;
	// get zone from start_lbai
	// uint32_t zid = lba_to_zone(zns_ftl, slba);
	// enum zone_state state = zone_descs[zid].state;

	uint64_t nsecs_start = cmd->nsecs_start;///////////////
	uint64_t nsecs_xfer_completed = nsecs_start;
	uint64_t nsecs_latest = nsecs_start;
	// uint32_t status = NVME_SC_SUCCESS;

	uint64_t pgs = 0;

	// struct buffer *write_buffer;

	// if (cmd->opcode == nvme_cmd_zone_append) {
	// 	slba = zone_descs[zid].wp;
	// 	cmd->slba = slba;
	// }

	if (zns_ftl->ssd){
		spp = &zns_ftl->ssd->sp;
	}else
		return false;

	slpn = lba_to_lpn(zns_ftl, slba);
	elpn = lba_to_lpn(zns_ftl, slba + nr_lba - 1);
	// zone_elpn = zone_to_elpn(zns_ftl, zid);

	NVMEV_ZNS_DEBUG("%s slba 0x%llx nr_lba 0x%lx\n", __func__, slba,
			nr_lba);

	// if (zns_ftl->zp.zone_wb_size)
	// 	write_buffer = &(zns_ftl->zone_write_buffer[zid]);
	// else
	// 	write_buffer = zns_ftl->ssd->write_buffer;

	// if (buffer_allocate(write_buffer, LBA_TO_BYTE(nr_lba)) < LBA_TO_BYTE(nr_lba))
	// 	return false;

	// if ((LBA_TO_BYTE(nr_lba) % spp->write_unit_size) != 0) {
	// 	// status = NVME_SC_ZNS_INVALID_WRITE;
	// 	goto out;
	// }

	// if (__check_boundary_error(zns_ftl, slba, nr_lba) == false) {
	// 	// return boundary error
	// 	// status = NVME_SC_ZNS_ERR_BOUNDARY;
	// 	goto out;
	// }

	// check if slba == current write pointer
	// if (slba != zone_descs[zid].wp) {
	// 	NVMEV_ERROR("%s WP error slba 0x%llx nr_lba 0x%llx zone_id %d wp %llx state %d\n",
	// 		    __func__, slba, nr_lba, zid, zns_ftl->zone_descs[zid].wp, state);
	// 	status = NVME_SC_ZNS_INVALID_WRITE;
	// 	goto out;
	// }

	// switch (state) {
	// case ZONE_STATE_EMPTY: {
	// 	// check if slba == start lba in zone
	// 	if (slba != zone_descs[zid].zslba) {
	// 		status = NVME_SC_ZNS_INVALID_WRITE;
	// 		goto out;
	// 	}

	// 	if (is_zone_resource_full(zns_ftl, ACTIVE_ZONE)) {
	// 		status = NVME_SC_ZNS_NO_ACTIVE_ZONE;
	// 		goto out;
	// 	}
	// 	if (is_zone_resource_full(zns_ftl, OPEN_ZONE)) {
	// 		status = NVME_SC_ZNS_NO_OPEN_ZONE;
	// 		goto out;
	// 	}
	// 	acquire_zone_resource(zns_ftl, ACTIVE_ZONE);
	// 	// go through
	// }
	// case ZONE_STATE_CLOSED: {
	// 	if (acquire_zone_resource(zns_ftl, OPEN_ZONE) == false) {
	// 		status = NVME_SC_ZNS_NO_OPEN_ZONE;
	// 		goto out;
	// 	}

	// 	// change to ZSIO
	// 	change_zone_state(zns_ftl, zid, ZONE_STATE_OPENED_IMPL);
	// 	break;
	// }
	// case ZONE_STATE_OPENED_IMPL:
	// case ZONE_STATE_OPENED_EXPL: {
	// 	break;
	// }
	// case ZONE_STATE_FULL:
	// 	status = NVME_SC_ZNS_ERR_FULL;
	// case ZONE_STATE_READ_ONLY:
	// 	status = NVME_SC_ZNS_ERR_READ_ONLY;
	// case ZONE_STATE_OFFLINE:
	// 	status = NVME_SC_ZNS_ERR_OFFLINE;
	// 	goto out;
	// }

	// __increase_write_ptr(zns_ftl, zid, nr_lba);

	// get delay from nand model
	// printk("zufs-write: start_lpn: %llu nr_lba: %llu end_lpn: %llu\n",slpn,nr_lba,elpn);
	// mutex_lock(&zns_ftl->write_lock);
	nsecs_latest = nsecs_start;
	nsecs_latest = ssd_advance_write_buffer(zns_ftl->ssd, nsecs_latest, LBA_TO_BYTE(nr_lba));
	nsecs_xfer_completed = nsecs_latest;
	// printk("start\n");

	for (lpn = slpn; lpn <= elpn; lpn += pgs) {
		struct ppa ppa;
		uint64_t pg_off;

		ppa = __lpn_to_ppa(zns_ftl, lpn);
		// if (pg->status != PG_FREE)
		// printk("zufs-write: ppa: %llu %llu %llu %llu %llu || lpn: %llu nr_lba: %llu elpn: %llu|| %llu %llu %llu %llu\n",ppa.g.pg,ppa.g.blk,ppa.g.pl,ppa.g.lun,ppa.g.ch,lpn,nr_lba,elpn,spp->pgs_per_blk, spp->blks_per_lun, spp->luns_per_ch, spp->nchs);

		pg_off = ppa.g.pg % spp->pgs_per_oneshotpg;
		pgs = min(elpn - lpn + 1, (uint64_t)(spp->pgs_per_oneshotpg - pg_off));
		// printk("pgs: %llu\n",pgs);

		/* Aggregate write io in flash page */
		// if (((pg_off + pgs) == spp->pgs_per_oneshotpg) || ((elpn - lpn) < (uint64_t)(spp->pgs_per_oneshotpg - pg_off))) {
		if (((pg_off + pgs) == spp->pgs_per_oneshotpg)) {
			struct nand_cmd swr = {
				.type = USER_IO,
				.cmd = NAND_WRITE,
				.stime = nsecs_xfer_completed,
				.xfer_size = spp->pgs_per_oneshotpg * spp->pgsz,
				.interleave_pci_dma = false,
				.ppa = &ppa,
			};
			// size_t bufs_to_release;
			// uint32_t unaligned_space =
			// 	zns_ftl->zp.zone_size % (spp->pgs_per_oneshotpg * spp->pgsz);
			uint64_t nsecs_completed = ssd_advance_nand(zns_ftl->ssd, &swr);
			// printk("ssd_advance_nand\n");

			nsecs_latest = max(nsecs_completed, nsecs_latest);
			NVMEV_ZNS_DEBUG("%s Flush slba 0x%llx nr_lba 0x%lx\n",
					__func__, slba, nr_lba);

			// if (((lpn + pgs - 1) == zone_elpn) && (unaligned_space > 0))
			// 	bufs_to_release = unaligned_space;
			// else
			// 	bufs_to_release = spp->pgs_per_oneshotpg * spp->pgsz;

			// schedule_internal_operation(req->sq_id, nsecs_completed, write_buffer,
			// 			    bufs_to_release);
		}
	}
	// mutex_unlock(&zns_ftl->write_lock);
// out:
	// ret->status = status;
	// if ((cmd->control & NVME_RW_FUA) ||
	//     (spp->write_early_completion == 0)) /*Wait all flash operations*/
		cmd->nsecs_target = nsecs_latest;
	// else /*Early completion*/
	// 	ret->nsecs_target = nsecs_xfer_completed;

	return true;
}

// static bool __zns_write_zrwa(struct zns_ftl *zns_ftl, struct nvmev_request *req,
// 			     struct nvmev_result *ret)
// {
// 	struct zone_descriptor *zone_descs = zns_ftl->zone_descs;
// 	struct ssdparams *spp = &zns_ftl->ssd->sp;
// 	struct znsparams *zpp = &zns_ftl->zp;
// 	struct nvme_rw_command *cmd = &(req->cmd->rw);
// 	uint64_t slba = cmd->slba;
// 	uint64_t nr_lba = __nr_lbas_from_rw_cmd(cmd);
// 	uint64_t elba = cmd->slba + nr_lba - 1;

// 	// get zone from start_lbai
// 	uint32_t zid = lba_to_zone(zns_ftl, slba);
// 	enum zone_state state = zone_descs[zid].state;

// 	uint64_t prev_wp = zone_descs[zid].wp;
// 	const uint32_t lbas_per_zrwa = zpp->lbas_per_zrwa;
// 	const uint32_t lbas_per_zrwafg = zpp->lbas_per_zrwafg;
// 	uint64_t zrwa_impl_start = prev_wp + lbas_per_zrwa;
// 	uint64_t zrwa_impl_end = prev_wp + (2 * lbas_per_zrwa) - 1;

// 	uint64_t nsecs_start = req->nsecs_start;
// 	uint64_t nsecs_completed = nsecs_start;
// 	uint64_t nsecs_xfer_completed = nsecs_start;
// 	uint64_t nsecs_latest = nsecs_start;
// 	uint32_t status = NVME_SC_SUCCESS;

// 	struct ppa ppa;
// 	struct nand_cmd swr;

// 	uint64_t nr_lbas_flush = 0, lpn, remaining, pgs = 0, pg_off;

// 	NVMEV_DEBUG(
// 		"%s slba 0x%llx nr_lba 0x%llx zone_id %d state %d wp 0x%llx zrwa_impl_start 0x%llx zrwa_impl_end 0x%llx  buffer %lu\n",
// 		__func__, slba, nr_lba, zid, state, prev_wp, zrwa_impl_start, zrwa_impl_end,
// 		zns_ftl->zwra_buffer[zid].remaining);

// 	if ((LBA_TO_BYTE(nr_lba) % spp->write_unit_size) != 0) {
// 		status = NVME_SC_ZNS_INVALID_WRITE;
// 		goto out;
// 	}

// 	if (__check_boundary_error(zns_ftl, slba, nr_lba) == false) {
// 		// return boundary error
// 		status = NVME_SC_ZNS_ERR_BOUNDARY;
// 		goto out;
// 	}

// 	// valid range : wp <=  <= wp + 2*(size of zwra) -1
// 	if (slba < zone_descs[zid].wp || elba > zrwa_impl_end) {
// 		NVMEV_ERROR("%s slba 0x%llx nr_lba 0x%llx zone_id %d wp 0x%llx state %d\n",
// 			    __func__, slba, nr_lba, zid, zone_descs[zid].wp, state);
// 		status = NVME_SC_ZNS_INVALID_WRITE;
// 		goto out;
// 	}

// 	switch (state) {
// 	case ZONE_STATE_CLOSED:
// 	case ZONE_STATE_EMPTY: {
// 		if (acquire_zone_resource(zns_ftl, OPEN_ZONE) == false) {
// 			status = NVME_SC_ZNS_NO_OPEN_ZONE;
// 			goto out;
// 		}

// 		if (!buffer_allocate(&zns_ftl->zwra_buffer[zid], zpp->zrwa_size))
// 			NVMEV_ASSERT(0);

// 		// change to ZSIO
// 		change_zone_state(zns_ftl, zid, ZONE_STATE_OPENED_IMPL);
// 		break;
// 	}
// 	case ZONE_STATE_OPENED_IMPL:
// 	case ZONE_STATE_OPENED_EXPL: {
// 		break;
// 	}
// 	case ZONE_STATE_FULL:
// 		status = NVME_SC_ZNS_ERR_FULL;
// 		goto out;
// 	case ZONE_STATE_READ_ONLY:
// 		status = NVME_SC_ZNS_ERR_READ_ONLY;
// 		goto out;
// 	case ZONE_STATE_OFFLINE:
// 		status = NVME_SC_ZNS_ERR_OFFLINE;
// 		goto out;
// #if 0
// 		case ZONE_STATE_EMPTY :
// 			return NVME_SC_ZNS_INVALID_ZONE_OPERATION;
// #endif
// 	}

// 	if (elba >= zrwa_impl_start) {
// 		nr_lbas_flush = DIV_ROUND_UP((elba - zrwa_impl_start + 1), lbas_per_zrwafg) *
// 				lbas_per_zrwafg;

// 		NVMEV_DEBUG("%s implicitly flush zid %d wp before 0x%llx after 0x%llx buffer %lu",
// 			    __func__, zid, prev_wp, zone_descs[zid].wp + nr_lbas_flush,
// 			    zns_ftl->zwra_buffer[zid].remaining);
// 	} else if (elba == zone_to_elba(zns_ftl, zid)) {
// 		// Workaround. move wp to end of the zone and make state full implicitly
// 		nr_lbas_flush = elba - prev_wp + 1;

// 		NVMEV_DEBUG("%s end of zone zid %d wp before 0x%llx after 0x%llx buffer %lu",
// 			    __func__, zid, prev_wp, zone_descs[zid].wp + nr_lbas_flush,
// 			    zns_ftl->zwra_buffer[zid].remaining);
// 	}

// 	if (nr_lbas_flush > 0) {
// 		if (!buffer_allocate(&zns_ftl->zwra_buffer[zid], LBA_TO_BYTE(nr_lbas_flush)))
// 			return false;

// 		__increase_write_ptr(zns_ftl, zid, nr_lbas_flush);
// 	}
// 	// get delay from nand model
// 	nsecs_latest = nsecs_start;
// 	nsecs_latest = ssd_advance_write_buffer(zns_ftl->ssd, nsecs_latest, LBA_TO_BYTE(nr_lba));
// 	nsecs_xfer_completed = nsecs_latest;

// 	lpn = lba_to_lpn(zns_ftl, prev_wp);
// 	remaining = nr_lbas_flush / spp->secs_per_pg;
// 	/* Aggregate write io in flash page */
// 	while (remaining > 0) {
// 		ppa = __lpn_to_ppa(zns_ftl, lpn);
// 		pg_off = ppa.g.pg % spp->pgs_per_oneshotpg;
// 		pgs = min(remaining, (uint64_t)(spp->pgs_per_oneshotpg - pg_off));

// 		if ((pg_off + pgs) == spp->pgs_per_oneshotpg) {
// 			swr.type = USER_IO;
// 			swr.cmd = NAND_WRITE;
// 			swr.stime = nsecs_xfer_completed;
// 			swr.xfer_size = spp->pgs_per_oneshotpg * spp->pgsz;
// 			swr.interleave_pci_dma = false;
// 			swr.ppa = &ppa;

// 			nsecs_completed = ssd_advance_nand(zns_ftl->ssd, &swr);
// 			nsecs_latest = max(nsecs_completed, nsecs_latest);

// 			schedule_internal_operation(req->sq_id, nsecs_completed,
// 						    &zns_ftl->zwra_buffer[zid],
// 						    spp->pgs_per_oneshotpg * spp->pgsz);
// 		}

// 		lpn += pgs;
// 		remaining -= pgs;
// 	}

// out:
// 	ret->status = status;

// 	if ((cmd->control & NVME_RW_FUA) ||
// 	    (spp->write_early_completion == 0)) /*Wait all flash operations*/
// 		ret->nsecs_target = nsecs_latest;
// 	else /*Early completion*/
// 		ret->nsecs_target = nsecs_xfer_completed;

// 	return true;
// }

bool zns_write(struct nullb_cmd *cmd, sector_t sector,
				    unsigned int nr_sectors)
{
	// struct zns_ftl *zns_ftl = cmd->nq->dev->zns_ftl;
	// struct zone_descriptor *zone_descs = zns_ftl->zone_descs;
	// struct nvme_rw_command *cmd = &(req->cmd->rw);
	// uint64_t slpn = lba_to_lpn(zns_ftl, sector);

	// get zone from start_lba
	// uint32_t zid = lpn_to_zone(zns_ftl, slpn);

	// NVMEV_DEBUG("%s slba 0x%llx zone_id %d \n", __func__, sector, zid);

	// if (zone_descs[zid].zrwav == 0)
	// printk("qwj-zns_write\n");
		return __zns_write(cmd, sector, nr_sectors);
	// else
		// return __zns_write_zrwa(zns_ftl, req, ret);
}

#ifdef SLC_CACHE
static inline unsigned int null_zone_no(struct nullb_device *dev, sector_t sect)
{
	return sect >> ilog2(dev->zone_size_sects);
}

static inline struct ppa __lpn_to_ppa_slc(struct slc_cache *slc_cache, uint64_t lpn)
{
	struct ssdparams *spp = &slc_cache->ssd->sp;
	struct slcparams *slcpp = &slc_cache->slcp;
	uint64_t zone = lpn / (slc_cache->slcp.zone_size / slc_cache->ssd->sp.pgsz);
	uint64_t off = lpn - zone * (slc_cache->slcp.zone_size / slc_cache->ssd->sp.pgsz);
	// uint64_t zone = lpn_to_zone(zns_ftl, lpn); // find corresponding zone
	// uint64_t off = lpn - zone_to_slpn(zns_ftl, zone);

	uint32_t sdie = (zone * slcpp->dies_per_zone) % spp->tt_luns;
	uint32_t die = sdie + ((off / spp->pgs_per_oneshotpg) % slcpp->dies_per_zone);

	uint32_t channel = die % slc_cache->ssd->sp.nchs;
	uint32_t lun = die / slc_cache->ssd->sp.nchs;
	// uint32_t channel = die_to_channel(zns_ftl, die);
	// uint32_t lun = die_to_lun(zns_ftl, die);
	struct ppa ppa = {
		.g = {
			.lun = lun,
			.ch = channel,
			.pg = off % spp->pgs_per_oneshotpg,
		},
	};

	return ppa;
}

static bool __zns_write_slc(struct nullb_cmd *cmd, sector_t sector,
				    unsigned int nr_sectors)
{
	struct slc_cache *slc_cache = cmd->nq->dev->slc_cache;
	// struct zone_descriptor *zone_descs = zns_ftl->zone_descs;
	struct ssdparams *spp;
	// struct nvme_rw_command *cmd = &(req->cmd->rw);

	uint64_t slba = sector;
	uint64_t nr_lba = nr_sectors;
	uint64_t slpn, elpn, lpn;
	// get zone from start_lbai
	// uint32_t zid = lba_to_zone(zns_ftl, slba);
	// enum zone_state state = zone_descs[zid].state;

	uint64_t nsecs_start = cmd->nsecs_start;
	uint64_t nsecs_xfer_completed = nsecs_start;
	uint64_t nsecs_latest = nsecs_start;
	// uint32_t status = NVME_SC_SUCCESS;

	uint64_t pgs = 0;

	// struct buffer *write_buffer;

	// if (cmd->opcode == nvme_cmd_zone_append) {
	// 	slba = zone_descs[zid].wp;
	// 	cmd->slba = slba;
	// }

	if (slc_cache->ssd){
		spp = &slc_cache->ssd->sp;
	}else
		return false;

	slpn = slba / slc_cache->ssd->sp.secs_per_pg;
	elpn = (slba + nr_lba - 1) / slc_cache->ssd->sp.secs_per_pg;
	// slpn = lba_to_lpn(slc_cache, slba);
	// elpn = lba_to_lpn(slc_cache, slba + nr_lba - 1);
	// zone_elpn = zone_to_elpn(zns_ftl, zid);

	NVMEV_ZNS_DEBUG("%s slba 0x%llx nr_lba 0x%lx\n", __func__, slba,
			nr_lba);

	// if (zns_ftl->zp.zone_wb_size)
	// 	write_buffer = &(zns_ftl->zone_write_buffer[zid]);
	// else
	// 	write_buffer = zns_ftl->ssd->write_buffer;

	// if (buffer_allocate(write_buffer, LBA_TO_BYTE(nr_lba)) < LBA_TO_BYTE(nr_lba))
	// 	return false;

	// if ((LBA_TO_BYTE(nr_lba) % spp->write_unit_size) != 0) {
	// 	// status = NVME_SC_ZNS_INVALID_WRITE;
	// 	goto out;
	// }

	// if (__check_boundary_error(zns_ftl, slba, nr_lba) == false) {
	// 	// return boundary error
	// 	// status = NVME_SC_ZNS_ERR_BOUNDARY;
	// 	goto out;
	// }

	// check if slba == current write pointer
	// if (slba != zone_descs[zid].wp) {
	// 	NVMEV_ERROR("%s WP error slba 0x%llx nr_lba 0x%llx zone_id %d wp %llx state %d\n",
	// 		    __func__, slba, nr_lba, zid, zns_ftl->zone_descs[zid].wp, state);
	// 	status = NVME_SC_ZNS_INVALID_WRITE;
	// 	goto out;
	// }

	// switch (state) {
	// case ZONE_STATE_EMPTY: {
	// 	// check if slba == start lba in zone
	// 	if (slba != zone_descs[zid].zslba) {
	// 		status = NVME_SC_ZNS_INVALID_WRITE;
	// 		goto out;
	// 	}

	// 	if (is_zone_resource_full(zns_ftl, ACTIVE_ZONE)) {
	// 		status = NVME_SC_ZNS_NO_ACTIVE_ZONE;
	// 		goto out;
	// 	}
	// 	if (is_zone_resource_full(zns_ftl, OPEN_ZONE)) {
	// 		status = NVME_SC_ZNS_NO_OPEN_ZONE;
	// 		goto out;
	// 	}
	// 	acquire_zone_resource(zns_ftl, ACTIVE_ZONE);
	// 	// go through
	// }
	// case ZONE_STATE_CLOSED: {
	// 	if (acquire_zone_resource(zns_ftl, OPEN_ZONE) == false) {
	// 		status = NVME_SC_ZNS_NO_OPEN_ZONE;
	// 		goto out;
	// 	}

	// 	// change to ZSIO
	// 	change_zone_state(zns_ftl, zid, ZONE_STATE_OPENED_IMPL);
	// 	break;
	// }
	// case ZONE_STATE_OPENED_IMPL:
	// case ZONE_STATE_OPENED_EXPL: {
	// 	break;
	// }
	// case ZONE_STATE_FULL:
	// 	status = NVME_SC_ZNS_ERR_FULL;
	// case ZONE_STATE_READ_ONLY:
	// 	status = NVME_SC_ZNS_ERR_READ_ONLY;
	// case ZONE_STATE_OFFLINE:
	// 	status = NVME_SC_ZNS_ERR_OFFLINE;
	// 	goto out;
	// }

	// __increase_write_ptr(zns_ftl, zid, nr_lba);

	// get delay from nand model
	// printk("zufs-write: start_lpn: %llu nr_lba: %llu end_lpn: %llu\n",slpn,nr_lba,elpn);
	// mutex_lock(&slc_cache->write_lock);
	nsecs_latest = nsecs_start;
	nsecs_latest = ssd_advance_write_buffer(slc_cache->ssd, nsecs_latest, LBA_TO_BYTE(nr_lba));
	nsecs_xfer_completed = nsecs_latest;
	// printk("start\n");

	for (lpn = slpn; lpn <= elpn; lpn += pgs) {
		struct ppa ppa;
		uint64_t pg_off;

		ppa = __lpn_to_ppa_slc(slc_cache, lpn);
		// if (pg->status != PG_FREE)
		// printk("zufs-write: ppa: %llu %llu %llu %llu %llu || lpn: %llu nr_lba: %llu elpn: %llu|| %llu %llu %llu %llu\n",ppa.g.pg,ppa.g.blk,ppa.g.pl,ppa.g.lun,ppa.g.ch,lpn,nr_lba,elpn,spp->pgs_per_blk, spp->blks_per_lun, spp->luns_per_ch, spp->nchs);

		pg_off = ppa.g.pg % spp->pgs_per_oneshotpg;
		pgs = min(elpn - lpn + 1, (uint64_t)(spp->pgs_per_oneshotpg - pg_off));
		// printk("pgs: %llu\n",pgs);

		/* Aggregate write io in flash page */
		// if (((pg_off + pgs) == spp->pgs_per_oneshotpg) || ((elpn - lpn) < (uint64_t)(spp->pgs_per_oneshotpg - pg_off))) {
		if (((pg_off + pgs) == spp->pgs_per_oneshotpg)) {
			struct nand_cmd swr = {
				.type = USER_IO,
				.cmd = NAND_WRITE,
				.stime = nsecs_xfer_completed,
				.xfer_size = spp->pgs_per_oneshotpg * spp->pgsz,
				.interleave_pci_dma = false,
				.ppa = &ppa,
			};
			// size_t bufs_to_release;
			// uint32_t unaligned_space =
			// 	zns_ftl->zp.zone_size % (spp->pgs_per_oneshotpg * spp->pgsz);
			uint64_t nsecs_completed = ssd_advance_nand(slc_cache->ssd, &swr);
			// printk("ssd_advance_nand\n");

			nsecs_latest = max(nsecs_completed, nsecs_latest);
			NVMEV_ZNS_DEBUG("%s Flush slba 0x%llx nr_lba 0x%lx\n",
					__func__, slba, nr_lba);

			// if (((lpn + pgs - 1) == zone_elpn) && (unaligned_space > 0))
			// 	bufs_to_release = unaligned_space;
			// else
			// 	bufs_to_release = spp->pgs_per_oneshotpg * spp->pgsz;

			// schedule_internal_operation(req->sq_id, nsecs_completed, write_buffer,
			// 			    bufs_to_release);
		}
	}
	// mutex_unlock(&slc_cache->write_lock);
// out:
	// ret->status = status;
	// if ((cmd->control & NVME_RW_FUA) ||
	//     (spp->write_early_completion == 0)) /*Wait all flash operations*/
	// start
	cmd->nsecs_start = nsecs_latest;
	cmd->nsecs_target = nsecs_latest;
	// else /*Early completion*/
	// 	ret->nsecs_target = nsecs_xfer_completed;

	return true;
}

bool zns_write_slc(struct nullb_cmd *cmd, sector_t sector,
				    unsigned int nr_sectors, struct nullb_zone *zone)
{
	struct nullb_device *dev = cmd->nq->dev;
	unsigned int zno = null_zone_no(dev, sector);

	struct slc_cache *slc_cache = dev->slc_cache;
	struct slcparams *slcpp = &slc_cache->slcp;
 	
	int zno_map = 0;
	sector_t sector_itor;
	sector_t map_sector;
	sector_t map_wp;
	sector_t bound;
	bool writeback_all = false;
	// unsigned long flags;

	// spin_lock_irqsave(&slc_cache->write_lock, flags);
	for (zno_map = 0; zno_map < slcpp->nr_zones; zno_map++) {
		if(slcpp->zone_map[zno_map] == zno) break;
	}

	if (zno_map >= slcpp->nr_zones) {
		while (atomic_read(&slcpp->active_zones) == slcpp->nr_zones) {//单线程增加
			// spin_unlock_irqrestore(&slc_cache->write_lock, flags);
			int zno_map_tmp = slcpp->nr_zones;
			// NVMEV_ERROR("[%s] No corresponding zno %d\n", __func__, zno);
			// NVMEV_ASSERT(atomic_read(&slcpp->active_zones) < slcpp->nr_zones);
			// return false;
			for (zno_map = 0; zno_map < slcpp->nr_zones; zno_map++) {
				if (slcpp->zone_map[zno_map] == -1) {
					zno_map_tmp = zno_map;
				}
				printk("slcpp->zone_map[%d] = %ld, slcpp->used_sectors[%d] = %ld\n",zno_map,slcpp->zone_map[zno_map],zno_map,slcpp->used_sectors[zno_map]);
			}

			NVMEV_ASSERT(zno_map != slcpp->nr_zones);

			mutex_unlock(&dev->zns_ftl->write_lock);
			null_unlock_zone(dev, zone);
			cond_resched();
			null_lock_zone(dev, zone);
			mutex_lock(&dev->zns_ftl->write_lock);
		}
		// atomic_inc(&slcpp->active_zones);
		NVMEV_ASSERT(atomic_inc_return(&slcpp->active_zones) <= slcpp->nr_zones);//非原子操作
		zno_map = 0;
		while (slcpp->zone_map[zno_map] != -1) zno_map++;
		slcpp->zone_map[zno_map] = zno;

		NVMEV_ASSERT(slcpp->used_sectors[zno_map] == 0);
		// bound = (zno_map + 1) * slcpp->secs_per_zone;
		// for (sector_itor = zno_map * slcpp->secs_per_zone; sector_itor < bound; sector_itor++) {
		// 	NVMEV_ASSERT(slcpp->bit_map[sector_itor] == false);
		// }

	}

	if ((zone->wp_tlc + slcpp->used_sectors[zno_map] + nr_sectors) < (zone->start + zone->capacity)) {
		map_sector = sector % slcpp->secs_per_zone + zno_map * slcpp->secs_per_zone;

		// printk("slcpp->secs_per_zone = %ld, sector = %ld\n",slcpp->secs_per_zone, sector);

		for(sector_itor = map_sector; sector_itor < map_sector + nr_sectors; sector_itor++) {
			slcpp->bit_map[sector_itor] = true;
			slcpp->used_sectors[zno_map]++;
			NVMEV_ASSERT(slcpp->used_sectors[zno_map] <= slcpp->secs_per_zone);
		}
	} else {
		NVMEV_ASSERT((zone->wp_tlc + slcpp->used_sectors[zno_map] + nr_sectors) == (zone->start + zone->capacity));

		map_sector = sector % slcpp->secs_per_zone + zno_map * slcpp->secs_per_zone;
		// printk("slcpp->secs_per_zone = %ld, sector = %ld\n",slcpp->secs_per_zone, sector);
		for(sector_itor = map_sector; sector_itor < map_sector + nr_sectors; sector_itor++) {
			slcpp->bit_map[sector_itor] = true;
			slcpp->used_sectors[zno_map]++;
			NVMEV_ASSERT(slcpp->used_sectors[zno_map] <= slcpp->secs_per_zone);
		}

		bound = (zno_map + 1) * slcpp->secs_per_zone;
		// printk("__zns_write slcpp->used_sectors[zno_map] = %ld  zone->wp_tlc = %ld zone->wp = %ld zone->start = %ld\n",slcpp->used_sectors[zno_map],zone->wp_tlc,zone->wp,zone->start);
		map_wp = zone->wp_tlc % slcpp->secs_per_zone + zno_map * slcpp->secs_per_zone;
		for (sector_itor = map_wp; sector_itor < bound; sector_itor++) {
			if (slcpp->bit_map[sector_itor] == true) {
				slcpp->bit_map[sector_itor] = false;
				slcpp->used_sectors[zno_map]--;
			}else{
				NVMEV_ASSERT(slcpp->bit_map[sector_itor] == true);//BUG
			}
			NVMEV_ASSERT(slcpp->used_sectors[zno_map] >= 0);
		}
		if (slcpp->used_sectors[zno_map] != 0){
			// spin_unlock_irqrestore(&slc_cache->write_lock, flags);
			NVMEV_ERROR("[%s] No corresponding used_sectors=%d zno_map=%d\n", __func__, slcpp->used_sectors[zno_map], zno_map);
			NVMEV_ASSERT(slcpp->used_sectors[zno_map] == 0);
		}
		writeback_all = true;
		// slcpp->zone_map[zno_map] = -1;
		// slcpp->active_zones--;
	}

	// spin_unlock_irqrestore(&slc_cache->write_lock, flags);

	// printk("zns_write_slc zno:%u, zno_map:%d sector:%lu",zno, zno_map,sector);

	if (!writeback_all) {
		// printk("slcpp->used_sectors[zno_map] = %ld  zone->wp_tlc = %ld zone->wp = %ld zone->start = %ld\n",slcpp->used_sectors[zno_map],zone->wp_tlc,zone->wp,zone->start);
		
		// printk("active_zones:%u,used_sectors:%d",slcpp->active_zones,slcpp->used_sectors[zno_map]);
		// struct zns_ftl *zns_ftl = cmd->nq->dev->zns_ftl;
		// struct zone_descriptor *zone_descs = zns_ftl->zone_descs;
		// struct nvme_rw_command *cmd = &(req->cmd->rw);
		// uint64_t slpn = lba_to_lpn(zns_ftl, sector);

		// get zone from start_lba
		// uint32_t zid = lpn_to_zone(zns_ftl, slpn);

		// NVMEV_DEBUG("%s slba 0x%llx zone_id %d \n", __func__, sector, zid);

		// if (zone_descs[zid].zrwav == 0)
		// printk("qwj-zns_write\n");
		__zns_write_slc(cmd, map_sector, nr_sectors);
	} else {
		if (atomic_read(&slcpp->active_zones) < slcpp->nr_zones){
		// if (false){
			struct nullb_cmd *cmd_tlc;
			cmd_tlc = &slcpp->cmd_tlc[zno_map];
			cmd_tlc->nq = cmd->nq;
			cmd_tlc->nsecs_start = cmd->nsecs_start;
			cmd_tlc->zno_map = zno_map;
			hrtimer_init(&cmd_tlc->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
			cmd_tlc->timer.function = null_cmd_timer_expired_slc;
			__zns_write(cmd_tlc, zone->wp_tlc, slcpp->used_sectors[zno_map] + nr_sectors);
			null_cmd_end_timer(cmd_tlc);
		} else {
			__zns_write(cmd, zone->wp_tlc, slcpp->used_sectors[zno_map] + nr_sectors);
			cmd->zno_map = zno_map;
		// cmd->zno_map = -1;
		// if (atomic_add_negative(-1, &cmd->nq->dev->slc_cache->slcp.active_zones)){
		// 	NVMEV_ASSERT(0);
		// }
		}
		zone->wp_tlc += (slcpp->used_sectors[zno_map] + nr_sectors);
		slcpp->zone_map[zno_map] = -1;
	}

	return 0;
	// else
		// return __zns_write_zrwa(zns_ftl, req, ret);

	
}

bool __zns_read_slc(struct nullb_cmd *cmd, sector_t sector,
				    unsigned int nr_sectors)
{
	struct slc_cache *slc_cache = cmd->nq->dev->slc_cache;
	struct ssdparams *spp;
	// struct zone_descriptor *zone_descs = zns_ftl->zone_descs;
	// struct nvme_rw_command *cmd = &(req->cmd->rw);

	uint64_t slba = sector;
	uint64_t nr_lba = nr_sectors;

	uint64_t slpn;
	uint64_t elpn;
	uint64_t lpn;

	// get zone from start_lba
	// uint32_t zid = lpn_to_zone(zns_ftl, slpn);
	// uint32_t status = NVME_SC_SUCCESS;
	uint64_t nsecs_start = cmd->nsecs_start;//////////////////
	uint64_t nsecs_completed = nsecs_start, nsecs_latest = 0;
	uint64_t pgs = 0, pg_off;
	struct ppa ppa;
	struct nand_cmd swr;
	// printk("qwj-zns_read\n");
	if (nr_sectors == 0) return true;
	if (slc_cache->ssd){
		spp = &slc_cache->ssd->sp;
		slpn = slba / slc_cache->ssd->sp.secs_per_pg;
		elpn = (slba + nr_lba - 1) / slc_cache->ssd->sp.secs_per_pg;
		// slpn = lba_to_lpn(zns_ftl, slba);
		// elpn = lba_to_lpn(zns_ftl, slba + nr_lba - 1);
	}else
		return false;

	// NVMEV_ZNS_DEBUG(
	// 	"%s slba 0x%llx nr_lba 0x%lx zone_id %d last lba 0x%llx\n",
	// 	__func__, slba, nr_lba, zid, (slba + nr_lba - 1));

	// if (zone_descs[zid].state == ZONE_STATE_OFFLINE) {
	// 	status = NVME_SC_ZNS_ERR_OFFLINE;
	// } else if (__check_boundary_error(zns_ftl, slba, nr_lba) == false) {
	// 	// return boundary error
	// 	status = NVME_SC_ZNS_ERR_BOUNDARY;
	// }

	// get delay from nand model
	nsecs_latest = nsecs_start;
	if (LBA_TO_BYTE(nr_lba) <= KB(4))
		nsecs_latest += spp->fw_4kb_rd_lat;
	else
		nsecs_latest += spp->fw_rd_lat;

	swr.type = USER_IO;
	swr.cmd = NAND_READ;
	swr.stime = nsecs_latest;
	swr.interleave_pci_dma = false;
	// printk("zufs-read: start_lpn: %llu nr_lba: %llu end_lpn: %llu\n",slpn,nr_lba,elpn);
	// mutex_lock(&slc_cache->write_lock);

	for (lpn = slpn; lpn <= elpn; lpn += pgs) {
		// ppa = __lpn_to_ppa(zns_ftl, lpn);
		ppa = __lpn_to_ppa_slc(slc_cache, lpn);
		pg_off = ppa.g.pg % spp->pgs_per_flashpg;
		pgs = min(elpn - lpn + 1, (uint64_t)(spp->pgs_per_flashpg - pg_off));
		swr.xfer_size = pgs * spp->pgsz;
		swr.ppa = &ppa;
		nsecs_completed = ssd_advance_nand(slc_cache->ssd, &swr);
		nsecs_latest = (nsecs_completed > nsecs_latest) ? nsecs_completed : nsecs_latest;
	}

	if (swr.interleave_pci_dma == false) {
		nsecs_completed = ssd_advance_pcie(slc_cache->ssd, nsecs_latest, nr_lba * spp->secsz);
		nsecs_latest = (nsecs_completed > nsecs_latest) ? nsecs_completed : nsecs_latest;
	}
	// mutex_unlock(&slc_cache->write_lock);
	// ret->status = status;
	// start
	cmd->nsecs_start = nsecs_latest;
	cmd->nsecs_target = nsecs_latest;
	return true;
}
// if return true,  need write slc
bool zns_read_slc(struct nullb_cmd *cmd, sector_t *sector,
				    unsigned int *nr_sectors, struct nullb_zone *zone)
{
	struct nullb_device *dev = cmd->nq->dev;
	unsigned int zno = null_zone_no(dev, *sector);

	struct slc_cache *slc_cache = dev->slc_cache;
	struct slcparams *slcpp = &slc_cache->slcp;
 	
	int zno_map;
	sector_t sector_itor;
	sector_t map_wp;
	sector_t map_sector;
	sector_t bound;
	// bool ret = false;

	// printk("zno:%u", zno);
	// return true;

	for (zno_map = 0; zno_map < slcpp->nr_zones; zno_map++) {
		if (slcpp->zone_map[zno_map] == zno) break;
	}

	// printk("zns_read_slc zno:%u, zno_map:%d",zno, zno_map);
	if (zno_map >= slcpp->nr_zones) {
		if (*sector == zone->wp_tlc) {
			zone->wp_tlc += *nr_sectors;
			return false;
		} else {
			return true;
		}
	}

	map_wp = zone->wp_tlc % slcpp->secs_per_zone + zno_map * slcpp->secs_per_zone;
	map_sector = *sector % slcpp->secs_per_zone + zno_map * slcpp->secs_per_zone;
	for (sector_itor = map_wp; slcpp->bit_map[sector_itor] == true && sector_itor < map_sector; sector_itor++) {
		// slcpp->bit_map[sector_itor] = false;
		// slcpp->used_sectors[zno_map]--;
	}
	if (sector_itor < map_sector) {
		return true;
	}

	// map_wp = zone->wp_tlc % slcpp->secs_per_zone + zno_map * slcpp->secs_per_zone;
	// map_sector = *sector % slcpp->secs_per_zone + zno_map * slcpp->secs_per_zone;
	// if (zone->wp_tlc != *sector) {
		for (sector_itor = map_wp; slcpp->bit_map[sector_itor] == true && sector_itor < map_sector; sector_itor++) {
			slcpp->bit_map[sector_itor] = false;
			slcpp->used_sectors[zno_map]--;
			NVMEV_ASSERT(slcpp->used_sectors[zno_map] >= 0);
		}
	// }

	if (sector_itor == map_sector) {
		__zns_read_slc(cmd, map_wp, sector_itor - map_wp);
		bound = (zno_map + 1) * slcpp->secs_per_zone;
		for (sector_itor = map_sector + *nr_sectors; slcpp->bit_map[sector_itor] == true && sector_itor < bound; sector_itor++) {
			slcpp->bit_map[sector_itor] = false;
			slcpp->used_sectors[zno_map]--;
			NVMEV_ASSERT(slcpp->used_sectors[zno_map] >= 0);
		}
		__zns_read_slc(cmd, map_sector + *nr_sectors, sector_itor - (map_sector + *nr_sectors));
	} else {
		// NVMEV_ERROR("[%s] No corresponding used_sectors=%d zno_map=%d\n", __func__, slcpp->used_sectors[zno_map], zno_map);
		NVMEV_ASSERT(sector_itor >= map_sector);
	}
	// printk("active_zones:%u,used_sectors:%d",slcpp->active_zones,slcpp->used_sectors[zno_map]);
	// if (ret == false){
		*sector = zone->wp_tlc;
		*nr_sectors = sector_itor - map_wp;
	// }
	zone->wp_tlc += *nr_sectors;
	if ((zone->wp_tlc == (zone->start + zone->capacity))) {
		NVMEV_ASSERT(slcpp->used_sectors[zno_map] == 0);
		slcpp->zone_map[zno_map] = -1;
		if (map_sector != map_wp){
		// if (false){
			cmd->zno_map = zno_map;
		} else {
			cmd->zno_map = -1;
			if (atomic_add_negative(-1, &slcpp->active_zones)){
				NVMEV_ASSERT(0);
			}
		}

	}
	NVMEV_ASSERT(zone->wp_tlc <= (zone->start + zone->capacity));
	return false;

	// if (wp < *sector) {
	// 	sector_t map_wp =  wp % slcpp->secs_per_zone + zno_map * slcpp->secs_per_zone;
	// 	map_sector = *sector % slcpp->secs_per_zone + zno_map * slcpp->secs_per_zone;
	// 	for (sector_itor = map_wp; slcpp->bit_map[sector_itor] == true && slcpp->used_sectors[zno_map] != 0; sector_itor++) {
	// 		slcpp->bit_map[sector_itor] = false;
	// 		sector_itor++;
	// 		slcpp->used_sectors[zno_map]--;
	// 	}
	// 	__zns_read_slc(cmd, map_wp, sector_itor - map_wp);
		
	// 	if (sector_itor == map_sector) {
	// 		for (sector_itor == map_sector + *nr_sectors; slcpp->bit_map[sector_itor] == true && slcpp->used_sectors[zno_map] != 0; sector_itor++) {
	// 			slcpp->bit_map[sector_itor] = false;
	// 			sector_itor++;
	// 			slcpp->used_sectors[zno_map]--;
	// 		}
	// 		__zns_read_slc(cmd, map_sector + *nr_sectors, sector_itor - (map_sector + *nr_sectors));
	// 	}

	// 	if (slcpp->used_sectors[zno_map] == 0) {
	// 		slcpp->zone_map[zno_map] = -1;
	// 		slcpp->active_zones--;
	// 	}

	// 	*sector = wp;
	// 	*nr_sectors = sector_itor - map_wp;
	// 	return true;
	// }

	// map_sector = *sector % slcpp->secs_per_zone + zno_map * slcpp->secs_per_zone;
	// sector_itor = map_sector + *nr_sectors;
	// // 越界判定
	// for (sector_itor == map_sector + *nr_sectors; slcpp->bit_map[sector_itor] == true && slcpp->used_sectors[zno_map] != 0; sector_itor++) {
	// 	slcpp->bit_map[sector_itor] = false;
	// 	sector_itor++;
	// 	slcpp->used_sectors[zno_map]--;
	// }

	// // slc清空了或者到达边界了
	// // if (slcpp->used_sectors[zno_map] == 0 || sector_itor % slcpp->secs_per_zone == 0) {
	// // 	slcpp->zone_map[zno_map] = -1;
	// // 	slcpp->used_sectors[zno_map] = 0;
	// // }

	// __zns_read_slc(cmd, map_sector + *nr_sectors, sector_itor - (map_sector + *nr_sectors));

	// if (slcpp->used_sectors[zno_map] == 0) {
	// 	slcpp->zone_map[zno_map] = -1;
	// 	slcpp->active_zones--;
	// }

	// *nr_sectors = sector_itor - map_sector;
	// return true;
}
#endif

// bool zns_read(struct nvmev_ns *ns, struct nvmev_request *req, struct nvmev_result *ret)
bool zns_read(struct nullb_cmd *cmd, sector_t sector,
				    unsigned int nr_sectors)
{
	struct zns_ftl *zns_ftl = cmd->nq->dev->zns_ftl;
	struct ssdparams *spp;
	// struct zone_descriptor *zone_descs = zns_ftl->zone_descs;
	// struct nvme_rw_command *cmd = &(req->cmd->rw);

	uint64_t slba = sector;
	uint64_t nr_lba = nr_sectors;

	uint64_t slpn;
	uint64_t elpn;
	uint64_t lpn;

	// get zone from start_lba
	// uint32_t zid = lpn_to_zone(zns_ftl, slpn);
	// uint32_t status = NVME_SC_SUCCESS;
	uint64_t nsecs_start = cmd->nsecs_start;//////////////////
	uint64_t nsecs_completed = nsecs_start, nsecs_latest = 0;
	uint64_t pgs = 0, pg_off;
	struct ppa ppa;
	struct nand_cmd swr;
	// printk("qwj-zns_read\n");
	if (zns_ftl->ssd){
		spp = &zns_ftl->ssd->sp;
		slpn = lba_to_lpn(zns_ftl, slba);
		elpn = lba_to_lpn(zns_ftl, slba + nr_lba - 1);
	}else
		return false;

	// NVMEV_ZNS_DEBUG(
	// 	"%s slba 0x%llx nr_lba 0x%lx zone_id %d last lba 0x%llx\n",
	// 	__func__, slba, nr_lba, zid, (slba + nr_lba - 1));

	// if (zone_descs[zid].state == ZONE_STATE_OFFLINE) {
	// 	status = NVME_SC_ZNS_ERR_OFFLINE;
	// } else if (__check_boundary_error(zns_ftl, slba, nr_lba) == false) {
	// 	// return boundary error
	// 	status = NVME_SC_ZNS_ERR_BOUNDARY;
	// }

	// get delay from nand model
	nsecs_latest = nsecs_start;
	if (LBA_TO_BYTE(nr_lba) <= KB(4))
		nsecs_latest += spp->fw_4kb_rd_lat;
	else
		nsecs_latest += spp->fw_rd_lat;

	swr.type = USER_IO;
	swr.cmd = NAND_READ;
	swr.stime = nsecs_latest;
	swr.interleave_pci_dma = false;
	// printk("zufs-read: start_lpn: %llu nr_lba: %llu end_lpn: %llu\n",slpn,nr_lba,elpn);
	mutex_lock(&zns_ftl->write_lock);

	for (lpn = slpn; lpn <= elpn; lpn += pgs) {
		ppa = __lpn_to_ppa(zns_ftl, lpn);
		pg_off = ppa.g.pg % spp->pgs_per_flashpg;
		pgs = min(elpn - lpn + 1, (uint64_t)(spp->pgs_per_flashpg - pg_off));
		swr.xfer_size = pgs * spp->pgsz;
		swr.ppa = &ppa;
		nsecs_completed = ssd_advance_nand(zns_ftl->ssd, &swr);
		nsecs_latest = (nsecs_completed > nsecs_latest) ? nsecs_completed : nsecs_latest;
	}

	if (swr.interleave_pci_dma == false) {
		nsecs_completed = ssd_advance_pcie(zns_ftl->ssd, nsecs_latest, nr_lba * spp->secsz);
		nsecs_latest = (nsecs_completed > nsecs_latest) ? nsecs_completed : nsecs_latest;
	}
	mutex_unlock(&zns_ftl->write_lock);
	// ret->status = status;
	cmd->nsecs_target = nsecs_latest;
	return true;
}
