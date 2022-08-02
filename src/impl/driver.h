#pragma once

#include "driver/gpio.h"
#include "driver/timer.h"
#include "esp_dmx.h"
#include "esp_intr_alloc.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "hal/uart_hal.h"
#include "soc/uart_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DMX_CONTEX_INIT_DEF(uart_num)                              \
  {                                                                \
    .hal.dev = UART_LL_GET_HW(uart_num),                           \
    .spinlock = portMUX_INITIALIZER_UNLOCKED, .hw_enabled = false, \
  }

static uint64_t dmx_uid = 0;  // The 48-bit unique ID of this device.

/* This is the DMX driver object used to handle tx'ing and rx'ing DMX data on
the UART port. It stores all the information needed to run and analyze DMX
including the buffer used as an intermediary to store reads/writes on the UART
bus. */
typedef struct {
  dmx_port_t dmx_num;             // The driver's DMX port number.
  rst_seq_hw_t rst_seq_hw;        // The hardware being used to transmit the DMX reset sequence. Can be either the UART or an ESP32 timer group.
  timer_idx_t timer_idx;          // The timer index being used for the reset sequence.
  intr_handle_t uart_isr_handle;  // The handle to the DMX UART ISR.

  uint32_t break_len;       // Length in microseconds of the transmitted break.
  uint32_t mab_len;         // Length in microseconds of the transmitted mark-after-break;

  struct {
    uint16_t head;                        // The index of the current slot being either transmitted or received.
    uint16_t size;                        // The size of the outgoing data packet or the expected size of the incoming data packet.
    uint8_t buffer[DMX_MAX_PACKET_SIZE];  // The buffer that stores the DMX packet.
 
    uint8_t previous_type;  // The type of the previous data packet. If the previous packet was an RDM packet, this is equal to its command class.
    int64_t previous_uid;   // The destination UID of the previous packet. Is -1 if the previous packet was not RDM.
    int64_t previous_ts;    // The timestamp (in microseconds since boot) of the last slot of the previous data packet.
    bool sent_previous;     // Is true if this device sent the previous data packet.
  } data;

  uint8_t is_in_break;
  uint8_t is_receiving;
  uint8_t is_sending;
  uint8_t mode;

  TaskHandle_t task_waiting;  // The handle to a task that is waiting for data to be sent or received.
  SemaphoreHandle_t mux;      // The handle to the driver mutex which allows multi-threaded driver function calls.

  /* These variables are used when receiving DMX. */
  struct {
    gpio_num_t intr_io_num;       // The GPIO number of the DMX sniffer interrupt pin.
    
    /* The remaining variables are only used if the DMX sniffer is enabled.
    They are uninitialized until dmx_sniffer_enable is called. */
    int32_t break_len;            // Length in microseconds of the last received break. Is always -1 unless the DMX sniffer is enabled.
    int32_t mab_len;              // Length in microseconds of the last received mark-after-break. Is always -1 unless the DMX sniffer is enabled.
    int64_t last_pos_edge_ts;     // Timestamp of the last positive edge on the sniffer pin.
    int64_t last_neg_edge_ts;     // Timestamp of the last negative edge on the sniffer pin.
  } rx;
} dmx_driver_t;

static IRAM_ATTR dmx_driver_t *dmx_driver[DMX_NUM_MAX] = {0};

typedef struct {
    uart_hal_context_t hal;
    portMUX_TYPE spinlock;
    bool hw_enabled;
} dmx_context_t;

static dmx_context_t dmx_context[DMX_NUM_MAX] = {
    DMX_CONTEX_INIT_DEF(DMX_NUM_0),
    DMX_CONTEX_INIT_DEF(DMX_NUM_1),
#if DMX_NUM_MAX > 2
    DMX_CONTEX_INIT_DEF(DMX_NUM_2),
#endif
};

#ifdef __cplusplus
}
#endif
