// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015-2021, Linaro Limited
 * Copyright (c) 2016, EPAM Systems
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

<<<<<<< HEAD
#include <linux/arm-smccc.h>
=======
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a
#include <linux/crash_dump.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/tee_drv.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include "optee_private.h"

int optee_pool_op_alloc_helper(struct tee_shm_pool *pool, struct tee_shm *shm,
			       size_t size, size_t align,
			       int (*shm_register)(struct tee_context *ctx,
						   struct tee_shm *shm,
						   struct page **pages,
						   size_t num_pages,
						   unsigned long start))
{
<<<<<<< HEAD
	int rc;
	size_t n;
	struct tee_shm *shm;
	phys_addr_t pa;

	for (n = 0; n < num_params; n++) {
		struct tee_param *p = params + n;
		const struct optee_msg_param *mp = msg_params + n;
		u32 attr = mp->attr & OPTEE_MSG_ATTR_TYPE_MASK;

		switch (attr) {
		case OPTEE_MSG_ATTR_TYPE_NONE:
			p->attr = TEE_IOCTL_PARAM_ATTR_TYPE_NONE;
			memset(&p->u, 0, sizeof(p->u));
			break;
		case OPTEE_MSG_ATTR_TYPE_VALUE_INPUT:
		case OPTEE_MSG_ATTR_TYPE_VALUE_OUTPUT:
		case OPTEE_MSG_ATTR_TYPE_VALUE_INOUT:
			p->attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT +
				  attr - OPTEE_MSG_ATTR_TYPE_VALUE_INPUT;
			p->u.value.a = mp->u.value.a;
			p->u.value.b = mp->u.value.b;
			p->u.value.c = mp->u.value.c;
			break;
		case OPTEE_MSG_ATTR_TYPE_TMEM_INPUT:
		case OPTEE_MSG_ATTR_TYPE_TMEM_OUTPUT:
		case OPTEE_MSG_ATTR_TYPE_TMEM_INOUT:
			p->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT +
				  attr - OPTEE_MSG_ATTR_TYPE_TMEM_INPUT;
			p->u.memref.size = mp->u.tmem.size;
			shm = (struct tee_shm *)(unsigned long)
				mp->u.tmem.shm_ref;
			if (!shm) {
				p->u.memref.shm_offs = 0;
				p->u.memref.shm = NULL;
				break;
			}
			rc = tee_shm_get_pa(shm, 0, &pa);
			if (rc)
				return rc;
			p->u.memref.shm_offs = mp->u.tmem.buf_ptr - pa;
			p->u.memref.shm = shm;
			break;
		case OPTEE_MSG_ATTR_TYPE_RMEM_INPUT:
		case OPTEE_MSG_ATTR_TYPE_RMEM_OUTPUT:
		case OPTEE_MSG_ATTR_TYPE_RMEM_INOUT:
			p->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT +
				  attr - OPTEE_MSG_ATTR_TYPE_RMEM_INPUT;
			p->u.memref.size = mp->u.rmem.size;
			shm = (struct tee_shm *)(unsigned long)
				mp->u.rmem.shm_ref;

			if (!shm) {
				p->u.memref.shm_offs = 0;
				p->u.memref.shm = NULL;
				break;
			}
			p->u.memref.shm_offs = mp->u.rmem.offs;
			p->u.memref.shm = shm;
=======
	unsigned int order = get_order(size);
	struct page *page;
	int rc = 0;
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a

	/*
	 * Ignore alignment since this is already going to be page aligned
	 * and there's no need for any larger alignment.
	 */
	page = alloc_pages(GFP_KERNEL | __GFP_ZERO, order);
	if (!page)
		return -ENOMEM;

	shm->kaddr = page_address(page);
	shm->paddr = page_to_phys(page);
	shm->size = PAGE_SIZE << order;

	if (shm_register) {
		unsigned int nr_pages = 1 << order, i;
		struct page **pages;

		pages = kcalloc(nr_pages, sizeof(*pages), GFP_KERNEL);
		if (!pages) {
			rc = -ENOMEM;
			goto err;
		}

		for (i = 0; i < nr_pages; i++)
			pages[i] = page + i;

		rc = shm_register(shm->ctx, shm, pages, nr_pages,
				  (unsigned long)shm->kaddr);
		kfree(pages);
		if (rc)
			goto err;
	}

	return 0;

err:
	free_pages((unsigned long)shm->kaddr, order);
	return rc;
}

void optee_pool_op_free_helper(struct tee_shm_pool *pool, struct tee_shm *shm,
			       int (*shm_unregister)(struct tee_context *ctx,
						     struct tee_shm *shm))
{
	if (shm_unregister)
		shm_unregister(shm->ctx, shm);
	free_pages((unsigned long)shm->kaddr, get_order(shm->size));
	shm->kaddr = NULL;
}

static void optee_bus_scan(struct work_struct *work)
{
	WARN_ON(optee_enumerate_devices(PTA_CMD_GET_DEVICES_SUPP));
}

int optee_open(struct tee_context *ctx, bool cap_memref_null)
{
	struct optee_context_data *ctxdata;
	struct tee_device *teedev = ctx->teedev;
	struct optee *optee = tee_get_drvdata(teedev);

	ctxdata = kzalloc(sizeof(*ctxdata), GFP_KERNEL);
	if (!ctxdata)
		return -ENOMEM;

	if (teedev == optee->supp_teedev) {
		bool busy = true;

		mutex_lock(&optee->supp.mutex);
		if (!optee->supp.ctx) {
			busy = false;
			optee->supp.ctx = ctx;
		}
		mutex_unlock(&optee->supp.mutex);
		if (busy) {
			kfree(ctxdata);
			return -EBUSY;
		}

		if (!optee->scan_bus_done) {
			INIT_WORK(&optee->scan_bus_work, optee_bus_scan);
			optee->scan_bus_wq = create_workqueue("optee_bus_scan");
			if (!optee->scan_bus_wq) {
				kfree(ctxdata);
				return -ECHILD;
			}
			queue_work(optee->scan_bus_wq, &optee->scan_bus_work);
			optee->scan_bus_done = true;
		}
	}
	mutex_init(&ctxdata->mutex);
	INIT_LIST_HEAD(&ctxdata->sess_list);

	ctx->cap_memref_null = cap_memref_null;
	ctx->data = ctxdata;
	return 0;
}

static void optee_release_helper(struct tee_context *ctx,
				 int (*close_session)(struct tee_context *ctx,
						      u32 session))
{
	struct optee_context_data *ctxdata = ctx->data;
	struct optee_session *sess;
	struct optee_session *sess_tmp;

	if (!ctxdata)
		return;

<<<<<<< HEAD
	shm = tee_shm_alloc(ctx, sizeof(struct optee_msg_arg),
			    TEE_SHM_MAPPED | TEE_SHM_PRIV);
	if (!IS_ERR(shm)) {
		arg = tee_shm_get_va(shm, 0);
		/*
		 * If va2pa fails for some reason, we can't call into
		 * secure world, only free the memory. Secure OS will leak
		 * sessions and finally refuse more sessions, but we will
		 * at least let normal world reclaim its memory.
		 */
		if (!IS_ERR(arg))
			if (tee_shm_va2pa(shm, arg, &parg))
				arg = NULL; /* prevent usage of parg below */
	}

=======
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a
	list_for_each_entry_safe(sess, sess_tmp, &ctxdata->sess_list,
				 list_node) {
		list_del(&sess->list_node);
		close_session(ctx, sess->session_id);
		kfree(sess);
	}
	kfree(ctxdata);
	ctx->data = NULL;
}

void optee_release(struct tee_context *ctx)
{
	optee_release_helper(ctx, optee_close_session_helper);
}

void optee_release_supp(struct tee_context *ctx)
{
	struct optee *optee = tee_get_drvdata(ctx->teedev);

	optee_release_helper(ctx, optee_close_session_helper);
	if (optee->scan_bus_wq) {
		destroy_workqueue(optee->scan_bus_wq);
		optee->scan_bus_wq = NULL;
	}
	optee_supp_release(&optee->supp);
}

<<<<<<< HEAD
/* optee_remove - Device Removal Routine
 * @pdev: platform device information struct
 *
 * optee_remove is called by platform subsystem to alert the driver
 * that it should release the device
 */

static int optee_remove(struct platform_device *pdev)
{
	struct optee *optee = platform_get_drvdata(pdev);

	/* Unregister OP-TEE specific client devices on TEE bus */
	optee_unregister_devices();

	teedev_close_context(optee->ctx);
	/*
	 * Ask OP-TEE to free all cached shared memory objects to decrease
	 * reference counters and also avoid wild pointers in secure world
	 * into the old shared memory range.
	 */
	optee_disable_shm_cache(optee);
=======
void optee_remove_common(struct optee *optee)
{
	/* Unregister OP-TEE specific client devices on TEE bus */
	optee_unregister_devices();
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a

	optee_notif_uninit(optee);
	teedev_close_context(optee->ctx);
	/*
	 * The two devices have to be unregistered before we can free the
	 * other resources.
	 */
	tee_device_unregister(optee->supp_teedev);
	tee_device_unregister(optee->teedev);

	tee_shm_pool_free(optee->pool);
	optee_supp_uninit(&optee->supp);
	mutex_destroy(&optee->call_queue.mutex);
}

<<<<<<< HEAD
/* optee_shutdown - Device Removal Routine
 * @pdev: platform device information struct
 *
 * platform_shutdown is called by the platform subsystem to alert
 * the driver that a shutdown, reboot, or kexec is happening and
 * device must be disabled.
 */
static void optee_shutdown(struct platform_device *pdev)
{
	optee_disable_shm_cache(platform_get_drvdata(pdev));
}

static int optee_probe(struct platform_device *pdev)
{
	optee_invoke_fn *invoke_fn;
	struct tee_shm_pool *pool = ERR_PTR(-EINVAL);
	struct optee *optee = NULL;
	void *memremaped_shm = NULL;
	struct tee_device *teedev;
	struct tee_context *ctx;
	u32 sec_caps;
	int rc;

	/*
	 * The kernel may have crashed at the same time that all available
	 * secure world threads were suspended and we cannot reschedule the
	 * suspended threads without access to the crashed kernel's wait_queue.
	 * Therefore, we cannot reliably initialize the OP-TEE driver in the
	 * kdump kernel.
	 */
	if (is_kdump_kernel())
		return -ENODEV;

	invoke_fn = get_invoke_func(&pdev->dev);
	if (IS_ERR(invoke_fn))
		return PTR_ERR(invoke_fn);

	if (!optee_msg_api_uid_is_optee_api(invoke_fn)) {
		pr_warn("api uid mismatch\n");
		return -EINVAL;
	}

	optee_msg_get_os_revision(invoke_fn);

	if (!optee_msg_api_revision_is_compatible(invoke_fn)) {
		pr_warn("api revision mismatch\n");
		return -EINVAL;
	}

	if (!optee_msg_exchange_capabilities(invoke_fn, &sec_caps)) {
		pr_warn("capabilities mismatch\n");
		return -EINVAL;
	}

	/*
	 * Try to use dynamic shared memory if possible
	 */
	if (sec_caps & OPTEE_SMC_SEC_CAP_DYNAMIC_SHM)
		pool = optee_config_dyn_shm();
=======
static int smc_abi_rc;
static int ffa_abi_rc;
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a

static int optee_core_init(void)
{
	/*
	 * The kernel may have crashed at the same time that all available
	 * secure world threads were suspended and we cannot reschedule the
	 * suspended threads without access to the crashed kernel's wait_queue.
	 * Therefore, we cannot reliably initialize the OP-TEE driver in the
	 * kdump kernel.
	 */
	if (is_kdump_kernel())
		return -ENODEV;

	smc_abi_rc = optee_smc_abi_register();
	ffa_abi_rc = optee_ffa_abi_register();

<<<<<<< HEAD
	teedev = tee_device_alloc(&optee_desc, NULL, pool, optee);
	if (IS_ERR(teedev)) {
		rc = PTR_ERR(teedev);
		goto err;
	}
	optee->teedev = teedev;

	teedev = tee_device_alloc(&optee_supp_desc, NULL, pool, optee);
	if (IS_ERR(teedev)) {
		rc = PTR_ERR(teedev);
		goto err;
	}
	optee->supp_teedev = teedev;

	rc = tee_device_register(optee->teedev);
	if (rc)
		goto err;

	rc = tee_device_register(optee->supp_teedev);
	if (rc)
		goto err;

	mutex_init(&optee->call_queue.mutex);
	INIT_LIST_HEAD(&optee->call_queue.waiters);
	optee_wait_queue_init(&optee->wait_queue);
	optee_supp_init(&optee->supp);
	optee->memremaped_shm = memremaped_shm;
	optee->pool = pool;
	ctx = teedev_open(optee->teedev);
	if (IS_ERR(ctx)) {
		rc = PTR_ERR(ctx);
		goto err;
	}
	optee->ctx = ctx;

	/*
	 * Ensure that there are no pre-existing shm objects before enabling
	 * the shm cache so that there's no chance of receiving an invalid
	 * address during shutdown. This could occur, for example, if we're
	 * kexec booting from an older kernel that did not properly cleanup the
	 * shm cache.
	 */
	optee_disable_unmapped_shm_cache(optee);

	optee_enable_shm_cache(optee);

	if (optee->sec_caps & OPTEE_SMC_SEC_CAP_DYNAMIC_SHM)
		pr_info("dynamic shared memory is enabled\n");

	platform_set_drvdata(pdev, optee);

	rc = optee_enumerate_devices(PTA_CMD_GET_DEVICES);
	if (rc) {
		optee_remove(pdev);
		return rc;
	}

	pr_info("initialized driver\n");
=======
	/* If both failed there's no point with this module */
	if (smc_abi_rc && ffa_abi_rc)
		return smc_abi_rc;
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a
	return 0;
}
module_init(optee_core_init);

<<<<<<< HEAD
static const struct of_device_id optee_dt_match[] = {
	{ .compatible = "linaro,optee-tz" },
	{},
};
MODULE_DEVICE_TABLE(of, optee_dt_match);

static struct platform_driver optee_driver = {
	.probe  = optee_probe,
	.remove = optee_remove,
	.shutdown = optee_shutdown,
	.driver = {
		.name = "optee",
		.of_match_table = optee_dt_match,
	},
};
module_platform_driver(optee_driver);
=======
static void optee_core_exit(void)
{
	if (!smc_abi_rc)
		optee_smc_abi_unregister();
	if (!ffa_abi_rc)
		optee_ffa_abi_unregister();
}
module_exit(optee_core_exit);
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a

MODULE_AUTHOR("Linaro");
MODULE_DESCRIPTION("OP-TEE driver");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:optee");
