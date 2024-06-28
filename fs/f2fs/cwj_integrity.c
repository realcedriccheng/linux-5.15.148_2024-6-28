#ifdef CONFIG_BLK_DEV_INTEGRITY
#include <linux/cwj_integrity.h>
#include <linux/blkdev.h>

static blk_status_t cwj_verify_fn(struct blk_integrity_iter *iter)
{
    // struct cwj_pi_tuple *pi = iter->prot_buf;
    // printk("READ:The value of iter->prot_buf is: 0x%lx\n", iter->prot_buf);
    // // 将字段都读出
    // printk("verify: iter->data_size=%u\n",iter->data_size);
    // printk("verify: iter->interval=%u\n",iter->interval);
    // printk("verify: pi->i_ino=%llu\n",pi->i_ino);
    // printk("verify: pi->index=%llu\n",pi->index);
    iter->prot_buf += sizeof(struct cwj_pi_tuple);
	return BLK_STS_OK;
}

static blk_status_t cwj_generate_fn(struct blk_integrity_iter *iter)
{
		struct cwj_pi_tuple *pi = iter->prot_buf;//填入bip->bip_vec中
		// printk("进入cwj_generate_fn\n");
		struct bio_vec *bv = (struct bio_vec *)(iter->data_buf);
		// printk("page->mapping->host>i_ino的size:%lu\n",sizeof(bv->bv_page->mapping->host->i_ino));
		// printk("page->index的size:%lu\n",sizeof(bv->bv_page->index));

		// printk("bv->len=%u\n",bv->bv_len);
		// printk("bv->bv_page->index=%lu\n",bv->bv_page->index);
		// printk("bv->bv_page->mapping->host->i_ino=%lu\n",bv->bv_page->mapping->host->i_ino);
		pi->i_ino = bv->bv_page->mapping->host->i_ino;
		pi->index = bv->bv_page->index;
		// printk("pi->i_ino=%u\n",pi->i_ino);
		// printk("pi->index=%u\n",pi->index);
		// printk("pi赋值成功\n");
        // printk("WRITE:The value of iter->prot_buf is: 0x%lx\n", iter->prot_buf);
        iter->prot_buf += sizeof(struct cwj_pi_tuple);

	return BLK_STS_OK;
}

static void cwj_prepare_fn(struct request *rq)
{
}

static void cwj_complete_fn(struct request *rq,
		unsigned int nr_bytes)
{
}

const struct blk_integrity_profile cwj_reverse_mapping = {
	.name			= "CWJ-DIF-TYPE1-NON",
	.generate_fn		= cwj_generate_fn,
	.verify_fn		= cwj_verify_fn,
	.prepare_fn		= cwj_prepare_fn,
	.complete_fn		= cwj_complete_fn,
};
EXPORT_SYMBOL(cwj_reverse_mapping);
#endif