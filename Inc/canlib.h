/*
 * canlib.h
 * Version 1.0
 * STM32 HAL FDCAN wrapper library to make CAN programming a bit more sane.
 * How to use:
 * * Enable an FDCAN module in the IOC configuration
 *
 * For simplicity, this removes some features from the HAL driver:
 * * Only classic frames are supported, not FD frames (limiting the maximum length to 8 bytes).
 * * Only data frames can be sent, not remote frames.
 * * Extended CAN IDs are not supported (for now)
 *
 *  Created on: Oct 24, 2021
 *      Author: aidan
 */

#ifndef INC_CANLIB_H_
#define INC_CANLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "stm32g0xx_hal.h"

#define CANLIB_LENGTH_0B FDCAN_DLC_BYTES_0
#define CANLIB_LENGTH_1B FDCAN_DLC_BYTES_1
#define CANLIB_LENGTH_2B FDCAN_DLC_BYTES_2
#define CANLIB_LENGTH_3B FDCAN_DLC_BYTES_3
#define CANLIB_LENGTH_4B FDCAN_DLC_BYTES_4
#define CANLIB_LENGTH_5B FDCAN_DLC_BYTES_5
#define CANLIB_LENGTH_6B FDCAN_DLC_BYTES_6
#define CANLIB_LENGTH_7B FDCAN_DLC_BYTES_7
#define CANLIB_LENGTH_8B FDCAN_DLC_BYTES_8

// CANLib filter modes

// send filtered messages to FIFO 0
#define CANLIB_FILTER_TO_FIFO0    FDCAN_FILTER_TO_RXFIFO0
// send filtered messages to FIFO 1
#define CANLIB_FILTER_TO_FIFO1    FDCAN_FILTER_TO_RXFIFO1
// reject filtered messages
#define CANLIB_FILTER_REJECT      FDCAN_FILTER_REJECT
// send filtered messages to FIFO 0 and set high priority
#define CANLIB_FILTER_HP_TO_FIFO0 FDCAN_FILTER_TO_RXFIFO0_HP
// send filtered messages to FIFO 1 and set high priority
#define CANLIB_FILTER_HP_TO_FIFO1 FDCAN_FILTER_TO_RXFIFO1_HP

// CANLib filter types

// filter IDs between id1 and id2 (inclusive or exclusive, I don't know)
#define CANLIB_FILTER_1_TO_2         FDCAN_FILTER_RANGE
// filter IDs matching either id1 or id2
#define CANLIB_FILTER_1_OR_2         FDCAN_FILTER_DUAL
// filter IDs which match id1 after being masked with id2
#define CANLIB_FILTER_1_MASK_2       FDCAN_FILTER_MASK
// filter IDs between id1 and id2 without EIDM mask (whatever that means)
#define CANLIB_FILTER_1_TO_2_NO_EIDM FDCAN_FILTER_RANGE_NO_EIDM

// CANLib non-matching frame behavior

// send non-matching/remote messages to FIFO 0
#define CANLIB_NONMATCH_TO_FIFO0 FDCAN_ACCEPT_IN_RX_FIFO0
// send non-matching/remote messages to FIFO 1
#define CANLIB_NONMATCH_TO_FIFO1 FDCAN_ACCEPT_IN_RX_FIFO1
// reject non-matching/remote messages
#define CANLIB_NONMATCH_REJECT   FDCAN_REJECT

// RX callback
typedef void (*CANLib_RXCallback)(FDCAN_HandleTypeDef *, const FDCAN_RxHeaderTypeDef *, const uint8_t *);

/*
 * Structure containing the internal state of the CAN bus
 * Do not touch any of these values; use the proper CANLib
 * functions to change/read them.
 *
 * Except tx_header; feel free to change that one.
 */
struct CANLib
{
  FDCAN_HandleTypeDef *__fdcan;
  FDCAN_TxHeaderTypeDef tx_header;
  uint32_t __next_filter;
};

/*
 * Initialize the CAN bus internal structures, but don't start
 * transmitting/receiving data.
 * The TX header is copied into the internal data structure and can be
 * destroyed after CANLib_Init finishes. It is expected to be fully
 * initialized except for the data length code, which can be set in
 * CANLib_SendMsg. Future modifications to the other header fields can be
 * done by simply changing the values in the CANLib object's tx_header.
 *
 * Returns 0 on success and 1 on failure.
 */
int CANLib_Init(struct CANLib *can, FDCAN_HandleTypeDef *fdcan, const FDCAN_TxHeaderTypeDef *header);

/*
 * Add a filter to the list. There must be space to add this filter
 * (defined by StdFiltersNbr in the FDCAN IOC configuration), otherwise this function will fail.
 * Returns 0 on success; returns 1 if the filter list is full.
 */
int CANLib_AddFilter(struct CANLib *can, uint32_t filt_type, uint32_t filt_mode, uint32_t filt_id1, uint32_t filt_id2);

/*
 * Set the filtering mode for all messages to handle receiving non-matching and remote messages.
 * nonmatch_mode can be one of:
 * * CANLIB_NONMATCH_TO_FIFO0 -- route non-matching messages to FIFO 0
 * * CANLIB_NONMATCH_TO_FIFO1 -- route non-matching messages to FIFO 1
 * * CANLIB_NONMATCH_REJECT   -- reject non-matching messages
 * reject_remote can be one of:
 * * 0 -- filter remote frames
 * * 1 -- reject all remote frames
 */
int CANLib_SetFilterMode(struct CANLib *can, uint32_t nonmatch_mode, uint32_t reject_remote);

/*
 * Set the received-message callback for *all* CAN peripherals.
 * fifo can be one of the following:
 * 0 -- attach callback to RX FIFO 0
 * 1 -- attach callback to RX FIFO 1
 *
 * If multiple CAN peripherals are operating simultaneously, the callback
 * function will receive messages from both (with the appropriate FDCAN handle attached),
 * and it will need to distinguish between the two. Setting multiple callbacks
 * will simply overwrite the first one.
 *
 * If multiple CAN peripherals are being used, care should also be taken to
 * call CANLib_Init for both of them *before* setting the common receive
 * callback. Otherwise, it will be reset by a second call to CANLib_Init.
 */
int CANLib_SetReceiveCallback(uint32_t fifo, CANLib_RXCallback cb);

// TODO: more callbacks to handle:
//  * RX FIFO full/overflow
//  * TX message sent
//  * etc.

/*
 * Attempt to start the CAN bus, allowing transmission and reception of messages.
 * If this fails, the CANLib's error state is set.
 * Returns 0 on success and 1 on failure.
 */
int CANLib_Start(struct CANLib *can);

// TODO: add functions to manipulate/remove existing filters if necessary

/*
 * Attempt to send a message on the CAN bus. There *must* be as many bytes in the data buffer as are indicated
 * by the length_code value; otherwise, this function will read past the end of the buffer.
 *
 * This function will return 0 on success and 1 if any error occurs.
 */
int CANLib_SendMsg(struct CANLib *can, uint32_t length_code, uint8_t *data);

/*
 * Receive a message from the CAN bus (when polling rather than using the callback)
 *
 * * The message header will be written into header
 * * The message data will be written into the data array, which should be at least 8 bytes long.
 * * fifo should be either 0 (for FIFO 0) or 1 (for FIFO 1)
 *
 * Returns 0 on success and 1 if an error occurs.
 */
int CANLib_RecvMsg(struct CANLib *can, uint32_t fifo, FDCAN_RxHeaderTypeDef *header, uint8_t *data);

/*
 * Function to manually call the RX callback if necessary. Also used
 * internally by the callback handlers created by this library.
 * * fifo should be either 0 (for FIFO 0) or 1 (for FIFO 1)
 */
int CANLib_CallRxCallback(FDCAN_HandleTypeDef *fdcan, uint32_t fifo, uint32_t it_flags);

// TODO: stop function (if necessary)

// TODO: example code/documentation

#ifdef __cplusplus
}
#endif

#endif /* INC_CANLIB_H_ */
