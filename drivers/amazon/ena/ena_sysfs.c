/*
 * Copyright 2015 Amazon.com, Inc. or its affiliates.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <linux/sysfs.h>
#include <linux/kernel.h>

#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>

#include "ena_netdev.h"
#include "ena_com.h"

#define to_ext_attr(x) container_of(x, struct dev_ext_attribute, attr)

static int ena_validate_small_copy_len(struct ena_adapter *adapter,
				       unsigned long len)
{
	if (len > adapter->netdev->mtu)
		return -EINVAL;

	return 0;
}

static ssize_t ena_store_small_copy_len(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t len)
{
	struct ena_adapter *adapter = dev_get_drvdata(dev);
	unsigned long small_copy_len;
	struct ena_ring *rx_ring;
	int err, i;

	err = kstrtoul(buf, 10, &small_copy_len);
	if (err < 0)
		return err;

	err = ena_validate_small_copy_len(adapter, small_copy_len);
	if (err)
		return err;

	rtnl_lock();
	adapter->small_copy_len = small_copy_len;

	for (i = 0; i < adapter->num_queues; i++) {
		rx_ring = &adapter->rx_ring[i];
		rx_ring->rx_small_copy_len = small_copy_len;
	}
	rtnl_unlock();

	return len;
}

static ssize_t ena_show_small_copy_len(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct ena_adapter *adapter = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", adapter->small_copy_len);
}

static struct device_attribute dev_attr_small_copy_len = {
	.attr = {.name = "small_copy_len", .mode = (S_IRUGO | S_IWUSR)},
	.show = ena_show_small_copy_len,
	.store = ena_store_small_copy_len,
};

/******************************************************************************
 *****************************************************************************/
int ena_sysfs_init(struct device *dev)
{
	if (device_create_file(dev, &dev_attr_small_copy_len))
		dev_info(dev, "failed to create small_copy_len sysfs entry");

	return 0;
}

/******************************************************************************
 *****************************************************************************/
void ena_sysfs_terminate(struct device *dev)
{
	device_remove_file(dev, &dev_attr_small_copy_len);
}
