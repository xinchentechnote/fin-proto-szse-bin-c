/* Encode/decode benchmarks for the SZSE binary library, mirroring
 * bench/bench.zig in fin-proto-szse-bin-zig and examples/bench.rs in
 * fin-proto-szse-bin-rs: same methodology (10k warmup iterations, then 10
 * batches of 100k operations, fastest batch reported), so the numbers are
 * directly comparable.
 *
 * Output is one TSV line per case on stdout:
 *   lib \t lang \t op \t ns_per_op \t frame_bytes
 */
/* clock_gettime/CLOCK_MONOTONIC are POSIX; -std=c11 hides them otherwise */
#define _POSIX_C_SOURCE 199309L
#include "root.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define LIB "szse"
#define ITERS 100000
#define BATCHES 10
#define WARMUP 10000

static volatile uint64_t sink;

static double now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

static void report(const char *op, double best_ns, size_t bytes) {
    printf("%s\tc\t%s\t%.1f\t%zu\n", LIB, op, best_ns / (double)ITERS, bytes);
}

static void fill_new_order(szse_binary_new_order *msg) {
    memset(msg, 0, sizeof(*msg));
    msg->appl_id = FC_STR("010");
    msg->submitting_pbuid = FC_STR("PBU001");
    msg->security_id = FC_STR("600000");
    msg->security_id_source = FC_STR("101");
    msg->owner_type = 1;
    msg->clearing_firm = FC_STR("01");
    msg->transact_time = 20260109093000123LL;
    msg->user_info = FC_STR("user01");
    msg->cl_ord_id = FC_STR("ORD000001");
    msg->account_id = FC_STR("ACC000000001");
    msg->branch_id = FC_STR("0001");
    msg->order_restrictions = FC_STR("R");
    msg->side = FC_STR("1");
    msg->ord_type = FC_STR("2");
    msg->order_qty = 100;
    msg->price = 10500;
    msg->appl_extend_tag = NEW_ORDER_APPL_EXTEND_EXTEND_100101;
    msg->appl_extend.extend_100101.stop_px = -123456789;
    msg->appl_extend.extend_100101.min_qty = -123456789;
    msg->appl_extend.extend_100101.max_price_levels = 1234;
    msg->appl_extend.extend_100101.time_in_force = FC_STR("a");
    msg->appl_extend.extend_100101.cash_margin = FC_STR("a");
}

static size_t bench_encode(void) {
    szse_binary_new_order msg;
    fill_new_order(&msg);
    fc_buffer buf;
    fc_buffer_init(&buf, 0);
    for (int i = 0; i < WARMUP; i++) {
        fc_buffer_reset(&buf);
        szse_binary_new_order_encode(&msg, &buf);
    }
    double best = 1e18;
    for (int b = 0; b < BATCHES; b++) {
        double t0 = now_ns();
        for (int i = 0; i < ITERS; i++) {
            fc_buffer_reset(&buf);
            szse_binary_new_order_encode(&msg, &buf);
            sink += buf.len;
        }
        double d = now_ns() - t0;
        if (d < best) best = d;
    }
    size_t bytes = buf.len;
    report("encode", best, bytes);
    fc_buffer_free(&buf);
    return bytes;
}

static void bench_decode(const uint8_t *raw, size_t len) {
    uint64_t s = 0;
    for (int i = 0; i < WARMUP; i++) {
        fc_reader r;
        fc_reader_init(&r, raw, len, 0);
        szse_binary_new_order m;
        szse_binary_new_order_decode(&r, &m);
        s += (uint64_t)m.transact_time;
        szse_binary_new_order_free(&m);
    }
    double best = 1e18;
    for (int b = 0; b < BATCHES; b++) {
        double t0 = now_ns();
        for (int i = 0; i < ITERS; i++) {
            fc_reader r;
            fc_reader_init(&r, raw, len, 0);
            szse_binary_new_order m;
            szse_binary_new_order_decode(&r, &m);
            s += (uint64_t)m.transact_time;
            szse_binary_new_order_free(&m);
        }
        double d = now_ns() - t0;
        if (d < best) best = d;
    }
    sink += s;
    report("decode", best, len);
}

int main(void) {
    bench_encode();

    /* pre-encode one frame for the decode benchmark */
    szse_binary_new_order msg;
    fill_new_order(&msg);
    fc_buffer buf;
    fc_buffer_init(&buf, 0);
    if (!szse_binary_new_order_encode(&msg, &buf)) return 1;
    bench_decode(buf.data, buf.len);
    fc_buffer_free(&buf);

    if (sink == 1) fprintf(stderr, "\n");
    return 0;
}
