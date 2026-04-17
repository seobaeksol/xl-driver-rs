/*---------------------------------------------------------------------------------------
| File:
|   xlKlineDemo.cpp
| Project:
|   xlKlineDemo
|
| Description:
|   Sample for the XL Driver Library
|   Example application using K-Line and 'vxlapi.dll'
|
|-----------------------------------------------------------------------------
|-----------------------------------------------------------------------------
| Copyright (c) 2020 by Vector Informatik GmbH.  All rights reserved.
----------------------------------------------------------------------------*/

/* ============================================================================== */
/*                                                                                */
/*       HEADER INCLUDE SECTION                                                   */
/*                                                                                */
/* ============================================================================== */

#if defined(_Windows) || defined(_MSC_VER) || defined (__GNUC__)
 #define  STRICT
 #include <windows.h>
#endif

#include <stdio.h>
#ifndef DYNAMIC_XLDRIVER_DLL 
#define DYNAMIC_XLDRIVER_DLL 
#endif
#include "vxlapi.h"

/* ============================================================================== */
/*                                                                                */
/*       DEFINES / MACROS                                                         */
/*                                                                                */
/* ============================================================================== */

#ifdef _MSC_VER
#pragma warning(disable : 4996) // Either disable all deprecation warnings
#endif // VC8+


#define RECEIVE_EVENT_SIZE    1                // DO NOT EDIT! Currently 1 is supported only
#define MAX_CH               16                // We have MAX_CH for one port

#define FUNC_UART             0                // UART operation mode
#define FUNC_TESTER           1                // Tester operation mode
#define FUNC_ECUFI            2                // ECU operation mode (fast init)
#define FUNC_ECU5BD           3                // ECU opertaion mode (5Bd init)

#define ECU_ADDR           0x97                // ECU address during 5Bd init

/* ============================================================================== */
/*                                                                                */
/*      TYPE DEFINITIONS                                                          */
/*                                                                                */
/* ============================================================================== */

typedef struct {
  XLportHandle xlPortHandle;
  HANDLE       hMsgEvent;
} TStruct;

/* ============================================================================== */
/*                                                                                */
/*      LOCAL VARIABLES                                                           */
/*                                                                                */
/* ============================================================================== */


/* ============================================================================== */
/*                                                                                */
/*      GLOBAL VARIABLES                                                          */
/*                                                                                */
/* ============================================================================== */

char            g_AppName[XL_MAX_LENGTH + 1] = "xlKlineDemo";          //!< Application name which is displayed in VHWconf
XLportHandle    g_xlPhKline                  = XL_INVALID_PORTHANDLE;  //!< Global porthandle (we use only one!)
XLdriverConfig  g_xlDrvConfig;                                         //!< Contains the actual hardware configuration
XLaccess        g_xlCmKline                  = 0;                      //!< Global channelmask (includes all found channels)
XLaccess        g_xlCmSelKline[MAX_CH];                                //!< Global channelmask array (a single channel per entry)
int             g_KlineChCtr                 = 0;                      //!< Global counter of K-Line channels
unsigned int    g_BaudRate                   = 10400;                  //!< Default KLine baudrate
int             g_silent                     = 1;                      //!< flag to print the message events (on/off)
int             g_stop                       = 0;                      //!< flag to stop the application
unsigned int    g_TimerRate                  = 0;                      //!< Global timerrate (to toggle)
unsigned int    g_TesterResistor             = 0;                      //!< Flag to toggle tester resistor (default off)
unsigned int    g_activated                  = 0;                      //!< Global flag to indicate if the K-Line channels are active or not
unsigned int    g_HsMode                     = 0;                      //!< Global flag to toggle high speed mode (on/off)
unsigned int    g_Function                   = FUNC_UART;              //!< Current function: UART, Tester, ECU
// thread variables
XLhandle        g_hKlineMsgEvent;                                      //!< Notification handle for the receive queue
HANDLE          g_hRXThread;                                           //!< Thread handle (RX) (K-Line)
int             g_RXKlineThreadRun;                                    //!< Flag to start/stop the RX thread

/* ============================================================================== */
/*                                                                                */
/*      FUNCTION PROTOTYPES LOCAL FUNCTIONS                                       */
/*                                                                                */
/* ============================================================================== */

DWORD     WINAPI RxKlineThread(PVOID par);

static void     demoHelp(void);
static void     demoPrintConfig(void);
static void     demoToggleVerbose(void);
static void     demoToggleTesterResistor(XLaccess xlChanMask);
static void     demoToggleTimerRate();
static void     demoToggleHighspeedMode(XLaccess xlChanMask);
static void     demoResetClock();
static void     demoGenerateSyncPulse(XLaccess xlChanMask);
static XLstatus demoTransmit(XLaccess xlChanMask, unsigned char *buf, unsigned int len);
static XLstatus demoStartStop(void);
static XLstatus demoCreateRxThread(void);
static XLstatus exec5BdInitTester(XLaccess xlChanMaskTester);
static XLstatus execFastInitTester(XLaccess xlChanMaskTester);
static XLstatus demoActivateChannel(unsigned int func, XLaccess xlChanMask);
static XLstatus demoInitDriver(void);
static XLstatus demoInitKline(XLaccess xlChanMask);
static XLstatus prepareUart(XLaccess xlChanMask);
static XLstatus prepareTester(XLaccess xlChanMaskTester);
static XLstatus prepareEcu(XLaccess xlChanMaskEcu, unsigned int func);
static XLstatus demoCleanUp(void);
static void     demoUsage(void);

/* ============================================================================== */
/*                                                                                */
/*      LOCAL FUNCTION DEFINITIONS                                                */
/*                                                                                */
/* ============================================================================== */

//------------------------------------------------------------------------------
//   Name:           demoHelp                                   
//   Description:    Shows the program functionality
//------------------------------------------------------------------------------
static void demoHelp(void) {

  printf("\n----------------------------------------------------------\n");
  printf("-                  xlKlineDemo - HELP                    -\n");
  printf("----------------------------------------------------------\n");
  printf("- Keyboard commands:                                     -\n");
  printf("- 'v'      Toggle verbose mode                           -\n");
  printf("- 't'      Transmit a message                            -\n");
  printf("- 'T'      Transmit a long message                       -\n");
  printf("- 'i'      Switch tester resistor                        -\n");
  printf("- 'a'      Toggle timer rate                             -\n");
  printf("- 'r'      Reset clock                                   -\n");
  printf("- 'G'      Generate sync pulse - K-Line                  -\n");
  printf("- 'f'      Exec fastInit (only tester)                   -\n");
  printf("- '5'      Exec 5BD Init (only tester)                   -\n");
  printf("- 'H'      Switch highspeed mode                         -\n");
  printf("- 's'      Activate/deactivate channels                  -\n");
  printf("- 'p'      Show hardware configuration                   -\n");
  printf("- 'h'      Help                                          -\n");
  printf("- 'ESC'    Exit                                          -\n");
  printf("----------------------------------------------------------\n");
  printf("- 'PH'->PortHandle; 'CM'->ChannelMask; 'PM'->Permission  -\n");
  printf("----------------------------------------------------------\n\n");

}


//------------------------------------------------------------------------------
//   Name:           demoPrintConfig                                   
//   Description:    Shows the actual hardware configuration
//------------------------------------------------------------------------------
static void demoPrintConfig(void) {

  unsigned int i;
  char         str[XL_MAX_LENGTH + 1] = "";

  printf("----------------------------------------------------------\n");
  printf("- %02d channels       Hardware Configuration               -\n", g_xlDrvConfig.channelCount);
  printf("----------------------------------------------------------\n");

  for (i=0; i < g_xlDrvConfig.channelCount; i++) {

    printf("- Ch:%02d, CM:0x%3I64x,", 
      g_xlDrvConfig.channel[i].channelIndex, g_xlDrvConfig.channel[i].channelMask);
    printf(" %20s ", g_xlDrvConfig.channel[i].name);
    
    if (g_xlDrvConfig.channel[i].transceiverType != XL_TRANSCEIVER_TYPE_NONE) {
      strncpy( str, g_xlDrvConfig.channel[i].transceiverName, 13);
      printf(" %13s -\n", str);
    }
    else {
      printf(" no transceiver! -\n");
    }
    
  }

  printf("----------------------------------------------------------\n\n");
 
}


//------------------------------------------------------------------------------
//   Name:           demoToggleVerbose                                   
//   Description:    Switch verbose mode on/off
//------------------------------------------------------------------------------
static void demoToggleVerbose(void) {
  if (g_silent) { 
    g_silent = 0;
    printf("demoToggleVerbose, verbose mode ON\n"); 
  }
  else {
    g_silent = 1;
    printf("demoToggleVerbose, verbose mode OFF\n");
  }
} // demoToggleVerbose


//------------------------------------------------------------------------------
//   Name:           demoToggleTesterResistor                                   
//   Description:    Switch tester resistor on/off
//------------------------------------------------------------------------------
static void demoToggleTesterResistor(XLaccess xlChanMask) {
  XLstatus xlStatus;
  unsigned int resistor;

  if (g_TesterResistor) {
    resistor = XL_KLINE_TESTERRESISTOR_OFF;
  }
  else {
    resistor = XL_KLINE_TESTERRESISTOR_ON;
  }
  xlStatus = xlKlineSwitchTesterResistor(g_xlPhKline, xlChanMask, resistor);
  printf("demoToggleTesterResistor, xlKlineSwitchTesterResistor (CM:0x%I64x) result: %s\n", xlChanMask, xlGetErrorString(xlStatus));
  if (XL_SUCCESS == xlStatus) {
    g_TesterResistor = resistor;
  }
  printf("demoToggleTesterResistor, g_TesterResistor: %s\n", (XL_KLINE_TESTERRESISTOR_ON == g_TesterResistor) ? "ON" : "OFF");
} // demoToggleTesterResistor


//------------------------------------------------------------------------------
//   Name:           demoToggleTimerRate                                   
//   Description:    Toggle the rate of cyclic timer events 1ms/off
//------------------------------------------------------------------------------
static void demoToggleTimerRate() {
  XLstatus xlStatus;
  unsigned int rate;

  if (g_TimerRate) {
    rate = 0;
  }
  else {
    rate = 100;
  }
  xlStatus = xlSetTimerRate(g_xlPhKline, rate);
  printf("demoToggleTimerRate, xlSetTimerRate (timerRate:%d) result: %s\n", rate, xlGetErrorString(xlStatus));
  if (XL_SUCCESS == xlStatus) {
    g_TimerRate = rate;
  }
  printf("demoToggleTimerRate, g_TimerRate: %d\n", g_TimerRate);
} // demoToggleTimerRate


//------------------------------------------------------------------------------
//   Name:           demoToggleHighspeedMode                                   
//   Description:    Toggle high speed mode on/off
//------------------------------------------------------------------------------
static void demoToggleHighspeedMode(XLaccess xlChanMask) {
  XLstatus xlStatus;
  unsigned int mode;

  if (g_HsMode) {
    mode = XL_KLINE_TRXMODE_NORMAL;
  }
  else {
    mode = XL_KLINE_TRXMODE_HIGHSPEED;
  }
  xlStatus = xlKlineSwitchHighspeedMode(g_xlPhKline, xlChanMask, mode);
  printf("demoToggleHighspeedMode, xlKlineSwitchHighspeedMode (CM:0x%I64x) result: %s\n", xlChanMask, xlGetErrorString(xlStatus));
  if (XL_SUCCESS == xlStatus) {
    g_HsMode = mode;
  }
  printf("demoToggleHighspeedMode, g_HsMode: %s\n", (XL_KLINE_TRXMODE_HIGHSPEED == g_HsMode) ? "HIGHSPEED" : "NORMAL");
} // demoToggleHighspeedMode


//------------------------------------------------------------------------------
//   Name:           demoResetClock                                   
//   Description:    Reset clock of the K-Line port
//------------------------------------------------------------------------------
static void demoResetClock() {
  XLstatus xlStatus;

  if (XL_INVALID_PORTHANDLE != g_xlPhKline) {
    xlStatus = xlResetClock(g_xlPhKline);
    printf("demoResetClock, xlResetClock result: %s\n", xlGetErrorString(xlStatus));
  }
  else {
    printf("demoResetClock, invalid port handle!\n");
  }
} // demoResetClock


//------------------------------------------------------------------------------
//   Name:           demoGenerateSyncPulse                                   
//   Description:    Generate a sync pulse event on the selected channel(s)
//------------------------------------------------------------------------------
static void demoGenerateSyncPulse(XLaccess xlChanMask) {
  XLstatus xlStatus;

  xlStatus = xlGenerateSyncPulse(g_xlPhKline, xlChanMask);
  printf("demoGenerateSyncPulse, xlGenerateSyncPulse (CM:0x%I64x) result: %s\n", xlChanMask, xlGetErrorString(xlStatus));
} // demoGenerateSyncPulse


//------------------------------------------------------------------------------
//   Name:           demoTransmit                                   
//   Description:    Transmit a K-Line message
//------------------------------------------------------------------------------
static XLstatus demoTransmit(XLaccess xlChanMask, unsigned char *buf, unsigned int len) {
  XLstatus xlStatus;
  
  xlStatus = xlKlineTransmit(g_xlPhKline, xlChanMask, len, buf);
  printf("demoTransmit, xlKlineTransmit (CM:0x%I64x) result: %s\n", xlChanMask, xlGetErrorString(xlStatus));

  return xlStatus;
}


//------------------------------------------------------------------------------
//   Name:           demoStartStop                                   
//   Description:    Toggle channel activation/deactivation
//------------------------------------------------------------------------------
static XLstatus demoStartStop(void) {
  XLstatus xlStatus;

  if (XL_INVALID_PORTHANDLE != g_xlPhKline) {
    if (!g_activated) {
      xlStatus = xlActivateChannel(g_xlPhKline, g_xlCmKline, XL_BUS_TYPE_KLINE, XL_ACTIVATE_RESET_CLOCK);
      printf("demoStartStop, xlActivateChannel (PH:%d, CM:0x%I64x) result: %s\n", g_xlPhKline, g_xlCmKline, xlGetErrorString(xlStatus));
      if (XL_SUCCESS == xlStatus) {
        g_activated = 1;
      }
    }
    else {
      xlStatus = xlDeactivateChannel(g_xlPhKline, g_xlCmKline);
      printf("demoStartStop, xlDeactivateChannel (PH:%d, CM:0x%I64x) result: %s\n", g_xlPhKline, g_xlCmKline, xlGetErrorString(xlStatus));
      if (XL_SUCCESS == xlStatus) {
        g_activated = 0;
      }
    }
  }
  else {
    xlStatus = XL_ERR_INVALID_HANDLE;
    printf("demoStartStop, invalid port handle!\n");
  }

  return xlStatus;
}


//------------------------------------------------------------------------------
//   Name:           demoCreateRxThread                                   
//   Description:    Set event notification and create a receive thread
//------------------------------------------------------------------------------
static XLstatus demoCreateRxThread(void) {

  XLstatus        xlStatus = XL_ERROR;
  DWORD           ThreadId = 0;
  static TStruct  lTh;
 
  // create the K-Line thread
  if (XL_INVALID_PORTHANDLE != g_xlPhKline) {

      // Send a event for each Msg!!!
      xlStatus = xlSetNotification(g_xlPhKline, &g_hKlineMsgEvent, 1);

      lTh.xlPortHandle = g_xlPhKline;
      g_hRXThread = CreateThread(0, 0x1000, RxKlineThread, (LPVOID) &lTh, 0, &ThreadId);
      if (!g_hRXThread) {
        xlStatus = XL_ERROR;
        printf("demoCreateRxThread, CreateThread result: %s\n", xlGetErrorString(xlStatus));
      }
      else {
        xlStatus = XL_SUCCESS;
        printf("demoCreateRxThread, CreateThread result: %s\n", xlGetErrorString(xlStatus));
      }
  }
  else {
    xlStatus = XL_ERR_INVALID_HANDLE;
    printf("demoCreateRxThread, invalid port handle!\n");
  }

  return xlStatus;
}


//------------------------------------------------------------------------------
//   Name:           exec5BdInitTester                                   
//   Description:    Execute 5Bd init sequence (tester only)
//------------------------------------------------------------------------------
static XLstatus exec5BdInitTester(XLaccess xlChanMaskTester) {
  XLstatus          xlStatus;
  XLkline5BdTester  xlKline5BdTester;

  memset(&xlKline5BdTester, 0, sizeof(xlKline5BdTester));

  xlKline5BdTester.addr    = ECU_ADDR;
  xlKline5BdTester.rate5bd = 50;
  xlKline5BdTester.W1min   = 5000;    //  5ms
  xlKline5BdTester.W1max   = 50000;   // 50ms
  xlKline5BdTester.W2min   = 5000;    //  5ms
  xlKline5BdTester.W2max   = 7000;    //  7ms
  xlKline5BdTester.W3min   = 7000;    //  7ms
  xlKline5BdTester.W3max   = 9000;    //  9ms
  xlKline5BdTester.W4min   = 25000;   // 25ms
  xlKline5BdTester.W4max   = 30000;   // 30ms
  xlKline5BdTester.W4      = 15000;   // 15ms

  xlStatus = xlKlineInit5BdTester(g_xlPhKline, xlChanMaskTester, &xlKline5BdTester);
  printf("exec5BdInitTester, xlKlineInit5BdTester result: %s\n", xlGetErrorString(xlStatus));

  return xlStatus;
}


//------------------------------------------------------------------------------
//   Name:           execFastInitTester                                   
//   Description:    Execute fast init sequence (tester only)
//------------------------------------------------------------------------------
static XLstatus execFastInitTester(XLaccess xlChanMaskTester) {
  XLstatus            xlStatus;
  unsigned char       data[5] = {0x81, 0xE9, 0xF1, 0x81, 0xDC};
  unsigned int        length;
  XLklineInitTester   xlKlineInitTester;

  xlKlineInitTester.TiniL = 25000; // 25ms
  xlKlineInitTester.Twup  = 50000; // 50ms
  
  length = sizeof(data);

  xlStatus = xlKlineFastInitTester(g_xlPhKline, xlChanMaskTester, length, data, &xlKlineInitTester);
  printf("execFastInitTester, xlKlineFastInitTester result: %s\n", xlGetErrorString(xlStatus));

  return xlStatus;
} // execFastInitTester


//------------------------------------------------------------------------------
//   Name:           demoActivateChannel                                   
//   Description:    Init tester/ECU and activate channel
//------------------------------------------------------------------------------
static XLstatus demoActivateChannel(unsigned int func, XLaccess xlChanMask) {

  XLstatus        xlStatus = XL_ERROR;
  
  if (XL_INVALID_PORTHANDLE != g_xlPhKline) {
    if (FUNC_UART == func) {
      xlStatus = prepareUart(xlChanMask);
      printf("demoActivateChannel, prepareUart result: %s\n", xlGetErrorString(xlStatus));
      if (XL_SUCCESS != xlStatus) {
        return xlStatus;
      }
    }

    if (FUNC_TESTER == func) {
      xlStatus = prepareTester(xlChanMask);
      printf("demoActivateChannel, prepareTester result: %s\n", xlGetErrorString(xlStatus));
      if (XL_SUCCESS != xlStatus) {
        return xlStatus;
      }
    }

    if (FUNC_ECUFI == func || FUNC_ECU5BD == func) {
      xlStatus = prepareEcu(xlChanMask, func);
      printf("demoActivateChannel, prepareEcu result: %s\n", xlGetErrorString(xlStatus));
      if (XL_SUCCESS != xlStatus) {
        return xlStatus;
      }
    }

    xlStatus = xlActivateChannel(g_xlPhKline, xlChanMask, XL_BUS_TYPE_KLINE, XL_ACTIVATE_RESET_CLOCK);
    printf("demoActivateChannel, xlActivateChannel (CM:0x%I64x) result: %s\n", xlChanMask, xlGetErrorString(xlStatus));
    if (XL_SUCCESS != xlStatus) {
      return xlStatus;
    }
  }
  else {
    xlStatus = XL_ERR_INVALID_HANDLE;
    printf("demoActivateChannel, invalid port handle!\n");
  }

  return xlStatus;
}


//------------------------------------------------------------------------------
//   Name:           demoInitDriver                                   
//   Description:    initializes the driver with one port and all founded
//                   channels which have a connected K-Line cab/piggy.
//------------------------------------------------------------------------------
static XLstatus demoInitDriver(void) {

  XLstatus        xlStatus;
  unsigned int    i;

  // open the driver
  xlStatus = xlOpenDriver();
  printf("demoInitDriver, xlOpenDriver result: %s\n", xlGetErrorString(xlStatus));
  if (XL_SUCCESS != xlStatus) {
    return xlStatus;
  }
  
  // get/print the hardware configuration
  xlStatus = xlGetDriverConfig(&g_xlDrvConfig);
  printf("demoInitDriver, xlGetDriverConfig result: %s\n", xlGetErrorString(xlStatus));
  if (XL_SUCCESS != xlStatus) {
    return xlStatus;
  }

  // select the wanted channels
  g_xlCmKline   = 0;
  g_KlineChCtr = 0;
  
  for (i = 0; i < g_xlDrvConfig.channelCount; i++) {

    // we take all K-Line hardware with K-Line cabs/piggy's
    if (g_xlDrvConfig.channel[i].channelBusCapabilities & XL_BUS_ACTIVE_CAP_KLINE) { 
      g_xlCmKline |= g_xlDrvConfig.channel[i].channelMask;
      if (g_KlineChCtr < MAX_CH) {
        g_xlCmSelKline[g_KlineChCtr] = g_xlDrvConfig.channel[i].channelMask;
      }
      g_KlineChCtr++;
    }
  }

  if (g_KlineChCtr > 0) {
    printf("demoInitDriver, %d K-Line channel(s) found, CM:0x%I64x\n", g_KlineChCtr, g_xlCmKline);
  }
  else {
    xlStatus = XL_ERROR;
    printf("demoInitDriver, No K-Line channels found!\n");
  }
  
  return xlStatus;
}


//------------------------------------------------------------------------------
//   Name:           demoInitKline                                   
//   Description:    Open a K-Line port
//------------------------------------------------------------------------------
static XLstatus demoInitKline(XLaccess xlChanMask) {

  XLstatus        xlStatus;
  XLaccess        xlPermissionMask = 0;
 
  if (!g_xlCmKline) {
    printf("demoInitKline, no K-Line channels found!\n");
    return XL_ERROR;
  }
  
  xlPermissionMask = g_xlCmKline;

  // open ONE port including all channels
  xlStatus = xlOpenPort(&g_xlPhKline, g_AppName, xlChanMask, &xlChanMask, 256, XL_INTERFACE_VERSION, XL_BUS_TYPE_KLINE);
  printf("demoInitKline, xlOpenPort (PH:%d, CM:0x%I64x, PM:0x%I64x) result: %s\n", 
          g_xlPhKline, g_xlCmKline, xlPermissionMask, xlGetErrorString(xlStatus));

  if ((XL_SUCCESS != xlStatus) || (XL_INVALID_PORTHANDLE == g_xlPhKline)) {
    xlClosePort(g_xlPhKline);
    g_xlPhKline = XL_INVALID_PORTHANDLE;
    xlStatus = XL_ERROR;
  }
  
  return xlStatus;
  
} // demoInitKline            


//------------------------------------------------------------------------------
//   Name:           prepareUart
//   Description:    Set UART params, baudrate of K-Line tester
//------------------------------------------------------------------------------
static XLstatus prepareUart(XLaccess xlChanMask) {
  XLstatus              xlStatus;
  XLklineUartParameter  uartParams;

  uartParams.databits = 8;
  uartParams.parity = XL_KLINE_UART_PARITY_NONE;
  uartParams.stopbits = 1;

  if (!xlChanMask) {
    printf("prepareUart, invalid channel mask!\n");
    return XL_ERR_WRONG_PARAMETER;
  }

  xlStatus = xlKlineSetUartParams(g_xlPhKline, xlChanMask, &uartParams);
  printf("prepareUart, xlKlineSetUartParams (CM:0x%I64x) result: %s\n", xlChanMask, xlGetErrorString(xlStatus));
  if (XL_SUCCESS != xlStatus) {
    return xlStatus;
  }

  xlStatus = xlKlineSetBaudrate(g_xlPhKline, xlChanMask, g_BaudRate);
  printf("prepareUart, xlKlineSetBaudrate (CM:0x%I64x) result: %s\n", xlChanMask, xlGetErrorString(xlStatus));
  if (XL_SUCCESS != xlStatus) {
    return xlStatus;
  }

  xlStatus = xlKlineSwitchTesterResistor(g_xlPhKline, xlChanMask, XL_KLINE_TESTERRESISTOR_ON);
  printf("prepareUart, xlKlineSwitchTesterResistor (CM:0x%I64x) result: %s\n", xlChanMask, xlGetErrorString(xlStatus));
  if (XL_SUCCESS != xlStatus) {
    return xlStatus;
  }

  return xlStatus;

} // prepareUart


//------------------------------------------------------------------------------
//   Name:           prepareTester                                   
//   Description:    Set baudrate, UART, timing settings,... of K-Line tester
//------------------------------------------------------------------------------
static XLstatus prepareTester(XLaccess xlChanMaskTester) {
  XLstatus              xlStatus;
  XLklineUartParameter  uartParams;
  XLklineSetComTester   comTester;

  uartParams.databits = 8;
  uartParams.parity   = XL_KLINE_UART_PARITY_NONE;
  uartParams.stopbits = 1;

  comTester.P1min = 0;
  comTester.P4    = 150;

  if (!xlChanMaskTester) {
    printf("prepareTester, invalid channel mask!\n");
    return XL_ERR_WRONG_PARAMETER;
  }

  xlStatus = xlKlineSetUartParams(g_xlPhKline, xlChanMaskTester, &uartParams);
  printf("prepareTester, xlKlineSetUartParams (CM:0x%I64x) result: %s\n", xlChanMaskTester, xlGetErrorString(xlStatus));
  if (XL_SUCCESS != xlStatus) {
    return xlStatus;
  }

  xlStatus = xlKlineSetCommunicationTimingTester(g_xlPhKline, xlChanMaskTester, &comTester);
  printf("prepareTester, xlKlineSetCommunicationTimingTester (CM:0x%I64x) result: %s\n", xlChanMaskTester, xlGetErrorString(xlStatus));
  if (XL_SUCCESS != xlStatus) {
    return xlStatus;
  }

  xlStatus = xlKlineSetBaudrate(g_xlPhKline, xlChanMaskTester, g_BaudRate);
  printf("prepareTester, xlKlineSetBaudrate (CM:0x%I64x) result: %s\n", xlChanMaskTester, xlGetErrorString(xlStatus));
  if (XL_SUCCESS != xlStatus) {
    return xlStatus;
  }

  xlStatus = xlKlineSwitchTesterResistor(g_xlPhKline, xlChanMaskTester, XL_KLINE_TESTERRESISTOR_ON);
  printf("prepareTester, xlKlineSwitchTesterResistor (CM:0x%I64x) result: %s\n", xlChanMaskTester, xlGetErrorString(xlStatus));
  if (XL_SUCCESS != xlStatus) {
    return xlStatus;
  }

  return xlStatus;

} // prepareTester


//------------------------------------------------------------------------------
//   Name:           prepareEcu                                   
//   Description:    Set baudrate, UART, timing settings,... of K-Line ECU
//                   and execute 5Bd init sequence if required
//------------------------------------------------------------------------------
static XLstatus prepareEcu(XLaccess xlChanMaskEcu, unsigned int func) {
  XLstatus              xlStatus;
  XLklineUartParameter  uartParams;
  XLklineSetComEcu      xlKlineSetComEcu;

  if (!xlChanMaskEcu) {
    printf("prepareEcu, invalid channel mask!\n");
    return XL_ERR_WRONG_PARAMETER;
  }

  switch (func) {
  case FUNC_ECUFI:
  case FUNC_ECU5BD:
    break;
  default:
    printf("prepareEcu, invalid ECU function!\n");
    return XL_ERR_WRONG_PARAMETER;
  }

  memset(&uartParams, 0, sizeof(uartParams));
  
  uartParams.databits = 8;
  uartParams.parity   = XL_KLINE_UART_PARITY_NONE;
  uartParams.stopbits = 1;

  xlStatus = xlKlineSetUartParams(g_xlPhKline, xlChanMaskEcu, &uartParams);
  printf("prepareEcu, xlKlineSetUartParams (CM:0x%I64x) result: %s\n", xlChanMaskEcu, xlGetErrorString(xlStatus));
  if (XL_SUCCESS != xlStatus) {
    return xlStatus;
  }
  
  memset(&xlKlineSetComEcu, 0, sizeof(xlKlineSetComEcu));

  xlKlineSetComEcu.P1       = 5000;
  xlKlineSetComEcu.P4min    = 100;

  if (FUNC_ECUFI == func) {
    xlKlineSetComEcu.TinilMin = 24000; // 24ms
    xlKlineSetComEcu.TinilMax = 26000; // 26ms
    xlKlineSetComEcu.TwupMin  = 49000; // 49ms
    xlKlineSetComEcu.TwupMax  = 51000; // 51ms
  }
  else {
    xlKlineSetComEcu.TinilMin = 0;
    xlKlineSetComEcu.TinilMax = 0;
    xlKlineSetComEcu.TwupMin  = 0;
    xlKlineSetComEcu.TwupMax  = 0;
  }

  xlStatus = xlKlineSetCommunicationTimingEcu(g_xlPhKline, xlChanMaskEcu, &xlKlineSetComEcu);
  printf("prepareEcu, xlKlineSetCommunicationTimingEcu (CM:0x%I64x) result: %s\n", xlChanMaskEcu, xlGetErrorString(xlStatus));
  if (XL_SUCCESS != xlStatus) {
    return xlStatus;
  }
  
  xlStatus = xlKlineSetBaudrate(g_xlPhKline, xlChanMaskEcu, g_BaudRate);
  printf("prepareEcu, xlKlineSetBaudrate (CM:0x%I64x) result: %s\n", xlChanMaskEcu, xlGetErrorString(xlStatus));
  if (XL_SUCCESS != xlStatus) {
    return xlStatus;
  }
  
  if (FUNC_ECU5BD == func) {
    XLkline5BdEcu ecu5BdInitdata;
    memset(&ecu5BdInitdata, 0, sizeof(ecu5BdInitdata));
    ecu5BdInitdata.configure    = XL_KLINE_CONFIGURE_ECU;
    ecu5BdInitdata.addr         = ECU_ADDR;
    ecu5BdInitdata.rate5bd      = 50;
    ecu5BdInitdata.syncPattern  = 0x55;
    ecu5BdInitdata.W1           = 25000;  // 25ms
    ecu5BdInitdata.W2           = 6000;   //  6ms
    ecu5BdInitdata.W3           = 8000;   //  8ms
    ecu5BdInitdata.W4           = 27000;  // 27ms
    ecu5BdInitdata.W4min        = 12000;  // 12ms
    ecu5BdInitdata.W4max        = 17000;  // 17ms
    ecu5BdInitdata.kb1          = 0xa1;
    ecu5BdInitdata.kb2          = 0xa2;
    ecu5BdInitdata.addrNot      = 0;

    xlStatus = xlKlineInit5BdEcu(g_xlPhKline, xlChanMaskEcu, &ecu5BdInitdata);
    printf("prepareEcu, xlKlineInit5BdEcu (CM:0x%I64x) result: %s\n", xlChanMaskEcu, xlGetErrorString(xlStatus));
    if (XL_SUCCESS != xlStatus) {
      return xlStatus;
    }
  }

  return xlStatus;
} // prepareEcu


//------------------------------------------------------------------------------
//   Name:           demoCleanUp                                   
//   Description:    Terminate receive thread, close K-Line port, and close
//                   the driver
//------------------------------------------------------------------------------
static XLstatus demoCleanUp(void)
{
  XLstatus xlStatus;

  // CleanUp K-Line stuff
  if (g_RXKlineThreadRun == 1) {
    g_RXKlineThreadRun = 0;
    // Wait until the threads is gone
    WaitForSingleObject(g_hRXThread, 100);
  }

  if (XL_INVALID_PORTHANDLE != g_xlPhKline) {
    xlStatus = xlClosePort(g_xlPhKline);
    printf("demoCleanUp, xlClosePort (PH:%d) result: %s\n", g_xlPhKline, xlGetErrorString(xlStatus));
  }

  g_xlPhKline = XL_INVALID_PORTHANDLE;

  xlCloseDriver();

  return XL_SUCCESS;    // No error handling
}


//------------------------------------------------------------------------------
//   Name:           demoUsage                                   
//   Description:    Displayed in case of wrong command line arguments
//------------------------------------------------------------------------------
static void demoUsage(void) {
  printf("Usage: xlKlineDemo [<function>] [<baudrate>] [<AppName>]\n");
  printf("\t<function>:   tester\n");
  printf("\t<function>:   ecufi\n");
  printf("\t<function>:   ecu5bd\n");
}


//------------------------------------------------------------------------------
//   Name:           main                                   
//   Description:    The main function.
//------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  XLstatus      xlStatus;
  XLaccess      xlChanMaskTester = 0x0001;
  XLaccess      xlChanMaskEcu = 0x0002;
  int           c;

  printf("----------------------------------------------------------\n");
  printf("-   xlKlineDemo - Test Application for K-Line Driver API -\n");
  printf("-             Vector Informatik GmbH,  " __DATE__"       -\n");
  printf("----------------------------------------------------------\n");

  // Parse command line params
  // First argument: function
  if (argc > 1) {
    if (!stricmp("-?", argv[1])) {
      demoUsage();
      exit(0);
    }
    if (!stricmp("tester", argv[1])) {
      g_Function = FUNC_TESTER;
    }
    else if (!stricmp("ecufi", argv[1])) {
      g_Function = FUNC_ECUFI;
    }
    else if (!stricmp("ecu5bd", argv[1])) {
      g_Function = FUNC_ECU5BD;
    }
    else {
      demoUsage();
      exit(1);
    }
    argc--;
    argv++;
  }
  // Second argument: baudrate
  if (argc > 1) {
    g_BaudRate = atoi(argv[1]);
    if (g_BaudRate) {
      printf("Baudrate = %u\n", g_BaudRate);
      argc--;
      argv++;
    }
    else {
      demoUsage();
      exit(1);
    }
  }
  // Third argument: application name
  if (argc > 1) {
    strncpy(g_AppName, argv[1], XL_MAX_APPNAME);
    g_AppName[XL_MAX_APPNAME] = 0;
    printf("AppName = %s\n", g_AppName);
    argc--;
    argv++;
  }

  // Initialize the driver structures for the application
  xlStatus = demoInitDriver();
  if (XL_SUCCESS != xlStatus) {
    exit(xlStatus);
  }

  // Display hardware config
  demoPrintConfig();

  // Select tester channel
  xlChanMaskTester = g_xlCmSelKline[0];
  // Select ECU channel
  if (g_KlineChCtr > 1) {
    xlChanMaskEcu = g_xlCmSelKline[1];
  }
  else {
    xlChanMaskEcu = g_xlCmSelKline[0];
  }

  // Use only one channel
  g_xlCmKline = xlChanMaskTester;
  if (FUNC_ECU5BD == g_Function || FUNC_ECUFI == g_Function) {
    g_xlCmKline = xlChanMaskEcu;
  }
  printf("\nSelected K-Line channel CM:0x%I64x.\n\n", g_xlCmKline);

  // Open K-Line port
  xlStatus = demoInitKline(g_xlCmKline);
  if (XL_SUCCESS != xlStatus) {
    exit(xlStatus);
  }

  // create the RX thread to read the messages
  xlStatus = demoCreateRxThread(); 
  if (XL_SUCCESS != xlStatus) {
    exit(xlStatus);
  }

  // Go with selected channel on bus
  xlStatus = demoActivateChannel(g_Function, g_xlCmKline);
  if (XL_SUCCESS != xlStatus) {
    exit(xlStatus);
  }
  g_activated = 1;

  printf("\n: Press <h> for help\n");

  // parse the key - commands
  while (g_stop == 0) {

    unsigned long n;
    INPUT_RECORD ir;

    ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &n);

    if ((n == 1) && (ir.EventType == KEY_EVENT)) {

      if (ir.Event.KeyEvent.bKeyDown) {

        c = ir.Event.KeyEvent.uChar.AsciiChar;

        if (isprint(c)) {
          printf("\n=== <%c> ===\n", c);
        }

        switch (c) {

        case 'v':
          demoToggleVerbose();
          break;

        case 't': // transmit a message
        {
          char buf[20];
          unsigned int len;

          memset(buf, 0x73, sizeof(buf));
          len = sizeof(buf);

          strcpy_s(buf, "KLine roxx");
          len = strlen(buf);

          demoTransmit(g_xlCmKline, (unsigned char*)buf, len);
        }
        break;

        case 'T': // transmit a long message
        {
          unsigned char buf[300];
          unsigned int len;

          len = sizeof(buf);
          for (unsigned int i = 0; i < len; ++i) {
            buf[i] = (len - i) % 255;
          }

          demoTransmit(g_xlCmKline, buf, len);
        }
        break;

        case 'i':
          demoToggleTesterResistor(g_xlCmKline);
          break;

        case 'a':
          demoToggleTimerRate();
          break;

        case 'r':
          demoResetClock();
          break;

        case 'G':
          demoGenerateSyncPulse(g_xlCmKline);
          break;

        case 'f':
          execFastInitTester(xlChanMaskTester);
          break;

        case '5':
          exec5BdInitTester(xlChanMaskTester);
          break;

        case 'H':
          demoToggleHighspeedMode(g_xlCmKline);
          break;

        case 's':
          demoStartStop();
          break;

        case 'p':
          demoPrintConfig();
          break;

        case 'h':
          demoHelp();
          break;

        case 27: // ESC: end application
          g_stop = 1;
          break;

        default:
          //printf("key: 0x%x\n",c);
          break;
          
        } // end switch
      }
    }
  } // end while
  

  if ((XL_SUCCESS != xlStatus) && g_activated) { 
    xlStatus = xlDeactivateChannel(g_xlPhKline, g_xlCmKline);
    printf("xlDeactivateChannel (CM:0x%I64x) result: %s\n", g_xlCmKline, xlGetErrorString(xlStatus));
  }

  demoCleanUp();

  return 0;
} // end main()


//------------------------------------------------------------------------------
//   Name:           RxKlineThread                                   
//   Description:    Thread to read out the event queue and parse and print out 
//                   the incoming messages
//------------------------------------------------------------------------------
DWORD WINAPI RxKlineThread(LPVOID par) 
{
  XLstatus        xlStatus;
  unsigned int    msgsrx = RECEIVE_EVENT_SIZE;
  XLevent         xlEvent;

  TStruct * th = (TStruct*)par;

  if (!th) {
    printf("RxKlineThread, invalid parameter passed!\n");
    return ERROR_INVALID_PARAMETER;
  }

  printf("RxKlineThread, PH:%d\n", th->xlPortHandle);

  g_RXKlineThreadRun = 1;

  while (g_RXKlineThreadRun) { 
    WaitForSingleObject(g_hKlineMsgEvent, 10);

    xlStatus = XL_SUCCESS;
    
    while (XL_SUCCESS == xlStatus) {
   
      msgsrx = RECEIVE_EVENT_SIZE;
      xlStatus = xlReceive(th->xlPortHandle, &msgsrx, &xlEvent);      

      if (XL_ERR_QUEUE_IS_EMPTY == xlStatus) {
        continue;
      }

      if (XL_SYNC_PULSE == xlEvent.tag) {
        printf("%s, ch:%d t:%I64u\n", xlGetEventString(&xlEvent), xlEvent.chanIndex, xlEvent.timeStamp);
      }
      else if (XL_TRANSCEIVER == xlEvent.tag) {
        printf("%s, ch:%d is_present:%d\n", xlGetEventString(&xlEvent), xlEvent.chanIndex, xlEvent.tagData.transceiver.is_present);
      }
      else if (XL_KLINE_MSG == xlEvent.tag) {

        switch (xlEvent.tagData.klineData.klineEvtTag) {
          case XL_KLINE_EVT_RX_DATA:
            printf("KLINE_RX_DATA, ch:%d d:0x%x t:%I64u", xlEvent.chanIndex, xlEvent.tagData.klineData.data.klineRx.data, xlEvent.timeStamp);
            if (xlEvent.tagData.klineData.data.klineRx.error) {
              printf(" err:0x%x", xlEvent.tagData.klineData.data.klineRx.error);
            }
            printf("\n");
            break;
         
          case XL_KLINE_EVT_TX_DATA:
            printf("KLINE_TX_DATA, ch:%d d:0x%x t:%I64u", xlEvent.chanIndex, xlEvent.tagData.klineData.data.klineTx.data, xlEvent.timeStamp);
            if (xlEvent.tagData.klineData.data.klineTx.error) {
              printf(" err:0x%x", xlEvent.tagData.klineData.data.klineTx.error);
            }
            printf("\n");
            break;
                                  
          case XL_KLINE_EVT_TESTER_5BD:
            printf("KLINE_TESTER_5BD - ");
            switch (xlEvent.tagData.klineData.data.klineTester5Bd.tag5bd) {
            case XL_KLINE_EVT_TAG_5BD_ADDR:
              printf("XL_KLINE_EVT_TAG_5BD_ADDR - 0x%02x\n", xlEvent.tagData.klineData.data.klineTester5Bd.data);
              break;
            case XL_KLINE_EVT_TAG_5BD_BAUDRATE:
              printf("XL_KLINE_EVT_TAG_5BD_BAUDRATE - %d\n", xlEvent.tagData.klineData.data.klineTester5Bd.data);
              break;
            case XL_KLINE_EVT_TAG_5BD_KB1:
              printf("XL_KLINE_EVT_TAG_5BD_KB1 0x%02x\n", xlEvent.tagData.klineData.data.klineTester5Bd.data);
              break;
            case XL_KLINE_EVT_TAG_5BD_KB2:
              printf("XL_KLINE_EVT_TAG_5BD_KB2 0x%02x\n", xlEvent.tagData.klineData.data.klineTester5Bd.data);
              break;
            case XL_KLINE_EVT_TAG_5BD_KB2NOT:
              printf("XL_KLINE_EVT_TAG_5BD_KB2NOT 0x%02x\n", xlEvent.tagData.klineData.data.klineTester5Bd.data);
              break;
            case XL_KLINE_EVT_TAG_5BD_ADDRNOT:
              printf("XL_KLINE_EVT_TAG_5BD_ADDRNOT 0x%02x\n", xlEvent.tagData.klineData.data.klineTester5Bd.data);
              break;
            default:
              printf("unknown tag5bd:0x%x\n", xlEvent.tagData.klineData.data.klineTester5Bd.tag5bd);
              break;
            }
            break;

          case XL_KLINE_EVT_ECU_5BD:
            printf("KLINE_ECU_5BD - ");
            switch (xlEvent.tagData.klineData.data.klineEcu5Bd.tag5bd) {
            case XL_KLINE_EVT_TAG_5BD_ADDR:
              printf("XL_KLINE_EVT_TAG_5BD_ADDR - 0x%02x\n", xlEvent.tagData.klineData.data.klineEcu5Bd.data);
              break;
            case XL_KLINE_EVT_TAG_5BD_BAUDRATE:
              printf("XL_KLINE_EVT_TAG_5BD_BAUDRATE - %d\n", xlEvent.tagData.klineData.data.klineEcu5Bd.data);
              break;
            case XL_KLINE_EVT_TAG_5BD_KB1:
              printf("XL_KLINE_EVT_TAG_5BD_KB1 0x%02x\n", xlEvent.tagData.klineData.data.klineEcu5Bd.data);
              break;
            case XL_KLINE_EVT_TAG_5BD_KB2:
              printf("XL_KLINE_EVT_TAG_5BD_KB2 0x%02x\n", xlEvent.tagData.klineData.data.klineEcu5Bd.data);
              break;
            case XL_KLINE_EVT_TAG_5BD_KB2NOT:
              printf("XL_KLINE_EVT_TAG_5BD_KB2NOT 0x%02x\n", xlEvent.tagData.klineData.data.klineEcu5Bd.data);
              break;
            case XL_KLINE_EVT_TAG_5BD_ADDRNOT:
              printf("XL_KLINE_EVT_TAG_5BD_ADDRNOT 0x%02x\n", xlEvent.tagData.klineData.data.klineEcu5Bd.data);
              break;
            default:
              printf("unknown tag5bd:0x%x\n", xlEvent.tagData.klineData.data.klineEcu5Bd.tag5bd);
              break;
            }
            break;

          case XL_KLINE_EVT_TESTER_FI_WU_PATTERN:
            printf("KLINE_TESTER_FI_WU_PATTERN\n");
            break;

          case XL_KLINE_EVT_ECU_FI_WU_PATTERN:
            printf("KLINE_ECU_FI_WU_PATTERN\n");
            break;

          case XL_KLINE_EVT_ERROR:
            printf("KLINE_ERROR - %s", 
              XL_KLINE_ERROR_TYPE_RXTX_ERROR==xlEvent.tagData.klineData.data.klineError.klineErrorTag?"RXTX_ERROR":
              XL_KLINE_ERROR_TYPE_5BD_TESTER==xlEvent.tagData.klineData.data.klineError.klineErrorTag?"ERROR_5BD_TESTER":
              XL_KLINE_ERROR_TYPE_5BD_ECU==xlEvent.tagData.klineData.data.klineError.klineErrorTag?"ERROR_5BD_ECU":
              XL_KLINE_ERROR_TYPE_IBS==xlEvent.tagData.klineData.data.klineError.klineErrorTag?"XL_KLINE_IBS":
              XL_KLINE_ERROR_TYPE_FI==xlEvent.tagData.klineData.data.klineError.klineErrorTag?"FI_ERROR":"unknown tag");
            if (XL_KLINE_ERROR_TYPE_IBS == xlEvent.tagData.klineData.data.klineError.klineErrorTag) {
              printf(" - %s", 
                XL_KLINE_ERR_IBS_P1==xlEvent.tagData.klineData.data.klineError.data.ibsErr.ibsErr?"IBS_P1":
                XL_KLINE_ERR_IBS_P4==xlEvent.tagData.klineData.data.klineError.data.ibsErr.ibsErr?"IBS_P4":"unknown ibs");
              if (XL_KLINE_ERR_RXTX_UA & xlEvent.tagData.klineData.data.klineError.data.ibsErr.rxtxErrData) {
                printf(" - Unexpected Activity");
              }
              if (XL_KLINE_ERR_RXTX_MA & xlEvent.tagData.klineData.data.klineError.data.ibsErr.rxtxErrData) {
                printf(" - Missing Activity");
              }
              if (XL_KLINE_ERR_RXTX_ISB & xlEvent.tagData.klineData.data.klineError.data.ibsErr.rxtxErrData) {
                printf(" - Invalid Sync Byte");
              }
            }
            printf("\n");
            break;

          case XL_KLINE_EVT_CONFIRMATION:
            printf("XL_KLINE_EVT_CONFIRMATION ch:%d tag:%04x result:%d\n",
              xlEvent.tagData.klineData.data.klineConfirmation.channel,
              xlEvent.tagData.klineData.data.klineConfirmation.confTag,
              xlEvent.tagData.klineData.data.klineConfirmation.result);
            break;

          default:
            printf("Unknown K-Line event!\n");
            break;
        }

      }
      else {
        if (!g_silent) {
          printf("ph:%d, %s\n", th->xlPortHandle, xlGetEventString(&xlEvent));
        }
      }
    
    }
          
  }
  return NO_ERROR; 
}
