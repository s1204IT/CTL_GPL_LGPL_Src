// SPDX-License-Identifier: GPL-2.0
#include "rpbuf_internal.h"

#include <linux/amp_timestamp.h>

void *g_rpbuf_amp_ts_dev;
EXPORT_SYMBOL(g_rpbuf_amp_ts_dev);

uint64_t trace_rpbuf_get_time(void)
{
	int ret = 0;
	uint64_t ts = 0;

	if (!g_rpbuf_amp_ts_dev)
		return 0;

	ret = amp_ts_get_timestamp(g_rpbuf_amp_ts_dev, &ts);
	if (ret) {
		pr_err("amp_ts_get_timestamp failed, ret: %d", ret);
	}

	return ts;
}
EXPORT_SYMBOL(trace_rpbuf_get_time);

void put_trace_rpbuf_time(struct rpbuf_buffer *buffer, int idx, uint64_t time)
{
	uint64_t *va;

	va = rpbuf_buffer_va(buffer) + rpbuf_buffer_len(buffer);

	va[idx] = time;
}
EXPORT_SYMBOL(put_trace_rpbuf_time);

void trace_rpbuf_time(struct rpbuf_buffer *buffer, int idx)
{
	uint64_t *va;

	va = rpbuf_buffer_va(buffer) + rpbuf_buffer_len(buffer);

	va[idx] = trace_rpbuf_get_time();
}
EXPORT_SYMBOL(trace_rpbuf_time);

int rpbuf_get_raw_perf_data(struct rpbuf_buffer *buffer, uint8_t *buf, uint32_t len)
{
	uint64_t *va;
	uint32_t target_len;

	if (!buffer || !buffer->controller || !buffer->controller->link || !buf || !len) {
		pr_err("invalid arguments\n");
		return -EINVAL;
	}

	target_len = (RPBUF_TRACE_END + 1) * sizeof(uint64_t);
	if (len != target_len) {
		pr_err("perf data buf length(%u) is not equal to %u", len, target_len);
		return -EINVAL;
	}

	va = rpbuf_buffer_va(buffer) + rpbuf_buffer_len(buffer);

	memcpy(buf, va, len);
	return 0;
}
EXPORT_SYMBOL(rpbuf_get_raw_perf_data);

int rpbuf_get_perf_data(struct rpbuf_buffer *buffer, rpbuf_perf_data_t *perf_data)
{
	int ret, i, j;
	uint64_t buf[RPBUF_TRACE_END + 1];

	if (((sizeof(perf_data->time_span) / sizeof(__u32)) != RPBUF_TRACE_END)
		|| (sizeof(perf_data->time_span) != sizeof(perf_data->timestamp))) {
		pr_err("rpbuf perf data definition is not correct!\n");
		return -EINVAL;
	}

	ret = rpbuf_get_raw_perf_data(buffer, (uint8_t *)buf, sizeof(buf));
	if (ret)
		return ret;

	perf_data->is_sync = (buffer->flags & BUFFER_SYNC_TRANSMIT) ? 1 : 0;
	perf_data->raw_timestamp = buf[RPBUF_TRACE_START];

	memset(perf_data->timestamp, 0, sizeof(perf_data->timestamp));
	for (i = RPBUF_TRACE_START + 1, j = 0; i <= RPBUF_TRACE_END; i++, j++) {
		if ((!perf_data->is_sync)
			&& ((i == RPBUF_TRACE_R_NOTIFY)
				|| (i == RPBUF_TRACE_R_S_CB)
				|| (i == RPBUF_TRACE_R_S_NOTIFY)
				|| (i == RPBUF_TRACE_R_FINISH))) {
			perf_data->timestamp[j] = buf[RPBUF_TRACE_WAIT_ACK];
			continue;
		}

		perf_data->timestamp[j] = buf[i] - buf[RPBUF_TRACE_START];
	}

	memset(perf_data->time_span, 0, sizeof(perf_data->time_span));
	for (i = RPBUF_TRACE_START + 1, j = 0; i <= RPBUF_TRACE_END; i++, j++) {
		if ((!perf_data->is_sync)
			&& ((i == RPBUF_TRACE_R_NOTIFY)
				|| (i == RPBUF_TRACE_R_S_CB)
				|| (i == RPBUF_TRACE_R_S_NOTIFY)
				|| (i == RPBUF_TRACE_R_FINISH))) {
			continue;
		}

		perf_data->time_span[j] = buf[i] - buf[i - 1];

		if ((!perf_data->is_sync) && (i == RPBUF_TRACE_END)) {
			perf_data->time_span[j] = buf[i] - buf[RPBUF_TRACE_WAIT_ACK];
		}
	}

	return 0;
}
EXPORT_SYMBOL(rpbuf_get_perf_data);

void rpbuf_dump_perf_data(const rpbuf_perf_data_t *perf_data)
{
	uint64_t quotient;
	uint32_t ts, time_span, remainder;

	quotient = div_u64_rem(perf_data->raw_timestamp, 1000, &remainder);
	pr_info("Mode: %s\n", perf_data->is_sync ? "Sync" : "Async");
	pr_info("Timestamp:\n");
	pr_info(">Start      : 0 us [%llu.%llu ms]\n", quotient, (uint64_t)remainder);

	ts = perf_data->timestamp[RPBUF_PERF_POINT_LOCAL_FLUSH];
	pr_info(">Flush cache : %u us\n", ts);

	ts = perf_data->timestamp[RPBUF_PERF_POINT_LOCAL_NOTIFY];
	pr_info(">Notify      : %u us\n", ts);

	ts = perf_data->timestamp[RPBUF_PERF_POINT_LOCAL_WAIT_ACK];
	pr_info(">Wait ACK    : %u us\n", ts);

	if (perf_data->is_sync) {
		ts = perf_data->timestamp[RPBUF_PERF_POINT_REMOTE_RECV];
		pr_info("  <Recv      : %u us\n", ts);

		ts = perf_data->timestamp[RPBUF_PERF_POINT_REMOTE_EXEC_CB];
		pr_info("  <Exec CB   : %u us\n", ts);

		ts = perf_data->timestamp[RPBUF_PERF_POINT_REMOTE_SEND_ACK];
		pr_info("  <Send ack  : %u us\n", ts);

		ts = perf_data->timestamp[RPBUF_PERF_POINT_REMOTE_FINISH];
		pr_info("  <Finish    : %u us\n", ts);
	}
	ts = perf_data->timestamp[RPBUF_PERF_POINT_LOCAL_FINISH];
	pr_info(">End         : %u us\n",  ts);

	pr_info("Time span:\n");

	time_span = perf_data->time_span[RPBUF_PERF_STAGE_LOCAL_PREPARE];
	pr_info(">Prepare     : %u us\n", time_span);

	time_span = perf_data->time_span[RPBUF_PERF_STAGE_LOCAL_FLUSH];
	pr_info(">Flush cache : %u us\n", time_span);

	time_span = perf_data->time_span[RPBUF_PERF_STAGE_LOCAL_NOTIFY];
	pr_info(">Notify     : %u us\n", time_span);

	if (perf_data->is_sync) {
		time_span = perf_data->time_span[RPBUF_PERF_STAGE_REMOTE_RECV];
		pr_info("  <Recv        : %u us\n", time_span);

		time_span = perf_data->time_span[RPBUF_PERF_STAGE_REMOTE_PREPARE];
		pr_info("  <Preprae     : %u us\n", time_span);

		time_span = perf_data->time_span[RPBUF_PERF_STAGE_REMOTE_EXEC_CB];
		pr_info("  <Exec CB     : %u us\n", time_span);

		time_span = perf_data->time_span[RPBUF_PERF_STAGE_REMOTE_SEND_ACK];
		pr_info("  <Send ACK    : %u us\n", time_span);
	}
	time_span = perf_data->time_span[RPBUF_PERF_STAGE_LOCAL_FINISH];
	pr_info(">Local finish: %u us\n",  time_span);

}
EXPORT_SYMBOL(rpbuf_dump_perf_data);
