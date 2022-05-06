// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015-2021, Linaro Limited
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/tee_drv.h>
#include "optee_private.h"
#include "optee_rpc_cmd.h"

static void handle_rpc_func_cmd_get_time(struct optee_msg_arg *arg)
{
	struct timespec64 ts;

	if (arg->num_params != 1)
		goto bad;
	if ((arg->params[0].attr & OPTEE_MSG_ATTR_TYPE_MASK) !=
			OPTEE_MSG_ATTR_TYPE_VALUE_OUTPUT)
		goto bad;

	ktime_get_real_ts64(&ts);
	arg->params[0].u.value.a = ts.tv_sec;
	arg->params[0].u.value.b = ts.tv_nsec;

	arg->ret = TEEC_SUCCESS;
	return;
bad:
	arg->ret = TEEC_ERROR_BAD_PARAMETERS;
}

#if IS_REACHABLE(CONFIG_I2C)
static void handle_rpc_func_cmd_i2c_transfer(struct tee_context *ctx,
					     struct optee_msg_arg *arg)
{
<<<<<<< HEAD
=======
	struct optee *optee = tee_get_drvdata(ctx->teedev);
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a
	struct tee_param *params;
	struct i2c_adapter *adapter;
	struct i2c_msg msg = { };
	size_t i;
	int ret = -EOPNOTSUPP;
	u8 attr[] = {
		TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT,
		TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT,
		TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT,
		TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT,
	};

	if (arg->num_params != ARRAY_SIZE(attr)) {
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		return;
	}

	params = kmalloc_array(arg->num_params, sizeof(struct tee_param),
			       GFP_KERNEL);
	if (!params) {
		arg->ret = TEEC_ERROR_OUT_OF_MEMORY;
		return;
	}

	if (optee->ops->from_msg_param(optee, params, arg->num_params,
				       arg->params))
		goto bad;

	for (i = 0; i < arg->num_params; i++) {
		if (params[i].attr != attr[i])
			goto bad;
	}

	adapter = i2c_get_adapter(params[0].u.value.b);
	if (!adapter)
		goto bad;

<<<<<<< HEAD
	if (params[1].u.value.a & OPTEE_MSG_RPC_CMD_I2C_FLAGS_TEN_BIT) {
=======
	if (params[1].u.value.a & OPTEE_RPC_I2C_FLAGS_TEN_BIT) {
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a
		if (!i2c_check_functionality(adapter,
					     I2C_FUNC_10BIT_ADDR)) {
			i2c_put_adapter(adapter);
			goto bad;
		}

		msg.flags = I2C_M_TEN;
	}

	msg.addr = params[0].u.value.c;
	msg.buf  = params[2].u.memref.shm->kaddr;
	msg.len  = params[2].u.memref.size;

	switch (params[0].u.value.a) {
<<<<<<< HEAD
	case OPTEE_MSG_RPC_CMD_I2C_TRANSFER_RD:
		msg.flags |= I2C_M_RD;
		break;
	case OPTEE_MSG_RPC_CMD_I2C_TRANSFER_WR:
=======
	case OPTEE_RPC_I2C_TRANSFER_RD:
		msg.flags |= I2C_M_RD;
		break;
	case OPTEE_RPC_I2C_TRANSFER_WR:
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a
		break;
	default:
		i2c_put_adapter(adapter);
		goto bad;
	}

	ret = i2c_transfer(adapter, &msg, 1);

	if (ret < 0) {
		arg->ret = TEEC_ERROR_COMMUNICATION;
	} else {
		params[3].u.value.a = msg.len;
<<<<<<< HEAD
		if (optee_to_msg_param(arg->params, arg->num_params, params))
=======
		if (optee->ops->to_msg_param(optee, arg->params,
					     arg->num_params, params))
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a
			arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		else
			arg->ret = TEEC_SUCCESS;
	}

	i2c_put_adapter(adapter);
	kfree(params);
	return;
bad:
	kfree(params);
	arg->ret = TEEC_ERROR_BAD_PARAMETERS;
}
#else
static void handle_rpc_func_cmd_i2c_transfer(struct tee_context *ctx,
					     struct optee_msg_arg *arg)
{
	arg->ret = TEEC_ERROR_NOT_SUPPORTED;
}
#endif

static void handle_rpc_func_cmd_wq(struct optee *optee,
				   struct optee_msg_arg *arg)
{
	if (arg->num_params != 1)
		goto bad;

	if ((arg->params[0].attr & OPTEE_MSG_ATTR_TYPE_MASK) !=
			OPTEE_MSG_ATTR_TYPE_VALUE_INPUT)
		goto bad;

	switch (arg->params[0].u.value.a) {
	case OPTEE_RPC_NOTIFICATION_WAIT:
		if (optee_notif_wait(optee, arg->params[0].u.value.b))
			goto bad;
		break;
	case OPTEE_RPC_NOTIFICATION_SEND:
		if (optee_notif_send(optee, arg->params[0].u.value.b))
			goto bad;
		break;
	default:
		goto bad;
	}

	arg->ret = TEEC_SUCCESS;
	return;
bad:
	arg->ret = TEEC_ERROR_BAD_PARAMETERS;
}

static void handle_rpc_func_cmd_wait(struct optee_msg_arg *arg)
{
	u32 msec_to_wait;

	if (arg->num_params != 1)
		goto bad;

	if ((arg->params[0].attr & OPTEE_MSG_ATTR_TYPE_MASK) !=
			OPTEE_MSG_ATTR_TYPE_VALUE_INPUT)
		goto bad;

	msec_to_wait = arg->params[0].u.value.a;

	/* Go to interruptible sleep */
	msleep_interruptible(msec_to_wait);

	arg->ret = TEEC_SUCCESS;
	return;
bad:
	arg->ret = TEEC_ERROR_BAD_PARAMETERS;
}

static void handle_rpc_supp_cmd(struct tee_context *ctx, struct optee *optee,
				struct optee_msg_arg *arg)
{
	struct tee_param *params;

	arg->ret_origin = TEEC_ORIGIN_COMMS;

	params = kmalloc_array(arg->num_params, sizeof(struct tee_param),
			       GFP_KERNEL);
	if (!params) {
		arg->ret = TEEC_ERROR_OUT_OF_MEMORY;
		return;
	}

	if (optee->ops->from_msg_param(optee, params, arg->num_params,
				       arg->params)) {
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		goto out;
	}

	arg->ret = optee_supp_thrd_req(ctx, arg->cmd, arg->num_params, params);

	if (optee->ops->to_msg_param(optee, arg->params, arg->num_params,
				     params))
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
out:
	kfree(params);
}

struct tee_shm *optee_rpc_cmd_alloc_suppl(struct tee_context *ctx, size_t sz)
{
	u32 ret;
	struct tee_param param;
	struct optee *optee = tee_get_drvdata(ctx->teedev);
	struct tee_shm *shm;

	param.attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;
	param.u.value.a = OPTEE_RPC_SHM_TYPE_APPL;
	param.u.value.b = sz;
	param.u.value.c = 0;

	ret = optee_supp_thrd_req(ctx, OPTEE_RPC_CMD_SHM_ALLOC, 1, &param);
	if (ret)
		return ERR_PTR(-ENOMEM);

	mutex_lock(&optee->supp.mutex);
	/* Increases count as secure world doesn't have a reference */
	shm = tee_shm_get_from_id(optee->supp.ctx, param.u.value.c);
	mutex_unlock(&optee->supp.mutex);
	return shm;
}

<<<<<<< HEAD
static void handle_rpc_func_cmd_shm_alloc(struct tee_context *ctx,
					  struct optee *optee,
					  struct optee_msg_arg *arg,
					  struct optee_call_ctx *call_ctx)
{
	phys_addr_t pa;
	struct tee_shm *shm;
	size_t sz;
	size_t n;

	arg->ret_origin = TEEC_ORIGIN_COMMS;

	if (!arg->num_params ||
	    arg->params[0].attr != OPTEE_MSG_ATTR_TYPE_VALUE_INPUT) {
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		return;
	}

	for (n = 1; n < arg->num_params; n++) {
		if (arg->params[n].attr != OPTEE_MSG_ATTR_TYPE_NONE) {
			arg->ret = TEEC_ERROR_BAD_PARAMETERS;
			return;
		}
	}

	sz = arg->params[0].u.value.b;
	switch (arg->params[0].u.value.a) {
	case OPTEE_MSG_RPC_SHM_TYPE_APPL:
		shm = cmd_alloc_suppl(ctx, sz);
		break;
	case OPTEE_MSG_RPC_SHM_TYPE_KERNEL:
		shm = tee_shm_alloc(optee->ctx, sz,
				    TEE_SHM_MAPPED | TEE_SHM_PRIV);
		break;
	default:
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		return;
	}

	if (IS_ERR(shm)) {
		arg->ret = TEEC_ERROR_OUT_OF_MEMORY;
		return;
	}

	if (tee_shm_get_pa(shm, 0, &pa)) {
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		goto bad;
	}

	sz = tee_shm_get_size(shm);

	if (tee_shm_is_registered(shm)) {
		struct page **pages;
		u64 *pages_list;
		size_t page_num;

		pages = tee_shm_get_pages(shm, &page_num);
		if (!pages || !page_num) {
			arg->ret = TEEC_ERROR_OUT_OF_MEMORY;
			goto bad;
		}

		pages_list = optee_allocate_pages_list(page_num);
		if (!pages_list) {
			arg->ret = TEEC_ERROR_OUT_OF_MEMORY;
			goto bad;
		}

		call_ctx->pages_list = pages_list;
		call_ctx->num_entries = page_num;

		arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_TMEM_OUTPUT |
				      OPTEE_MSG_ATTR_NONCONTIG;
		/*
		 * In the least bits of u.tmem.buf_ptr we store buffer offset
		 * from 4k page, as described in OP-TEE ABI.
		 */
		arg->params[0].u.tmem.buf_ptr = virt_to_phys(pages_list) |
			(tee_shm_get_page_offset(shm) &
			 (OPTEE_MSG_NONCONTIG_PAGE_SIZE - 1));
		arg->params[0].u.tmem.size = tee_shm_get_size(shm);
		arg->params[0].u.tmem.shm_ref = (unsigned long)shm;

		optee_fill_pages_list(pages_list, pages, page_num,
				      tee_shm_get_page_offset(shm));
	} else {
		arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_TMEM_OUTPUT;
		arg->params[0].u.tmem.buf_ptr = pa;
		arg->params[0].u.tmem.size = sz;
		arg->params[0].u.tmem.shm_ref = (unsigned long)shm;
	}

	arg->ret = TEEC_SUCCESS;
	return;
bad:
	tee_shm_free(shm);
}

static void cmd_free_suppl(struct tee_context *ctx, struct tee_shm *shm)
=======
void optee_rpc_cmd_free_suppl(struct tee_context *ctx, struct tee_shm *shm)
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a
{
	struct tee_param param;

	param.attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;
	param.u.value.a = OPTEE_RPC_SHM_TYPE_APPL;
	param.u.value.b = tee_shm_get_id(shm);
	param.u.value.c = 0;

	/*
	 * Match the tee_shm_get_from_id() in cmd_alloc_suppl() as secure
	 * world has released its reference.
	 *
	 * It's better to do this before sending the request to supplicant
	 * as we'd like to let the process doing the initial allocation to
	 * do release the last reference too in order to avoid stacking
	 * many pending fput() on the client process. This could otherwise
	 * happen if secure world does many allocate and free in a single
	 * invoke.
	 */
	tee_shm_put(shm);

	optee_supp_thrd_req(ctx, OPTEE_RPC_CMD_SHM_FREE, 1, &param);
}

void optee_rpc_cmd(struct tee_context *ctx, struct optee *optee,
		   struct optee_msg_arg *arg)
{
	switch (arg->cmd) {
	case OPTEE_RPC_CMD_GET_TIME:
		handle_rpc_func_cmd_get_time(arg);
		break;
	case OPTEE_RPC_CMD_NOTIFICATION:
		handle_rpc_func_cmd_wq(optee, arg);
		break;
	case OPTEE_RPC_CMD_SUSPEND:
		handle_rpc_func_cmd_wait(arg);
		break;
<<<<<<< HEAD
	case OPTEE_MSG_RPC_CMD_SHM_ALLOC:
		free_pages_list(call_ctx);
		handle_rpc_func_cmd_shm_alloc(ctx, optee, arg, call_ctx);
		break;
	case OPTEE_MSG_RPC_CMD_SHM_FREE:
		handle_rpc_func_cmd_shm_free(ctx, arg);
		break;
	case OPTEE_MSG_RPC_CMD_I2C_TRANSFER:
=======
	case OPTEE_RPC_CMD_I2C_TRANSFER:
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a
		handle_rpc_func_cmd_i2c_transfer(ctx, arg);
		break;
	default:
		handle_rpc_supp_cmd(ctx, optee, arg);
	}
}

<<<<<<< HEAD
/**
 * optee_handle_rpc() - handle RPC from secure world
 * @ctx:	context doing the RPC
 * @param:	value of registers for the RPC
 * @call_ctx:	call context. Preserved during one OP-TEE invocation
 *
 * Result of RPC is written back into @param.
 */
void optee_handle_rpc(struct tee_context *ctx, struct optee_rpc_param *param,
		      struct optee_call_ctx *call_ctx)
{
	struct tee_device *teedev = ctx->teedev;
	struct optee *optee = tee_get_drvdata(teedev);
	struct tee_shm *shm;
	phys_addr_t pa;

	switch (OPTEE_SMC_RETURN_GET_RPC_FUNC(param->a0)) {
	case OPTEE_SMC_RPC_FUNC_ALLOC:
		shm = tee_shm_alloc(optee->ctx, param->a1,
				    TEE_SHM_MAPPED | TEE_SHM_PRIV);
		if (!IS_ERR(shm) && !tee_shm_get_pa(shm, 0, &pa)) {
			reg_pair_from_64(&param->a1, &param->a2, pa);
			reg_pair_from_64(&param->a4, &param->a5,
					 (unsigned long)shm);
		} else {
			param->a1 = 0;
			param->a2 = 0;
			param->a4 = 0;
			param->a5 = 0;
		}
		break;
	case OPTEE_SMC_RPC_FUNC_FREE:
		shm = reg_pair_to_ptr(param->a1, param->a2);
		tee_shm_free(shm);
		break;
	case OPTEE_SMC_RPC_FUNC_FOREIGN_INTR:
		/*
		 * A foreign interrupt was raised while secure world was
		 * executing, since they are handled in Linux a dummy RPC is
		 * performed to let Linux take the interrupt through the normal
		 * vector.
		 */
		break;
	case OPTEE_SMC_RPC_FUNC_CMD:
		shm = reg_pair_to_ptr(param->a1, param->a2);
		handle_rpc_func_cmd(ctx, optee, shm, call_ctx);
		break;
	default:
		pr_warn("Unknown RPC func 0x%x\n",
			(u32)OPTEE_SMC_RETURN_GET_RPC_FUNC(param->a0));
		break;
	}
=======
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a

