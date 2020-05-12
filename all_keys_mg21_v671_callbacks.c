/***************************************************************************//**
 * @file
 * @brief
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
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

// This callback file is created for your convenience. You may add application
// code to this file. If you regenerate this file over a previous version, the
// previous version will be overwritten and any code you have added will be
// lost.

#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"
#include "app/framework/plugin/green-power-common/green-power-common.h"


#include "../../platform/emdrv/nvm3/inc/nvm3.h"
#include "../../platform/radio/rail_lib/common/rail.h"

#define RSSI_OFFSET -11

//#inlcude "../../platform/radio/rail_lib/common/rail_types.h"


#include EMBER_AF_API_NETWORK_CREATOR
#include EMBER_AF_API_NETWORK_CREATOR_SECURITY
#include EMBER_AF_API_NETWORK_STEERING
#include EMBER_AF_API_FIND_AND_BIND_TARGET
#include EMBER_AF_API_ZLL_PROFILE

#define LIGHT_ENDPOINT (1)

#define DELAY 1500

EmberEventControl commissioningLedEventControl;
EmberEventControl findingAndBindingEventControl;
EmberEventControl delayResetEventControl; 

static uint8_t needs_leave = 0;
static uint8_t needs_reset = 0;
static uint16_t rounds = 100;

void commissioningLedEventHandler(void)
{
  emberEventControlSetInactive(commissioningLedEventControl);

  if (emberAfNetworkState() == EMBER_JOINED_NETWORK) {
    uint16_t identifyTime;
    emberAfReadServerAttribute(LIGHT_ENDPOINT,
                               ZCL_IDENTIFY_CLUSTER_ID,
                               ZCL_IDENTIFY_TIME_ATTRIBUTE_ID,
                               (uint8_t *)&identifyTime,
                               sizeof(identifyTime));
    if (identifyTime > 0) {
      halToggleLed(COMMISSIONING_STATUS_LED);
      emberEventControlSetDelayMS(commissioningLedEventControl,
                                  LED_BLINK_PERIOD_MS << 1);
    } else {
      halSetLed(COMMISSIONING_STATUS_LED);
    }
  } else {
    EmberStatus status = emberAfPluginNetworkSteeringStart();
    emberAfCorePrintln("%p network %p: 0x%X", "Join", "start", status);
  }
}

void findingAndBindingEventHandler()
{
  if (emberAfNetworkState() == EMBER_JOINED_NETWORK) {
    emberEventControlSetInactive(findingAndBindingEventControl);
    emberAfCorePrintln("Find and bind target start: 0x%X",
                       emberAfPluginFindAndBindTargetStart(LIGHT_ENDPOINT));
  }
}

void delayResetEventHandler ()
{
    uint16_t val;

    emberEventControlSetInactive(delayResetEventControl);
    if (needs_leave){
        emberAfCorePrintln("Leave network");
        emberLeaveNetwork();
        needs_leave = 0;
        needs_reset = 1;
        //halReboot();
        emberEventControlSetDelayMS(delayResetEventControl,DELAY);
    }else if(needs_reset && (emberAfNetworkState() != EMBER_JOINED_NETWORK)) {
        needs_reset = 0;
        halReboot();
    }else{
        emberAfCorePrintln("network not down:%d",emberAfNetworkState());
        emberEventControlSetDelayMS(delayResetEventControl,DELAY);

        
        if (emberAfNetworkState() == EMBER_JOINED_NETWORK){
            needs_reset = 1;
            emberLeaveNetwork();
        }
        emberEventControlSetDelayMS(delayResetEventControl,DELAY);
    }
}

static void setZllState(uint16_t clear, uint16_t set)
{
  EmberTokTypeStackZllData token;
  emberZllGetTokenStackZllData(&token);
  token.bitmask &= ~clear;
  token.bitmask |= set;
  emberZllSetTokenStackZllData(&token);
}

/** @brief Stack Status
 *
 * This function is called by the application framework from the stack status
 * handler.  This callbacks provides applications an opportunity to be notified
 * of changes to the stack status and take appropriate action.  The return code
 * from this callback is ignored by the framework.  The framework will always
 * process the stack status after the callback returns.
 *
 * @param status   Ver.: always
 */
bool emberAfStackStatusCallback(EmberStatus status)
{
  // Make sure to change the ZLL factory new state based on whether or not
  // we are on a network.
  if (status == EMBER_NETWORK_DOWN) {
    halClearLed(COMMISSIONING_STATUS_LED);
    setZllState(0, (EMBER_ZLL_STATE_FACTORY_NEW | EMBER_ZLL_STATE_PROFILE_INTEROP));

    if (needs_reset){
        emberEventControlSetDelayMS(delayResetEventControl,DELAY);
    }
  } else if (status == EMBER_NETWORK_UP) {
    halSetLed(COMMISSIONING_STATUS_LED);
    setZllState(EMBER_ZLL_STATE_FACTORY_NEW, EMBER_ZLL_STATE_PROFILE_INTEROP);

    emberEventControlSetActive(findingAndBindingEventControl);
  }

  // This value is ignored by the framework.
  return false;
}

void adjustRssiOffset(int8_t offset)
{
  RAIL_Handle_t rail_handle = NULL;
  RAIL_RadioState_t state;
  RAIL_Status_t status = RAIL_STATUS_INVALID_PARAMETER;

  rail_handle = emberGetRailHandle();

//  RAIL_Idle(rail_handle,RAIL_IDLE_FORCE_SHUTDOWN_CLEAR_FLAGS,1);

  state = RAIL_GetRadioState(rail_handle);

  switch(state){
      case RAIL_RF_STATE_INACTIVE:
        emberAfCorePrintln("RAIL_RF_STATE_INACTIVE");
        break;
      case RAIL_RF_STATE_ACTIVE:
        emberAfCorePrintln("RAIL_RF_STATE_ACTIVE");
        break;
      case RAIL_RF_STATE_RX:
        emberAfCorePrintln("RAIL_RF_STATE_RX");
        break;

      case RAIL_RF_STATE_TX:
        emberAfCorePrintln("RAIL_RF_STATE_TX");
        break;


      case RAIL_RF_STATE_RX_ACTIVE:
        emberAfCorePrintln("RAIL_RF_STATE_RX_ACTIVE");
        break;

      case RAIL_RF_STATE_TX_ACTIVE:
        emberAfCorePrintln("RAIL_RF_STATE_TX_ACTIVE");
        break;

      default:
           emberAfCorePrintln("Unknown state");
           break;
  }

  if (rail_handle != NULL) {
      status = RAIL_SetRssiOffset(rail_handle, offset);


      if (status == RAIL_STATUS_NO_ERROR ) {
           emberAfCorePrintln("Set RSSI offset %d dBm error status %d.", offset, status);
      }else {
           emberAfCorePrintln("Get RAIL handle error.");
      }
   }

/*

  RAIL_Idle(rail_handle,RAIL_IDLE,1);
  state = RAIL_GetRadioState(rail_handle);

  switch(state){
      case RAIL_RF_STATE_INACTIVE:
        emberAfCorePrintln("RAIL_RF_STATE_INACTIVE");
        break;
      case RAIL_RF_STATE_ACTIVE:
        emberAfCorePrintln("RAIL_RF_STATE_ACTIVE");
        break;
      case RAIL_RF_STATE_RX:
        emberAfCorePrintln("RAIL_RF_STATE_RX");
        break;

      case RAIL_RF_STATE_TX:
        emberAfCorePrintln("RAIL_RF_STATE_TX");
        break;


      case RAIL_RF_STATE_RX_ACTIVE:
        emberAfCorePrintln("RAIL_RF_STATE_RX_ACTIVE");
        break;

      case RAIL_RF_STATE_TX_ACTIVE:
        emberAfCorePrintln("RAIL_RF_STATE_TX_ACTIVE");
        break;

      default:
           emberAfCorePrintln("Unknown state");
           break;
  }
*/

}



/** @brief Main Init
 *
 * This function is called from the application's main function. It gives the
 * application a chance to do any initialization required at system startup.
 * Any code that you would normally put into the top of the application's
 * main() routine should be put into this function.
        Note: No callback
 * in the Application Framework is associated with resource cleanup. If you
 * are implementing your application on a Unix host where resource cleanup is
 * a consideration, we expect that you will use the standard Posix system
 * calls, including the use of atexit() and handlers for signals such as
 * SIGTERM, SIGINT, SIGCHLD, SIGPIPE and so on. If you use the signal()
 * function to register your signal handler, please mind the returned value
 * which may be an Application Framework function. If the return value is
 * non-null, please make sure that you call the returned function from your
 * handler to avoid negating the resource cleanup of the Application Framework
 * itself.
 *
 */
void emberAfMainInitCallback(void)
{
    uint16_t val;

    needs_leave = 0;
    needs_reset = 0;
    adjustRssiOffset(RSSI_OFFSET);
    emberEventControlSetActive(commissioningLedEventControl);
}

/** @brief Complete
 *
 * This callback is fired when the Network Steering plugin is complete.
 *
 * @param status On success this will be set to EMBER_SUCCESS to indicate a
 * network was joined successfully. On failure this will be the status code of
 * the last join or scan attempt. Ver.: always
 * @param totalBeacons The total number of 802.15.4 beacons that were heard,
 * including beacons from different devices with the same PAN ID. Ver.: always
 * @param joinAttempts The number of join attempts that were made to get onto
 * an open Zigbee network. Ver.: always
 * @param finalState The finishing state of the network steering process. From
 * this, one is able to tell on which channel mask and with which key the
 * process was complete. Ver.: always
 */
void emberAfPluginNetworkSteeringCompleteCallback(EmberStatus status,
                                                  uint8_t totalBeacons,
                                                  uint8_t joinAttempts,
                                                  uint8_t finalState)
{
    uint16_t val; 
    emberAfCorePrintln("%p network %p: 0x%X", "Join", "complete", status);

  if ((status == EMBER_SUCCESS) || (status == EMBER_SECURITY_CONFIGURATION_INVALID)) {
        
        halCommonGetToken(&val, TOKEN_COUNTERS_SUCCESS);
        val++;
        halCommonSetToken(TOKEN_COUNTERS_SUCCESS,&val);

        halCommonGetToken(&val, TOKEN_COUNTERS_TOTAL);
        val++;
        halCommonSetToken(TOKEN_COUNTERS_TOTAL,&val);

        if (val < rounds){
            needs_reset = 1;
            needs_leave = 1;
            emberEventControlSetDelayMS(delayResetEventControl,DELAY);
            emberAfCorePrintln("start reset...");
        }

    }else{
        halCommonGetToken(&val, TOKEN_COUNTERS_FAILURE);
        val++;
        halCommonSetToken(TOKEN_COUNTERS_FAILURE,&val);

        halCommonGetToken(&val, TOKEN_COUNTERS_TOTAL);
        val++;
        halCommonSetToken(TOKEN_COUNTERS_TOTAL,&val);
        
        emberAfCorePrintln("check reset...");

        if (val < rounds){
            needs_leave = 0;
            needs_reset = 1;
            emberEventControlSetDelayMS(delayResetEventControl,500);
            emberAfCorePrintln("start reset...");
        }
    }


/*
    status = emberAfPluginNetworkCreatorStart(false); // distributed
    emberAfCorePrintln("%p network %p: 0x%X", "Form", "start", status);
    */
}


/** @brief Complete
 *
 * This callback notifies the user that the network creation process has
 * completed successfully.
 *
 * @param network The network that the network creator plugin successfully
 * formed. Ver.: always
 * @param usedSecondaryChannels Whether or not the network creator wants to
 * form a network on the secondary channels Ver.: always
 */
void emberAfPluginNetworkCreatorCompleteCallback(const EmberNetworkParameters *network,
                                                 bool usedSecondaryChannels)
{
  emberAfCorePrintln("%p network %p: 0x%X",
                     "Form distributed",
                     "complete",
                     EMBER_SUCCESS);
}

/** @brief Server Init
 *
 * On/off cluster, Server Init
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 */
void emberAfPluginOnOffClusterServerPostInitCallback(uint8_t endpoint)
{
  // At startup, trigger a read of the attribute and possibly a toggle of the
  // LED to make sure they are always in sync.
  emberAfOnOffClusterServerAttributeChangedCallback(endpoint,
                                                    ZCL_ON_OFF_ATTRIBUTE_ID);
}

/** @brief Server Attribute Changed
 *
 * On/off cluster, Server Attribute Changed
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 * @param attributeId Attribute that changed  Ver.: always
 */
void emberAfOnOffClusterServerAttributeChangedCallback(uint8_t endpoint,
                                                       EmberAfAttributeId attributeId)
{
  // When the on/off attribute changes, set the LED appropriately.  If an error
  // occurs, ignore it because there's really nothing we can do.
  if (attributeId == ZCL_ON_OFF_ATTRIBUTE_ID) {
    bool onOff;
    if (emberAfReadServerAttribute(endpoint,
                                   ZCL_ON_OFF_CLUSTER_ID,
                                   ZCL_ON_OFF_ATTRIBUTE_ID,
                                   (uint8_t *)&onOff,
                                   sizeof(onOff))
        == EMBER_ZCL_STATUS_SUCCESS) {
      if (onOff) {
        halSetLed(ON_OFF_LIGHT_LED);
      } else {
        halClearLed(ON_OFF_LIGHT_LED);
      }
    }
  }
}

/** @brief Hal Button Isr
 *
 * This callback is called by the framework whenever a button is pressed on the
 * device. This callback is called within ISR context.
 *
 * @param button The button which has changed state, either BUTTON0 or BUTTON1
 * as defined in the appropriate BOARD_HEADER.  Ver.: always
 * @param state The new state of the button referenced by the button parameter,
 * either ::BUTTON_PRESSED if the button has been pressed or ::BUTTON_RELEASED
 * if the button has been released.  Ver.: always
 */
void emberAfHalButtonIsrCallback(uint8_t button, uint8_t state)
{
  if (state == BUTTON_RELEASED) {
    emberEventControlSetActive(findingAndBindingEventControl);
  }
  static uint8_t comm_state, bp1_state;
  if (state == BUTTON_PRESSED) {
    if (button == BUTTON0) {
      if ( comm_state == 1) {
        // comm enter stop
        halClearLed(BOARDLED0);
        emberAfGreenPowerClusterGpSinkCommissioningModeCallback(0x00,    //options)
                                                                0xFFFF,    //addr
                                                                0xFFFF,    //addr
                                                                0xFF); //all endpoints
        comm_state = 1;
      } else {
        // comm enter start
        halSetLed(BOARDLED0);
        emberAfGreenPowerClusterGpSinkCommissioningModeCallback(0x09,    //options)
                                                                0xFFFF,    //addr
                                                                0xFFFF,    //addr
                                                                0xFF); //all endpoints
        comm_state = 0;
      }
    }
  }
}

void setSteeringCounters()
{
    uint16_t val;

    val = 0x0;
    halCommonSetToken(TOKEN_COUNTERS_TOTAL,&val);
    halCommonSetToken(TOKEN_COUNTERS_SUCCESS,&val);
    halCommonSetToken(TOKEN_COUNTERS_FAILURE,&val);
}
void getSteeringCounters()
{
    uint16_t val;

    halCommonGetToken(&val, TOKEN_COUNTERS_TOTAL);
    emberAfCorePrintln("Total: %4x",val);
    halCommonGetToken(&val, TOKEN_COUNTERS_SUCCESS);
    emberAfCorePrintln("Success: %4x",val);
    halCommonGetToken(&val, TOKEN_COUNTERS_FAILURE);
    emberAfCorePrintln("Failure: %4x",val);
}

void start()
{
    setSteeringCounters();    
    emberEventControlSetActive(commissioningLedEventControl);
}
uint8_t objects_in_nvm3 = 0;
extern nvm3_Handle_t *nvm3_defaultHandle;

void getObjectCounts()
{
    objects_in_nvm3=nvm3_countObjects(nvm3_defaultHandle);

    emberAfCorePrintln("total objects in NVM3:%d",objects_in_nvm3);
}

void getKeyList()
{
  uint32_t keys[4*objects_in_nvm3];
  uint16_t num;

  nvm3_enumObjects(nvm3_defaultHandle,keys,4*objects_in_nvm3,NVM3_KEY_MIN,NVM3_KEY_MAX);

  for(num=0;num<(4*objects_in_nvm3);num++){
     halResetWatchdog();
    emberAfCorePrintln("key:%4x",keys[num]);
  }
}

uint8_t * watch(uint8_t *value)
{
    return value;
}

void getObjectByKey()
{
  uint8_t  *value=NULL;
  uint32_t key;
  uint32_t object;
  uint32_t type;
  size_t len;

  uint8_t num;

  key = emberUnsignedCommandArgument(0);

  emberAfCorePrintln("key:%4x",key);

  nvm3_getObjectInfo(nvm3_defaultHandle,key,&type,&len);

  emberAfCorePrintln("object length:%2x",len);

  if (len > 0){

    value = malloc(len);

    watch(value);

    if (value !=  NULL){


        nvm3_readData(nvm3_defaultHandle,key,value,len);

        for(num=0;num<len;num++)
            emberAfCorePrintln("object:%x",value[num]);

        free(value);
    }
  }
 }



void getCcaThreshold()
{
    int8_t signal;

    signal = emRadioGetEdCcaThreshold();
    emberAfCorePrintln("Threshold:%d\n",signal);
}

extern RAIL_Handle_t emPhyRailHandle;

void setCcaThreshold()
{
    int8_t signal;
    RAIL_Handle_t handle;

    signal = emberSignedCommandArgument(0);

//  emRadioSetEdCcaThreshold(signal);

    handle = emberGetRailHandle();

//    handle = emPhyRailHandle;

    emberAfCorePrintln("Handle:%d",(uint32_t)handle);

//    RAIL_SetCcaThreshold(handle,signal);
    emRadioSetEdCcaThreshold(signal);
}

void getRailOffsetThreshold()
{
    RAIL_Handle_t handle;
    handle = emberGetRailHandle();
    emberAfCorePrintln("RSSI offset:%d",RAIL_GetRssiOffset(handle));
}

void setRssiOffsetThreshold(void)
{
  int8_t offset;
  RAIL_Handle_t handle;

  offset = emberUnsignedCommandArgument(0);
  handle = emberGetRailHandle();

  adjustRssiOffset(offset);
}

void getRssi()
{
    RAIL_Handle_t handle;
    handle = emberGetRailHandle();
    emberAfCorePrintln("RSSI offset:%d",RAIL_GetRssi(handle,true));
}

EmberCommandEntry emberAfCustomCommands[] = {
#ifdef EMBER_AF_ENABLE_CUSTOM_COMMANDS

  emberCommandEntryAction("set-steering-counters",
                          setSteeringCounters,
                          "",
                          "set steering counters"),

  emberCommandEntryAction("get-steering-counters",
                          getSteeringCounters,
                          "",
                          "get steering counters"),

  emberCommandEntryAction("start",
                          start,
                          "",
                          "start steering"),

  emberCommandEntryAction("get-object-counts",
                          getObjectCounts,
                          "",
                          "get object number from NVM3"),
  emberCommandEntryAction("get-object-by-key",
                          getObjectByKey,
                          "w",
                          "get object by key from NVM3"),
  emberCommandEntryAction("get-key-list",
                          getKeyList,
                          "w",
                          "get key list in NVM3"),

  emberCommandEntryAction("set-cca-threshold",
                          setCcaThreshold,
                          "s",
                          "get cca threshold"),

  emberCommandEntryAction("get-cca-threshold",
                          getCcaThreshold,
                          "",
                          "get cca threshold"),

  emberCommandEntryAction("get-Rail-Offset-threshold",
                          getRailOffsetThreshold,
                          "",
                          "get rssi offset threshold"),

  emberCommandEntryAction("set-Rail-Offset-threshold",
                          setRssiOffsetThreshold,
                          "s",
                          "set rssi offset"),

  emberCommandEntryAction("get-Rssi",
                          getRssi,
                          "",
                          "get Rssi"),




  #endif //ENABLE_CUSTOM_COMMANDS
  emberCommandEntryTerminator(),
};



