/*
 * canlib.c
 * Version 1.0
 *
 * Created on: Nov 9, 2021
 *     Author: aidan
 */

#include "../Inc/canlib.h"

// callback function pointers
static CANLib_RXCallback rx0_callback = NULL;
static CANLib_RXCallback rx1_callback = NULL;

int CANLib_Init(struct CANLib *can, FDCAN_HandleTypeDef *fdcan,
    const FDCAN_TxHeaderTypeDef *header)
{
  can->__fdcan = fdcan; // keep pointer
  can->tx_header = *header; // copy
  can->__next_filter = 0;

  rx0_callback = NULL;
  rx1_callback = NULL;

  return 0;
}

int CANLib_AddFilter(struct CANLib *can, uint32_t filt_type,
    uint32_t filt_mode, uint32_t filt_id1, uint32_t filt_id2)
{
  if (can->__next_filter >= can->__fdcan->Init.StdFiltersNbr)
    return 1; // filter count exceeded

  FDCAN_FilterTypeDef filter;
  filter.FilterConfig = filt_mode;
  filter.FilterType   = filt_type;
  filter.FilterIndex  = can->__next_filter;
  filter.IdType       = FDCAN_STANDARD_ID;
  filter.FilterID1    = filt_id1;
  filter.FilterID2    = filt_id2;

  if (HAL_FDCAN_ConfigFilter(can->__fdcan, &filter) != HAL_OK)
    return 1; // impossible unless HAL annotations are enabled

  return 0;
}

int CANLib_SetFilterMode(struct CANLib *can, uint32_t nonmatch_mode,
    uint32_t reject_remote)
{
  // NOTE: this automatically rejects all extended ID messages
  if (HAL_FDCAN_ConfigGlobalFilter(can->__fdcan, nonmatch_mode, FDCAN_REJECT,
          reject_remote, FDCAN_REJECT_REMOTE) != HAL_OK)
    return 1;

  return 0;
}

int CANLib_SetReceiveCallback(uint32_t fifo, CANLib_RXCallback cb)
{
  if (fifo == 0)      rx0_callback = cb;
  else if (fifo == 1) rx1_callback = cb;
  else return 1;

  return 0;
}

int CANLib_Start(struct CANLib *can)
{
  if (HAL_FDCAN_Start(can->__fdcan) != HAL_OK) return 1;

  return 0;
}

int CANLib_SendMsg(struct CANLib *can, uint32_t length_code, uint8_t *data)
{
  can->tx_header.DataLength = length_code;
  if (HAL_FDCAN_AddMessageToTxFifoQ(can->__fdcan, &can->tx_header, data) != HAL_OK)
    return 1;

  return 0;
}


static int recvMsg(FDCAN_HandleTypeDef *fdcan, uint32_t fifo,
    FDCAN_RxHeaderTypeDef *header, uint8_t *data)
{
  uint32_t rx_location = (fifo == 1) ? FDCAN_RX_FIFO1 : FDCAN_RX_FIFO0;

  if (HAL_FDCAN_GetRxMessage(fdcan, rx_location, header, data) != HAL_OK)
    return 1;

  return 0;
}

int CANLib_RecvMsg(struct CANLib *can, uint32_t fifo,
    FDCAN_RxHeaderTypeDef *header, uint8_t *data)
{
  return recvMsg(can->__fdcan, fifo, header, data);
}

// more callback shenanigans

int CANLib_CallRxCallback(FDCAN_HandleTypeDef *fdcan, uint32_t fifo, uint32_t it_flags)
{
  FDCAN_RxHeaderTypeDef header;
  uint8_t data[8];

  if (fifo == 0 && rx0_callback != NULL && (it_flags & FDCAN_IT_RX_FIFO0_NEW_MESSAGE))
  {
    if (recvMsg(fdcan, fifo, &header, data) != 0) return 1;

    rx0_callback(fdcan, &header, data);
  }
  else if (fifo == 1 && rx1_callback != NULL && (it_flags & FDCAN_IT_RX_FIFO1_NEW_MESSAGE))
  {
    if (recvMsg(fdcan, fifo, &header, data) != 0) return 1;

    rx1_callback(fdcan, &header, data);
  }

  return 0;
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
  CANLib_CallRxCallback(hfdcan, 0, RxFifo0ITs);
}

void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs)
{
  CANLib_CallRxCallback(hfdcan, 1, RxFifo1ITs);
}







