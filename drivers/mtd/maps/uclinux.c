/****************************************************************************/

/*
 *	uclinux.c -- generic memory mapped MTD driver for uclinux
 *
 *	(C) Copyright 2002, Greg Ungerer (gerg@snapgear.com)
 */
/* Version modifiee !! phef */
/****************************************************************************/

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <asm/io.h>
#include <asm/sections.h>
/****************************************************************************/

struct map_info uclinux_romfs_map = {
#ifdef CONFIG_MTD_UCLINUX_RELOCATE
	.name = "RAM",
	.phys = (unsigned long)_end,
#else
	.name = "ROM",
	.phys = (unsigned long)CONFIG_MTD_UCLINUX_PHYADDR,
#endif
	.size = 0,
};

static struct mtd_info *uclinux_romfs_mtdinfo;

/****************************************************************************/

static struct mtd_partition uclinux_romfs[] = {
	{ .name = "ROMfs" }
};

#define	NUM_PARTITIONS	ARRAY_SIZE(uclinux_romfs)

/****************************************************************************/

static int uclinux_point(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, void **virt, resource_size_t *phys)
{
	struct map_info *map = mtd->priv;
	*virt = map->virt + from;
	if (phys)
		*phys = map->phys + from;
	*retlen = len;
	return(0);
}

/****************************************************************************/

static int __init uclinux_mtd_init(void)
{
	struct mtd_info *mtd;
	struct map_info *mapp;

	mapp = &uclinux_romfs_map;
	if (!mapp->size)
		mapp->size = PAGE_ALIGN(ntohl(*((unsigned long *)(mapp->phys + 8))));
	mapp->bankwidth = 4;

#ifdef CONFIG_MTD_UCLINUX_RELOCATE
	printk("uclinux[mtd]: RAM probe address=0x%x size=0x%x\n",
#else
	printk("uclinux[mtd]: ROM probe address=0x%x size=0x%x\n",
#endif
		(int) mapp->phys, (int) mapp->size);

	mapp->virt = ioremap_nocache(mapp->phys, mapp->size);

	if (mapp->virt == 0) {
		printk("uclinux[mtd]: ioremap_nocache() failed\n");
		return(-EIO);
	}

	simple_map_init(mapp);

#ifdef CONFIG_MTD_UCLINUX_RELOCATE
	mtd = do_map_probe("map_ram", mapp);
#else
	mtd = do_map_probe("map_rom", mapp);
#endif
	if (!mtd) {
		printk("uclinux[mtd]: failed to find a mapping?\n");
		iounmap(mapp->virt);
		return(-ENXIO);
	}

	mtd->owner = THIS_MODULE;
	mtd->point = uclinux_point;
	mtd->priv = mapp;

	uclinux_romfs_mtdinfo = mtd;
#ifdef CONFIG_MTD_PARTITIONS
	add_mtd_partitions(mtd, uclinux_romfs, NUM_PARTITIONS);
#else
	add_mtd_device(mtd);
#endif

	return(0);
}

/****************************************************************************/

static void __exit uclinux_mtd_cleanup(void)
{
	if (uclinux_romfs_mtdinfo) {
#ifdef CONFIG_MTD_PARTITIONS
		del_mtd_partitions(uclinux_romfs_mtdinfo);
#else
		del_mtd_device(uclinux_romfs_mtdinfo);
#endif
		map_destroy(uclinux_romfs_mtdinfo);
		uclinux_romfs_mtdinfo = NULL;
	}
	if (uclinux_romfs_map.virt) {
		iounmap((void *) uclinux_romfs_map.virt);
		uclinux_romfs_map.virt = 0;
	}
}

/****************************************************************************/

module_init(uclinux_mtd_init);
module_exit(uclinux_mtd_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Greg Ungerer <gerg@snapgear.com>");
MODULE_DESCRIPTION("Generic RAM/ROM based MTD for uClinux");

/****************************************************************************/
