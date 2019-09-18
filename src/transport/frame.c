/*
 * Copyright (c) 2018 SignalWire, Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "signalwire-client-c/client.h"


static ks_status_t __to_json(swclt_frame_t *frame, ks_pool_t *pool, ks_json_t **json)
{
	if (!(*json = ks_json_pparse(pool, frame->data))) {
		ks_log(KS_LOG_WARNING, "Failed to parse json");
		return KS_STATUS_INVALID_ARGUMENT;
	}

	return KS_STATUS_SUCCESS;
}

static ks_status_t __to_json_lookup(swclt_frame_t *frame, ks_pool_t *pool, ks_json_t **json, uint32_t components, va_list args)
{
	ks_json_t *item = NULL;
	ks_status_t status = KS_STATUS_SUCCESS;
	ks_json_t *frame_json = NULL;

	if (status = __to_json(frame, pool, &frame_json)) {
		return status;
	}

	/* Do the lookup within our json object and duplicate it for them */
	if (!(item = ks_json_valookup(frame_json, components, args))) {
		status = KS_STATUS_NOT_FOUND;
		goto done;
	}

	/* Found it so now dupe it for them */
	if (!(*json = ks_json_pduplicate(pool, item, KS_TRUE))) {
		status = KS_STATUS_NO_MEM;
		goto done;
	}

done:
	ks_json_delete(&frame_json);
	return status;
}

static ks_status_t __copy_data(swclt_frame_t *frame, ks_pool_t *pool, void *data, ks_size_t len, kws_opcode_t opcode)
{
	if (!data || opcode == WSOC_INVALID) {
		ks_log(KS_LOG_ERROR, "Unable to copy data because: len = %lu, opcode = %d\n", len, opcode);
		return KS_STATUS_INVALID_ARGUMENT;
	}

	frame->len = 0;
	frame->opcode = -1;

	// Note kws returns a size that does not include the final null so account for that here
	if (len > 0) {
		if (frame->data) {
			if (!(frame->data = ks_pool_resize(frame->data, len + 1)))
				return KS_STATUS_NO_MEM;
		} else {
			if (!(frame->data = ks_pool_alloc(pool, len + 1)))
				return KS_STATUS_NO_MEM;
		}
		memcpy(frame->data, data, len + 1);
	}

	frame->opcode = opcode;
	frame->len = len;
	return KS_STATUS_SUCCESS;
}


/**
 * Allocates a frame, wrapped in a handle. A frame is a context used
 * for reading and writing into a kws socket.
 */
SWCLT_DECLARE(ks_status_t) swclt_frame_alloc(swclt_frame_t **frame, ks_pool_t *pool)
{
	if (frame) {
		*frame = ks_pool_alloc(pool, sizeof(swclt_frame_t));
		return KS_STATUS_SUCCESS;
	}
	return KS_STATUS_INVALID_ARGUMENT;
}

/**
 * swclt_frame_to_json - Converts the json in the frame, and returns a copy which
 * the owner must release.
 */
SWCLT_DECLARE(ks_status_t) swclt_frame_to_json(swclt_frame_t *frame, ks_pool_t *pool, ks_json_t **json)
{
	return __to_json(frame, pool, json);
}

SWCLT_DECLARE(ks_status_t) swclt_frame_to_json_lookup(swclt_frame_t *frame, ks_pool_t *pool, ks_json_t **json, int components, ...)
{
	ks_status_t status;
	va_list ap;
	va_start(ap, components);
	status = __to_json_lookup(frame, pool, json, components, ap);
	va_end(ap);
	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_frame_copy_data(swclt_frame_t *frame, ks_pool_t *pool, void *data, ks_size_t len, kws_opcode_t opcode)
{
	return __copy_data(frame, pool, data, len, opcode);
}

SWCLT_DECLARE(ks_status_t) swclt_frame_get_data(swclt_frame_t *frame, void **data, ks_size_t *len, kws_opcode_t *opcode)
{
	*data = frame->data;
	*len = frame->len;
	*opcode = frame->opcode;
	return KS_STATUS_SUCCESS;
}

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
