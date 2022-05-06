// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2017 Red Hat, Inc.
 * Copyright (c) 2018-2021 Christoph Hellwig.
 */
#include <linux/module.h>
#include <linux/compiler.h>
#include <linux/fs.h>
#include <linux/iomap.h>
#include <linux/pagemap.h>
#include <linux/pagevec.h>

static loff_t iomap_seek_hole_iter(const struct iomap_iter *iter,
		loff_t *hole_pos)
{
	loff_t length = iomap_length(iter);

	switch (iter->iomap.type) {
	case IOMAP_UNWRITTEN:
		*hole_pos = mapping_seek_hole_data(iter->inode->i_mapping,
				iter->pos, iter->pos + length, SEEK_HOLE);
		if (*hole_pos == iter->pos + length)
			return length;
		return 0;
	case IOMAP_HOLE:
		*hole_pos = iter->pos;
		return 0;
	default:
		return length;
	}
}

loff_t
iomap_seek_hole(struct inode *inode, loff_t pos, const struct iomap_ops *ops)
{
	loff_t size = i_size_read(inode);
<<<<<<< HEAD
	loff_t ret;
=======
	struct iomap_iter iter = {
		.inode	= inode,
		.pos	= pos,
		.flags	= IOMAP_REPORT,
	};
	int ret;
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a

	/* Nothing to be found before or beyond the end of the file. */
	if (pos < 0 || pos >= size)
		return -ENXIO;

<<<<<<< HEAD
	while (offset < size) {
		ret = iomap_apply(inode, offset, size - offset, IOMAP_REPORT,
				  ops, &offset, iomap_seek_hole_actor);
		if (ret < 0)
			return ret;
		if (ret == 0)
			break;
		offset += ret;
	}

	return offset;
=======
	iter.len = size - pos;
	while ((ret = iomap_iter(&iter, ops)) > 0)
		iter.processed = iomap_seek_hole_iter(&iter, &pos);
	if (ret < 0)
		return ret;
	if (iter.len) /* found hole before EOF */
		return pos;
	return size;
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a
}
EXPORT_SYMBOL_GPL(iomap_seek_hole);

static loff_t iomap_seek_data_iter(const struct iomap_iter *iter,
		loff_t *hole_pos)
{
	loff_t length = iomap_length(iter);

	switch (iter->iomap.type) {
	case IOMAP_HOLE:
		return length;
	case IOMAP_UNWRITTEN:
		*hole_pos = mapping_seek_hole_data(iter->inode->i_mapping,
				iter->pos, iter->pos + length, SEEK_DATA);
		if (*hole_pos < 0)
			return length;
		return 0;
	default:
		*hole_pos = iter->pos;
		return 0;
	}
}

loff_t
iomap_seek_data(struct inode *inode, loff_t pos, const struct iomap_ops *ops)
{
	loff_t size = i_size_read(inode);
<<<<<<< HEAD
	loff_t ret;
=======
	struct iomap_iter iter = {
		.inode	= inode,
		.pos	= pos,
		.flags	= IOMAP_REPORT,
	};
	int ret;
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a

	/* Nothing to be found before or beyond the end of the file. */
	if (pos < 0 || pos >= size)
		return -ENXIO;

<<<<<<< HEAD
	while (offset < size) {
		ret = iomap_apply(inode, offset, size - offset, IOMAP_REPORT,
				  ops, &offset, iomap_seek_data_actor);
		if (ret < 0)
			return ret;
		if (ret == 0)
			return offset;
		offset += ret;
	}

=======
	iter.len = size - pos;
	while ((ret = iomap_iter(&iter, ops)) > 0)
		iter.processed = iomap_seek_data_iter(&iter, &pos);
	if (ret < 0)
		return ret;
	if (iter.len) /* found data before EOF */
		return pos;
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a
	/* We've reached the end of the file without finding data */
	return -ENXIO;
}
EXPORT_SYMBOL_GPL(iomap_seek_data);
