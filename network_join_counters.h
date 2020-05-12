/***************************************************************************//**
 * @file
 * @brief Tokens for the CLI password protection plugin.
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

//#include "cli-password-protection.h"

#ifndef COUNTERS_H
#define COUNTERS_H


#endif

/**
 * Cli password protction Tokens
 */
#define CREATOR_COUNTERS_TOTAL  (0x8020)
#define CREATOR_COUNTERS_SUCCESS         (0x8021)
#define CREATOR_COUNTERS_FAILURE  (0x8022)

#define NVM3KEY_COUNTERS_TOTAL  (NVM3KEY_DOMAIN_USER | 0x8020)
#define NVM3KEY_COUNTERS_SUCCESS        (NVM3KEY_DOMAIN_USER | 0x8021)
#define NVM3KEY_COUNTERS_FAILURE       (NVM3KEY_DOMAIN_USER | 0x8022)

#ifdef DEFINETYPES
// Include or define any typedef for tokens here
#endif  // DEFINETYPES

#ifdef DEFINETOKENS
// Define the actual token storage information here
DEFINE_BASIC_TOKEN(COUNTERS_TOTAL,
                   uint16_t,
                   0x00)
DEFINE_BASIC_TOKEN(COUNTERS_SUCCESS,
                   uint16_t,
                   0x00)
DEFINE_BASIC_TOKEN(COUNTERS_FAILURE,
                   uint16_t,
                   0x00)

#endif  // DEFINETOKENS
