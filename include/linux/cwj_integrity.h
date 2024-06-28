#ifndef CWJ_INTERGRITY_H
#define CWJ_INTERGRITY_H

#include <linux/types.h>

struct cwj_pi_tuple {//cwj_oob
	__be32 index;
    __be32 i_ino;
};
extern const struct blk_integrity_profile cwj_reverse_mapping;
#endif