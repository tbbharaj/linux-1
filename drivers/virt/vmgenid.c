// SPDX-License-Identifier: GPL-2.0
/*
<<<<<<< HEAD
 * Virtual Machine Generation ID driver
 *
 * Copyright (C) 2018 Red Hat Inc. All rights reserved.
 *
 * Copyright (C) 2020 Amazon. All rights reserved.
 *
 *	Authors:
 *	  Adrian Catangiu <acatan@amazon.com>
 *	  Or Idgar <oridgar@gmail.com>
 *	  Gal Hammer <ghammer@redhat.com>
 *
 */
#include <linux/acpi.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/uuid.h>
#include <linux/sysgenid.h>

#define DEV_NAME "vmgenid"
ACPI_MODULE_NAME(DEV_NAME);

struct vmgenid_data {
	uuid_t uuid;
	void *uuid_iomap;
};
static struct vmgenid_data vmgenid_data;

static int vmgenid_acpi_map(struct vmgenid_data *priv, acpi_handle handle)
{
	int i;
	phys_addr_t phys_addr;
	struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
	acpi_status status;
	union acpi_object *pss;
	union acpi_object *element;

	status = acpi_evaluate_object(handle, "ADDR", NULL, &buffer);
=======
 * Copyright (C) 2022 Jason A. Donenfeld <Jason@zx2c4.com>. All Rights Reserved.
 *
 * The "Virtual Machine Generation ID" is exposed via ACPI and changes when a
 * virtual machine forks or is cloned. This driver exists for shepherding that
 * information to random.c.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/acpi.h>
#include <linux/random.h>

ACPI_MODULE_NAME("vmgenid");

enum { VMGENID_SIZE = 16 };

struct vmgenid_state {
	u8 *next_id;
	u8 this_id[VMGENID_SIZE];
};

static int vmgenid_add(struct acpi_device *device)
{
	struct acpi_buffer parsed = { ACPI_ALLOCATE_BUFFER };
	struct vmgenid_state *state;
	union acpi_object *obj;
	phys_addr_t phys_addr;
	acpi_status status;
	int ret = 0;

	state = devm_kmalloc(&device->dev, sizeof(*state), GFP_KERNEL);
	if (!state)
		return -ENOMEM;

	status = acpi_evaluate_object(device->handle, "ADDR", NULL, &parsed);
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a
	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status, "Evaluating ADDR"));
		return -ENODEV;
	}
<<<<<<< HEAD
	pss = buffer.pointer;
	if (!pss || pss->type != ACPI_TYPE_PACKAGE || pss->package.count != 2)
		return -EINVAL;

	phys_addr = 0;
	for (i = 0; i < pss->package.count; i++) {
		element = &(pss->package.elements[i]);
		if (element->type != ACPI_TYPE_INTEGER)
			return -EINVAL;
		phys_addr |= element->integer.value << i * 32;
	}

	priv->uuid_iomap = acpi_os_map_memory(phys_addr, sizeof(uuid_t));
	if (!priv->uuid_iomap) {
		pr_err("Could not map memory at 0x%llx, size %u\n",
			   phys_addr,
			   (u32) sizeof(uuid_t));
		return -ENOMEM;
	}

	memcpy_fromio(&priv->uuid, priv->uuid_iomap, sizeof(uuid_t));

	return 0;
}

static int vmgenid_acpi_add(struct acpi_device *device)
{
	int ret;

	if (!device)
		return -EINVAL;
	device->driver_data = &vmgenid_data;

	ret = vmgenid_acpi_map(device->driver_data, device->handle);
	if (ret < 0) {
		pr_err("vmgenid: failed to map acpi device\n");
		device->driver_data = NULL;
	}

	return ret;
}

static int vmgenid_acpi_remove(struct acpi_device *device)
{
	if (!device || acpi_driver_data(device) != &vmgenid_data)
		return -EINVAL;
	device->driver_data = NULL;

	if (vmgenid_data.uuid_iomap)
		acpi_os_unmap_memory(vmgenid_data.uuid_iomap, sizeof(uuid_t));
	vmgenid_data.uuid_iomap = NULL;

	return 0;
}

static void vmgenid_acpi_notify(struct acpi_device *device, u32 event)
{
	uuid_t old_uuid;

	if (!device || acpi_driver_data(device) != &vmgenid_data) {
		pr_err("VMGENID notify with unexpected driver private data\n");
		return;
	}

	/* update VM Generation UUID */
	old_uuid = vmgenid_data.uuid;
	memcpy_fromio(&vmgenid_data.uuid, vmgenid_data.uuid_iomap, sizeof(uuid_t));

	if (memcmp(&old_uuid, &vmgenid_data.uuid, sizeof(uuid_t))) {
		/* HW uuid updated */
		sysgenid_bump_generation();
		add_device_randomness(&vmgenid_data.uuid, sizeof(uuid_t));
	}
}

static const struct acpi_device_id vmgenid_ids[] = {
	{"VMGENID", 0},
	{"QEMUVGID", 0},
	{"", 0},
};

static struct acpi_driver acpi_vmgenid_driver = {
	.name = "vm_generation_id",
	.ids = vmgenid_ids,
	.owner = THIS_MODULE,
	.ops = {
		.add = vmgenid_acpi_add,
		.remove = vmgenid_acpi_remove,
		.notify = vmgenid_acpi_notify,
	}
};

static int __init vmgenid_init(void)
{
	return acpi_bus_register_driver(&acpi_vmgenid_driver);
}

static void __exit vmgenid_exit(void)
{
	acpi_bus_unregister_driver(&acpi_vmgenid_driver);
}

module_init(vmgenid_init);
module_exit(vmgenid_exit);

MODULE_AUTHOR("Adrian Catangiu");
MODULE_DESCRIPTION("Virtual Machine Generation ID");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
=======
	obj = parsed.pointer;
	if (!obj || obj->type != ACPI_TYPE_PACKAGE || obj->package.count != 2 ||
	    obj->package.elements[0].type != ACPI_TYPE_INTEGER ||
	    obj->package.elements[1].type != ACPI_TYPE_INTEGER) {
		ret = -EINVAL;
		goto out;
	}

	phys_addr = (obj->package.elements[0].integer.value << 0) |
		    (obj->package.elements[1].integer.value << 32);
	state->next_id = devm_memremap(&device->dev, phys_addr, VMGENID_SIZE, MEMREMAP_WB);
	if (IS_ERR(state->next_id)) {
		ret = PTR_ERR(state->next_id);
		goto out;
	}

	memcpy(state->this_id, state->next_id, sizeof(state->this_id));
	add_device_randomness(state->this_id, sizeof(state->this_id));

	device->driver_data = state;

out:
	ACPI_FREE(parsed.pointer);
	return ret;
}

static void vmgenid_notify(struct acpi_device *device, u32 event)
{
	struct vmgenid_state *state = acpi_driver_data(device);
	u8 old_id[VMGENID_SIZE];

	memcpy(old_id, state->this_id, sizeof(old_id));
	memcpy(state->this_id, state->next_id, sizeof(state->this_id));
	if (!memcmp(old_id, state->this_id, sizeof(old_id)))
		return;
	add_vmfork_randomness(state->this_id, sizeof(state->this_id));
}

static const struct acpi_device_id vmgenid_ids[] = {
	{ "VMGENCTR", 0 },
	{ "VM_GEN_COUNTER", 0 },
	{ }
};

static struct acpi_driver vmgenid_driver = {
	.name = "vmgenid",
	.ids = vmgenid_ids,
	.owner = THIS_MODULE,
	.ops = {
		.add = vmgenid_add,
		.notify = vmgenid_notify
	}
};

module_acpi_driver(vmgenid_driver);

MODULE_DEVICE_TABLE(acpi, vmgenid_ids);
MODULE_DESCRIPTION("Virtual Machine Generation ID");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jason A. Donenfeld <Jason@zx2c4.com>");
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a
