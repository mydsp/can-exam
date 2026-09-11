/*
 * ESP32-S3 双节点 CAN 预验收
 *
 * 同一份源码通过 CAN_ROLE=master/slave 生成两个固件：
 *   master: 发送 ID=0x11 DATA=0x11，等待 ID=0x22 DATA=0x22
 *   slave:  收到 ID=0x11 DATA=0x11，回复 ID=0x22 DATA=0x22
 *
 * 这里使用正常 CAN 模式，不使用单板 self-test/loopback，因而可以验证：
 * 两个 TWAI 控制器、两个 TJA1050、CANH/CANL 接线、终端电阻和 ACK。
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"

#define TX_GPIO       4
#define RX_GPIO       5
#define CAN_BITRATE   500000
#define CMD_ID        0x11
#define CMD_DATA      0x11
#define ACK_ID        0x22
#define ACK_DATA      0x22
#define TEST_FRAMES   5

#if !defined(CAN_ROLE_MASTER) && !defined(CAN_ROLE_SLAVE)
#error "Build with -D CAN_ROLE=master or -D CAN_ROLE=slave"
#endif

typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
} app_frame_t;

static twai_node_handle_t s_node;
static QueueHandle_t s_rx_queue;

static bool on_rx_done(twai_node_handle_t handle,
                       const twai_rx_done_event_data_t *edata,
                       void *user_ctx)
{
    (void)edata;
    (void)user_ctx;

    uint8_t buffer[8] = {0};
    twai_frame_t rx = {
        .buffer = buffer,
        .buffer_len = sizeof(buffer),
    };
    if (twai_node_receive_from_isr(handle, &rx) != ESP_OK) {
        return false;
    }

    app_frame_t frame = {0};
    frame.id = rx.header.id;
    frame.dlc = rx.header.dlc > 8 ? 8 : rx.header.dlc;
    memcpy(frame.data, buffer, frame.dlc);

    BaseType_t higher_priority_task_woken = pdFALSE;
    xQueueSendFromISR(s_rx_queue, &frame, &higher_priority_task_woken);
    return higher_priority_task_woken == pdTRUE;
}

static void send_frame(uint32_t id, uint8_t value)
{
    twai_frame_t tx = {
        .header = {
            .id = id,
            .dlc = 1,
            .ide = 0,
        },
        .buffer = &value,
        .buffer_len = 1,
    };

    esp_err_t err = twai_node_transmit(s_node, &tx, 200);
    if (err != ESP_OK) {
        printf("TX failed id=0x%02lX: %s\n", (unsigned long)id, esp_err_to_name(err));
        return;
    }
    err = twai_node_transmit_wait_all_done(s_node, 200);
    if (err != ESP_OK) {
        printf("TX not acknowledged id=0x%02lX: %s\n",
               (unsigned long)id, esp_err_to_name(err));
    }
}

static void init_can(void)
{
    twai_onchip_node_config_t node_cfg = {
        .io_cfg = {
            .tx = TX_GPIO,
            .rx = RX_GPIO,
            .quanta_clk_out = -1,
            .bus_off_indicator = -1,
        },
        .bit_timing = {
            .bitrate = CAN_BITRATE,
            .sp_permill = 750,
        },
        .fail_retry_cnt = 2,
        .tx_queue_depth = 4,
        .intr_priority = 0,
        .flags = {
            .enable_self_test = 0,
            .enable_loopback = 0,
        },
    };
    ESP_ERROR_CHECK(twai_new_node_onchip(&node_cfg, &s_node));

    twai_event_callbacks_t callbacks = {
        .on_rx_done = on_rx_done,
    };
    ESP_ERROR_CHECK(twai_node_register_event_callbacks(s_node, &callbacks, NULL));
    ESP_ERROR_CHECK(twai_node_enable(s_node));
}

#if defined(CAN_ROLE_MASTER)
static void run_master(void)
{
    for (;;) {
        int ack_count = 0;
        printf("MASTER TEST START: %d frames, 500kbps\n", TEST_FRAMES);

        for (int i = 1; i <= TEST_FRAMES; i++) {
            send_frame(CMD_ID, CMD_DATA);
            printf("TX [%d/%d] ID=0x11 DATA=0x11\n", i, TEST_FRAMES);

            TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(500);
            app_frame_t frame;
            while (xTaskGetTickCount() < deadline) {
                if (xQueueReceive(s_rx_queue, &frame, pdMS_TO_TICKS(50)) == pdTRUE) {
                    printf("RX ID=0x%02lX DATA=0x%02X\n",
                           (unsigned long)frame.id, (unsigned)frame.data[0]);
                    if (frame.id == ACK_ID && frame.dlc == 1 && frame.data[0] == ACK_DATA) {
                        ack_count++;
                        break;
                    }
                }
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        if (ack_count == TEST_FRAMES) {
            printf("DUAL CAN PASS: %d/%d ACK frames OK\n", ack_count, TEST_FRAMES);
        } else {
            printf("DUAL CAN FAIL: %d/%d ACK frames OK\n", ack_count, TEST_FRAMES);
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
#else
static void run_slave(void)
{
    app_frame_t frame;
    for (;;) {
        if (xQueueReceive(s_rx_queue, &frame, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        printf("RX ID=0x%02lX DATA=0x%02X\n",
               (unsigned long)frame.id, (unsigned)frame.data[0]);
        if (frame.id == CMD_ID && frame.dlc == 1 && frame.data[0] == CMD_DATA) {
            send_frame(ACK_ID, ACK_DATA);
            printf("TX ACK ID=0x22 DATA=0x22\n");
        }
    }
}
#endif

void app_main(void)
{
    s_rx_queue = xQueueCreate(8, sizeof(app_frame_t));
    if (s_rx_queue == NULL) {
        printf("RX queue allocation failed\n");
        return;
    }

#if defined(CAN_ROLE_MASTER)
    printf("ESP32-S3 CAN dual-node role=master TX=%d RX=%d bitrate=%d\n",
           TX_GPIO, RX_GPIO, CAN_BITRATE);
#else
    printf("ESP32-S3 CAN dual-node role=slave TX=%d RX=%d bitrate=%d\n",
           TX_GPIO, RX_GPIO, CAN_BITRATE);
#endif

    init_can();

#if defined(CAN_ROLE_MASTER)
    run_master();
#else
    run_slave();
#endif
}
