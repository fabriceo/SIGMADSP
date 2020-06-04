/*
 * File:           \\VBOXSVR\VirtualBox_VMs\SigmaStudioProjects\ADAU1452\blink_1452\blink_1452_IC 1\blink_1452_IC_1_PARAM.h
 *
 * Created:        Thursday, June 04, 2020 11:28:56 AM
 * Description:    blink_1452:IC 1 parameter RAM definitions.
 *
 * This software is distributed in the hope that it will be useful,
 * but is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * This software may only be used to program products purchased from
 * Analog Devices for incorporation by you into audio products that
 * are intended for resale to audio product end users. This software
 * may not be distributed whole or in any part to third parties.
 *
 * Copyright ©2020 Analog Devices, Inc. All rights reserved.
 */
#ifndef __BLINK_1452_IC_1_PARAM_H__
#define __BLINK_1452_IC_1_PARAM_H__


/* Module SafeLoadModule - SafeLoadModule*/
#define MOD_SAFELOADMODULE_COUNT                       10
#define MOD_SAFELOADMODULE_DEVICE                      "IC1"
#define MOD_SAFELOADMODULE_DATA_SAFELOAD0_ADDR         24576
#define MOD_SAFELOADMODULE_DATA_SAFELOAD1_ADDR         24577
#define MOD_SAFELOADMODULE_DATA_SAFELOAD2_ADDR         24578
#define MOD_SAFELOADMODULE_DATA_SAFELOAD3_ADDR         24579
#define MOD_SAFELOADMODULE_DATA_SAFELOAD4_ADDR         24580
#define MOD_SAFELOADMODULE_ADDRESS_SAFELOAD_ADDR       24581
#define MOD_SAFELOADMODULE_NUM_SAFELOAD_ADDR           24582

/* Module Pulse1 - Pulse with dynamic duty cycle*/
#define MOD_PULSE1_COUNT                               4
#define MOD_PULSE1_DEVICE                              "IC1"
#define MOD_PULSE1_ALG0_ISROUNDED_ADDR                 20
#define MOD_PULSE1_ALG0_ISROUNDED_VALUE                SIGMASTUDIOTYPE_8_24_CONVERT(0)
#define MOD_PULSE1_ALG0_ISROUNDED_TYPE                 SIGMASTUDIOTYPE_8_24
#define MOD_PULSE1_ALG0_FREQ_STEP_ADDR                 21
#define MOD_PULSE1_ALG0_FREQ_STEP_VALUE                SIGMASTUDIOTYPE_8_24_CONVERT(0.000208333333333333)
#define MOD_PULSE1_ALG0_FREQ_STEP_TYPE                 SIGMASTUDIOTYPE_8_24
#define MOD_PULSE1_ALG0_ON_ADDR                        22
#define MOD_PULSE1_ALG0_ON_VALUE                       SIGMASTUDIOTYPE_8_24_CONVERT(1)
#define MOD_PULSE1_ALG0_ON_TYPE                        SIGMASTUDIOTYPE_8_24
#define MOD_PULSE1_ALG0_TH_ADDR                        23
#define MOD_PULSE1_ALG0_TH_VALUE                       SIGMASTUDIOTYPE_8_24_CONVERT(0.5)
#define MOD_PULSE1_ALG0_TH_TYPE                        SIGMASTUDIOTYPE_8_24

/* Module Pulse2 - Pulse with dynamic duty cycle*/
#define MOD_PULSE2_COUNT                               4
#define MOD_PULSE2_DEVICE                              "IC1"
#define MOD_PULSE2_ALG0_ISROUNDED_ADDR                 24
#define MOD_PULSE2_ALG0_ISROUNDED_VALUE                SIGMASTUDIOTYPE_8_24_CONVERT(0)
#define MOD_PULSE2_ALG0_ISROUNDED_TYPE                 SIGMASTUDIOTYPE_8_24
#define MOD_PULSE2_ALG0_FREQ_STEP_ADDR                 25
#define MOD_PULSE2_ALG0_FREQ_STEP_VALUE                SIGMASTUDIOTYPE_8_24_CONVERT(2.08333333333333E-05)
#define MOD_PULSE2_ALG0_FREQ_STEP_TYPE                 SIGMASTUDIOTYPE_8_24
#define MOD_PULSE2_ALG0_ON_ADDR                        26
#define MOD_PULSE2_ALG0_ON_VALUE                       SIGMASTUDIOTYPE_8_24_CONVERT(1)
#define MOD_PULSE2_ALG0_ON_TYPE                        SIGMASTUDIOTYPE_8_24
#define MOD_PULSE2_ALG0_TH_ADDR                        27
#define MOD_PULSE2_ALG0_TH_VALUE                       SIGMASTUDIOTYPE_8_24_CONVERT(0.5)
#define MOD_PULSE2_ALG0_TH_TYPE                        SIGMASTUDIOTYPE_8_24

/* Module READ_GPI6 - DSP Readback*/
#define MOD_READ_GPI6_COUNT                            1
#define MOD_READ_GPI6_DEVICE                           "IC1"
#define MOD_READ_GPI6_READBACKALGNEWSIGMA3001VALUE_ADDR 28
#define MOD_READ_GPI6_READBACKALGNEWSIGMA3001VALUE_VALUE SIGMASTUDIOTYPE_8_24_CONVERT(0)
#define MOD_READ_GPI6_READBACKALGNEWSIGMA3001VALUE_TYPE SIGMASTUDIOTYPE_8_24

#endif
