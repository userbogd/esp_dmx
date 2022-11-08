#pragma once

#include <stdint.h>

#include "dmx_types.h"
#include "driver/timer.h"
#include "esp_err.h"
#include "esp_intr_alloc.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "hal/uart_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The DMX driver object used to handle reading and writing DMX data on 
 * the UART port. It storese all the information needed to run and analyze DMX
 * and RDM.
 */
typedef __attribute__((aligned(4))) struct dmx_driver_t {
  dmx_port_t dmx_num;  // The driver's DMX port number.

  uart_dev_t *restrict dev;
  intr_handle_t uart_isr_handle;  // The handle to the DMX UART ISR.
#if ESP_IDF_MAJOR_VERSION >= 5
#error ESP-IDF v5 not supported yet!
  // TODO
#else
  timer_group_t timer_group;
  timer_idx_t timer_idx;
#endif

  uint32_t break_len;  // Length in microseconds of the transmitted break.
  uint32_t mab_len;    // Length in microseconds of the transmitted mark-after-break;

  struct dmx_data_t {
    int head;         // The index of the current slot being either transmitted or received.
    uint8_t *buffer;  // The buffer that stores the DMX packet.
    size_t tx_size;   // The size of the outgoing data packet.
    size_t rx_size;   // The expected size of the incoming data packet.

    int sent_last;      // True if the last packet was sent from this driver.
    int type;           // The type of the packet received.
    int64_t timestamp;  // The timestamp (in microseconds since boot) of the last slot of the previous data packet.

    esp_err_t err;  // The error state of the received DMX data.
  } data;

  int is_in_break;         // True if the driver is sending or receiving a DMX break.
  int received_a_packet;   // True if the driver is receiving data.
  int packet_was_handled;  // True if the latest packet has been handled by a call to dmx_receive()
  int is_sending;          // True if the driver is sending data.

  TaskHandle_t task_waiting;  // The handle to a task that is waiting for data to be sent or received.
  SemaphoreHandle_t mux;      // The handle to the driver mutex which allows multi-threaded driver function calls.

  struct rdm_info_t {
    rdm_uid_t uid;
    uint32_t tn;
    int discovery_is_muted;
  } rdm;

  struct dmx_sniffer_t {
    QueueHandle_t queue;       // The queue handle used to receive sniffer data.
    dmx_sniffer_data_t data;   // The data received by the DMX sniffer.

    int intr_pin;              // The GPIO number of the DMX sniffer interrupt pin.
    int is_in_mab;             // True if the sniffer is receiving a DMX mark-after-break.
    int64_t last_pos_edge_ts;  // Timestamp of the last positive edge on the sniffer pin.
    int64_t last_neg_edge_ts;  // Timestamp of the last negative edge on the sniffer pin.
  } sniffer;
} dmx_driver_t;

extern dmx_driver_t *dmx_driver[DMX_NUM_MAX];

#ifdef __cplusplus
}
#endif
