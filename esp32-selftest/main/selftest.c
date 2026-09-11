/* =====================================================================
 * ESP32-S3 CAN 自测（软3 联调前的模块验证，IDF v6 新版 TWAI 驱动）
 *
 * 目的：单板验证 CAN 控制器 + TJA1050 收发器链路是好的，
 *       到学校拼 C8T6 双板联调时，问题域就只剩"两板之间的接线"。
 *
 * 两种模式（改 USE_TRANSCEIVER 重新编译）：
 *   USE_TRANSCEIVER 1  帧 出 TXD -> TJA1050 -> CANH/CANL -> TJA1050 -> RXD 回来
 *                      （需要接好收发器，CANH/CANL 之间接 120 欧终端电阻，绝不可直短）
 *   USE_TRANSCEIVER 0  片内回环，信号不出芯片（不接收发器也能跑，
 *                      用于先验证 ESP32 侧代码）
 *
 * 自测原理：enable_self_test = 1 -> 本节点发的帧不需要总线上别的节点应答 ACK，
 *           硬件会把帧收回来，触发 on_rx_done 回调。
 * ===================================================================== */

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"

#define TX_GPIO          4       /* S3 GPIO4 -> TJA1050 TXD（3.3V，TJA1050 输入阈值约 2V，直连） */
#define RX_GPIO          5       /* TJA1050 RXD -> 1k/2k 分压 -> S3 GPIO5（5V 不能直灌 3.3V 引脚） */
#define TEST_BAUDRATE    500000  /* 与 C8T6 双板工程一致的 500kbps */
#define TEST_ID          0x11
#define TEST_DATA        0xAA
#define N_TEST_FRAMES    5
#define USE_TRANSCEIVER  1       /* 1=过收发器实测；0=纯片内回环 */

#define PASS_COND(s_rx, s_ok) ((s_rx) == N_TEST_FRAMES && (s_ok) == N_TEST_FRAMES)

static twai_node_handle_t s_node;
static volatile int s_rx_count;     /* 收到的帧数 */
static volatile int s_rx_match;     /* 内容与发送完全一致的帧数 */
static uint8_t s_rx_buffer[8];

static bool on_rx_done(twai_node_handle_t handle,
                       const twai_rx_done_event_data_t *edata, void *user_ctx)
{
    (void)edata; (void)user_ctx;

    twai_frame_t rx = {
        .buffer = s_rx_buffer,
        .buffer_len = sizeof(s_rx_buffer),
    };
    if (twai_node_receive_from_isr(handle, &rx) != ESP_OK) {
        return false;
    }
    s_rx_count++;
    if (rx.header.id == TEST_ID && rx.header.dlc == 1 && s_rx_buffer[0] == TEST_DATA) {
        s_rx_match++;
    }
    return false;
}

void app_main(void)
{
    printf("ESP32-S3 CAN self-test  GPIO TX=%d RX=%d  %dkbps  mode=%s\n",
           TX_GPIO, RX_GPIO, TEST_BAUDRATE / 1000,
           USE_TRANSCEIVER ? "self-test via transceiver" : "internal loopback");

    twai_onchip_node_config_t node_cfg = {
        .io_cfg = {
            .tx = TX_GPIO,
            .rx = RX_GPIO,
            .quanta_clk_out = -1,
            .bus_off_indicator = -1,
        },
        .bit_timing = { .bitrate = TEST_BAUDRATE, .sp_permill = 750 },
        .fail_retry_cnt = -1,          /* 发送失败无限重试（自测模式不会有 ACK 错误） */
        .tx_queue_depth = 5,
        .intr_priority = 0,
        .flags = {
            .enable_self_test = 1,     /* 帧无需其他节点 ACK */
            .enable_loopback =
#if USE_TRANSCEIVER
                0,                     /* 真出引脚，穿过收发器再回来 */
#else
                1,                     /* 片内回环，不出引脚 */
#endif
        },
    };
    ESP_ERROR_CHECK(twai_new_node_onchip(&node_cfg, &s_node));

    twai_event_callbacks_t cbs = {
        .on_rx_done = on_rx_done,
    };
    ESP_ERROR_CHECK(twai_node_register_event_callbacks(s_node, &cbs, NULL));
    ESP_ERROR_CHECK(twai_node_enable(s_node));

    for (int i = 1; i <= N_TEST_FRAMES; i++) {
        uint8_t payload = TEST_DATA;
        twai_frame_t tx = {
            .header = {
                .id = TEST_ID,
                .dlc = 1,              /* 经典帧 DLC = 字节数 */
                .ide = 0,              /* 标准 11 位 ID，与 C8T6 工程一致 */
            },
            .buffer = &payload,
            .buffer_len = 1,
        };

        esp_err_t tx_err = twai_node_transmit(s_node, &tx, 100);
        if (tx_err != ESP_OK) {
            printf("[%d/%d] TX failed: %s\n", i, N_TEST_FRAMES, esp_err_to_name(tx_err));
            continue;
        }
        ESP_ERROR_CHECK(twai_node_transmit_wait_all_done(s_node, 100));
        vTaskDelay(pdMS_TO_TICKS(50));   /* 给 RX 回调到达的时间 */

        printf("[%d/%d] TX ID=0x11 DATA=0xAA -> rx=%d match=%d\n",
               i, N_TEST_FRAMES, s_rx_count, s_rx_match);
    }

    if (PASS_COND(s_rx_count, s_rx_match)) {
        printf("SELF-TEST PASS: %d/%d frames round-trip OK\n", s_rx_match, N_TEST_FRAMES);
    } else {
        printf("SELF-TEST FAIL: rx=%d match=%d of %d\n", s_rx_count, s_rx_match, N_TEST_FRAMES);
        printf("检查：TJA1050 5V 供电 / 共地 / RXD 分压 / CANH-CANL 间约 120 欧终端\n");
    }
}
