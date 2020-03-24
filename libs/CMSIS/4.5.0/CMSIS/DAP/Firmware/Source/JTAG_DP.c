/******************************************************************************
 * @file     JTAG_DP.c
 * @brief    CMSIS-DAP JTAG DP I/O
 * @version  V1.10
 * @date     20. May 2015
 *
 * @note
 * Copyright (C) 2012-2015 ARM Limited. All rights reserved.
 *
 * @par
 * ARM Limited (ARM) is supplying this software for use with Cortex-M
 * processor based microcontrollers.
 *
 * @par
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * ARM SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 ******************************************************************************/

#include "DAP_config.h"
#include "DAP.h"


// JTAG Macros

#define PIN_TCK_SET PIN_SWCLK_TCK_SET
#define PIN_TCK_CLR PIN_SWCLK_TCK_CLR
#define PIN_TMS_SET PIN_SWDIO_TMS_SET
#define PIN_TMS_CLR PIN_SWDIO_TMS_CLR

#define JTAG_CYCLE_TCK()                \
  PIN_TCK_CLR();                        \
  PIN_DELAY();                          \
  PIN_TCK_SET();                        \
  PIN_DELAY()

#define JTAG_CYCLE_TDI(tdi)             \
  PIN_TDI_OUT(tdi);                     \
  PIN_TCK_CLR();                        \
  PIN_DELAY();                          \
  PIN_TCK_SET();                        \
  PIN_DELAY()

#define JTAG_CYCLE_TDO(tdo)             \
  PIN_TCK_CLR();                        \
  PIN_DELAY();                          \
  tdo = PIN_TDO_IN();                   \
  PIN_TCK_SET();                        \
  PIN_DELAY()

#define JTAG_CYCLE_TDIO(tdi,tdo)        \
  PIN_TDI_OUT(tdi);                     \
  PIN_TCK_CLR();                        \
  PIN_DELAY();                          \
  tdo = PIN_TDO_IN();                   \
  PIN_TCK_SET();                        \
  PIN_DELAY()

#define PIN_DELAY() PIN_DELAY_SLOW(DAP_Data.clock_delay)


#if (DAP_JTAG != 0)


// Generate JTAG Sequence
//   info:   sequence information
//   tdi:    pointer to TDI generated data
//   tdo:    pointer to TDO captured data
//   return: none
void JTAG_Sequence (uint32_t info, const uint8_t *tdi, uint8_t *tdo) {
  uint32_t i_val;
  uint32_t o_val;
  uint32_t bit;
  uint32_t n, k;

  n = info & JTAG_SEQUENCE_TCK;
  if (n == 0U) { n = 64U; }

  if (info & JTAG_SEQUENCE_TMS) {
    PIN_TMS_SET();
  } else {
    PIN_TMS_CLR();
  }

  while (n) {
    i_val = *tdi++;
    o_val = 0U;
    for (k = 8U; k && n; k--, n--) {
      JTAG_CYCLE_TDIO(i_val, bit);
      i_val >>= 1;
      o_val >>= 1;
      o_val  |= bit << 7;
    }
    o_val >>= k;
    if (info & JTAG_SEQUENCE_TDO) {
      *tdo++ = (uint8_t)o_val;
    }
  }
}


// JTAG Set IR
//   ir:     IR value
//   return: none
#define JTAG_IR_Function(speed) /**/                                            \
void JTAG_IR_##speed (uint32_t ir) {                                            \
  uint32_t n;                                                                   \
                                                                                \
  PIN_TMS_SET();                                                                \
  JTAG_CYCLE_TCK();                         /* Select-DR-Scan */                \
  JTAG_CYCLE_TCK();                         /* Select-IR-Scan */                \
  PIN_TMS_CLR();                                                                \
  JTAG_CYCLE_TCK();                         /* Capture-IR */                    \
  JTAG_CYCLE_TCK();                         /* Shift-IR */                      \
                                                                                \
  PIN_TDI_OUT(1U);                                                              \
  for (n = DAP_Data.jtag_dev.ir_before[DAP_Data.jtag_dev.index]; n; n--) {      \
    JTAG_CYCLE_TCK();                       /* Bypass before data */            \
  }                                                                             \
  for (n = DAP_Data.jtag_dev.ir_length[DAP_Data.jtag_dev.index] - 1U; n; n--) { \
    JTAG_CYCLE_TDI(ir);                     /* Set IR bits (except last) */     \
    ir >>= 1;                                                                   \
  }                                                                             \
  n = DAP_Data.jtag_dev.ir_after[DAP_Data.jtag_dev.index];                      \
  if (n) {                                                                      \
    JTAG_CYCLE_TDI(ir);                     /* Set last IR bit */               \
    PIN_TDI_OUT(1U);                                                            \
    for (--n; n; n--) {                                                         \
      JTAG_CYCLE_TCK();                     /* Bypass after data */             \
    }                                                                           \
    PIN_TMS_SET();                                                              \
    JTAG_CYCLE_TCK();                       /* Bypass & Exit1-IR */             \
  } else {                                                                      \
    PIN_TMS_SET();                                                              \
    JTAG_CYCLE_TDI(ir);                     /* Set last IR bit & Exit1-IR */    \
  }                                                                             \
                                                                                \
  JTAG_CYCLE_TCK();                         /* Update-IR */                     \
  PIN_TMS_CLR();                                                                \
  JTAG_CYCLE_TCK();                         /* Idle */                          \
  PIN_TDI_OUT(1U);                                                              \
}


// JTAG Transfer I/O
//   request: A[3:2] RnW APnDP
//   data:    DATA[31:0]
//   return:  ACK[2:0]
#define JTAG_TransferFunction(speed)        /**/                                \
uint8_t JTAG_Transfer##speed (uint32_t request, uint32_t *data) {               \
  uint32_t ack;                                                                 \
  uint32_t bit;                                                                 \
  uint32_t val;                                                                 \
  uint32_t n;                                                                   \
                                                                                \
  PIN_TMS_SET();                                                                \
  JTAG_CYCLE_TCK();                         /* Select-DR-Scan */                \
  PIN_TMS_CLR();                                                                \
  JTAG_CYCLE_TCK();                         /* Capture-DR */                    \
  JTAG_CYCLE_TCK();                         /* Shift-DR */                      \
                                                                                \
  for (n = DAP_Data.jtag_dev.index; n; n--) {                                   \
    JTAG_CYCLE_TCK();                       /* Bypass before data */            \
  }                                                                             \
                                                                                \
  JTAG_CYCLE_TDIO(request >> 1, bit);       /* Set RnW, Get ACK.0 */            \
  ack  = bit << 1;                                                              \
  JTAG_CYCLE_TDIO(request >> 2, bit);       /* Set A2,  Get ACK.1 */            \
  ack |= bit << 0;                                                              \
  JTAG_CYCLE_TDIO(request >> 3, bit);       /* Set A3,  Get ACK.2 */            \
  ack |= bit << 2;                                                              \
                                                                                \
  if (ack != DAP_TRANSFER_OK) {                                                 \
    /* Exit on error */                                                         \
    PIN_TMS_SET();                                                              \
    JTAG_CYCLE_TCK();                       /* Exit1-DR */                      \
    goto exit;                                                                  \
  }                                                                             \
                                                                                \
  if (request & DAP_TRANSFER_RnW) {                                             \
    /* Read Transfer */                                                         \
    val = 0U;                                                                   \
    for (n = 31U; n; n--) {                                                     \
      JTAG_CYCLE_TDO(bit);                  /* Get D0..D30 */                   \
      val  |= bit << 31;                                                        \
      val >>= 1;                                                                \
    }                                                                           \
    n = DAP_Data.jtag_dev.count - DAP_Data.jtag_dev.index - 1U;                 \
    if (n) {                                                                    \
      JTAG_CYCLE_TDO(bit);                  /* Get D31 */                       \
      for (--n; n; n--) {                                                       \
        JTAG_CYCLE_TCK();                   /* Bypass after data */             \
      }                                                                         \
      PIN_TMS_SET();                                                            \
      JTAG_CYCLE_TCK();                     /* Bypass & Exit1-DR */             \
    } else {                                                                    \
      PIN_TMS_SET();                                                            \
      JTAG_CYCLE_TDO(bit);                  /* Get D31 & Exit1-DR */            \
    }                                                                           \
    val |= bit << 31;                                                           \
    if (data) { *data = val; }                                                  \
  } else {                                                                      \
    /* Write Transfer */                                                        \
    val = *data;                                                                \
    for (n = 31U; n; n--) {                                                     \
      JTAG_CYCLE_TDI(val);                  /* Set D0..D30 */                   \
      val >>= 1;                                                                \
    }                                                                           \
    n = DAP_Data.jtag_dev.count - DAP_Data.jtag_dev.index - 1u;                 \
    if (n) {                                                                    \
      JTAG_CYCLE_TDI(val);                  /* Set D31 */                       \
      for (--n; n; n--) {                                                       \
        JTAG_CYCLE_TCK();                   /* Bypass after data */             \
      }                                                                         \
      PIN_TMS_SET();                                                            \
      JTAG_CYCLE_TCK();                     /* Bypass & Exit1-DR */             \
    } else {                                                                    \
      PIN_TMS_SET();                                                            \
      JTAG_CYCLE_TDI(val);                  /* Set D31 & Exit1-DR */            \
    }                                                                           \
  }                                                                             \
                                                                                \
exit:                                                                           \
  JTAG_CYCLE_TCK();                         /* Update-DR */                     \
  PIN_TMS_CLR();                                                                \
  JTAG_CYCLE_TCK();                         /* Idle */                          \
  PIN_TDI_OUT(1U);                                                              \
                                                                                \
  /* Idle cycles */                                                             \
  n = DAP_Data.transfer.idle_cycles;                                            \
  while (n--) {                                                                 \
    JTAG_CYCLE_TCK();                       /* Idle */                          \
  }                                                                             \
                                                                                \
  return ((uint8_t)ack);                                                        \
}


#undef  PIN_DELAY
#define PIN_DELAY() PIN_DELAY_FAST()
JTAG_IR_Function(Fast);
JTAG_TransferFunction(Fast);

#undef  PIN_DELAY
#define PIN_DELAY() PIN_DELAY_SLOW(DAP_Data.clock_delay)
JTAG_IR_Function(Slow);
JTAG_TransferFunction(Slow);


// JTAG Read IDCODE register
//   return: value read
uint32_t JTAG_ReadIDCode (void) {
  uint32_t bit;
  uint32_t val;
  uint32_t n;

  PIN_TMS_SET();
  JTAG_CYCLE_TCK();                         /* Select-DR-Scan */
  PIN_TMS_CLR();
  JTAG_CYCLE_TCK();                         /* Capture-DR */
  JTAG_CYCLE_TCK();                         /* Shift-DR */

  for (n = DAP_Data.jtag_dev.index; n; n--) {
    JTAG_CYCLE_TCK();                       /* Bypass before data */
  }

  val = 0U;
  for (n = 31U; n; n--) {
    JTAG_CYCLE_TDO(bit);                    /* Get D0..D30 */
    val  |= bit << 31;
    val >>= 1;
  }
  PIN_TMS_SET();
  JTAG_CYCLE_TDO(bit);                      /* Get D31 & Exit1-DR */
  val |= bit << 31;

  JTAG_CYCLE_TCK();                         /* Update-DR */
  PIN_TMS_CLR();
  JTAG_CYCLE_TCK();                         /* Idle */

  return (val);
}


// JTAG Write ABORT register
//   data:   value to write
//   return: none
void JTAG_WriteAbort (uint32_t data) {
  uint32_t n;

  PIN_TMS_SET();
  JTAG_CYCLE_TCK();                         /* Select-DR-Scan */
  PIN_TMS_CLR();
  JTAG_CYCLE_TCK();                         /* Capture-DR */
  JTAG_CYCLE_TCK();                         /* Shift-DR */

  for (n = DAP_Data.jtag_dev.index; n; n--) {
    JTAG_CYCLE_TCK();                       /* Bypass before data */
  }

  PIN_TDI_OUT(0U);
  JTAG_CYCLE_TCK();                         /* Set RnW=0 (Write) */
  JTAG_CYCLE_TCK();                         /* Set A2=0 */
  JTAG_CYCLE_TCK();                         /* Set A3=0 */

  for (n = 31U; n; n--) {
    JTAG_CYCLE_TDI(data);                   /* Set D0..D30 */
    data >>= 1;
  }
  n = DAP_Data.jtag_dev.count - DAP_Data.jtag_dev.index - 1u;
  if (n) {
    JTAG_CYCLE_TDI(data);                   /* Set D31 */
    for (--n; n; n--) {
      JTAG_CYCLE_TCK();                     /* Bypass after data */
    }
    PIN_TMS_SET();
    JTAG_CYCLE_TCK();                       /* Bypass & Exit1-DR */
  } else {
    PIN_TMS_SET();
    JTAG_CYCLE_TDI(data);                   /* Set D31 & Exit1-DR */
  }

  JTAG_CYCLE_TCK();                         /* Update-DR */
  PIN_TMS_CLR();
  JTAG_CYCLE_TCK();                         /* Idle */
  PIN_TDI_OUT(1U);
}


// JTAG Set IR
//   ir:     IR value
//   return: none
void JTAG_IR (uint32_t ir) {
  if (DAP_Data.fast_clock) {
    JTAG_IR_Fast(ir);
  } else {
    JTAG_IR_Slow(ir);
  }
}


// JTAG Transfer I/O
//   request: A[3:2] RnW APnDP
//   data:    DATA[31:0]
//   return:  ACK[2:0]
uint8_t  JTAG_Transfer(uint32_t request, uint32_t *data) {
  if (DAP_Data.fast_clock) {
    return JTAG_TransferFast(request, data);
  } else {
    return JTAG_TransferSlow(request, data);
  }
}


#endif  /* (DAP_JTAG != 0) */
