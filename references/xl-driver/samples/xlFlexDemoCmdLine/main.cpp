/*-------------------------------------------------------------------------------------------
| File        : main.cpp
| Project     : Vector FlexRay Command Line Example
|
| Description : Shows the basic FlexRay functionality for the XL Driver Library
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2012 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/


#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <SYS\Stat.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h> 
#include <string.h>
#include <conio.h>
#include <shlwapi.h> // e.g. conversion string to integer

#include "FibexReader.h"


#ifndef DYNAMIC_XLDRIVER_DLL 
#define DYNAMIC_XLDRIVER_DLL 
#endif

#include "vxlapi.h"

#define RX_FIFO_SIZE            0x20000 
#define MAX_PORT                10
#define RECEIVE_EVENT_SIZE      1

// If you want to have only ONE port for FlexRay then set this define!
//#define FR_ONE_PORT

////////////////////////////////////////////////////////////////////////////
// structures
////////////////////////////////////////////////////////////////////////////

typedef struct {
  XLportHandle  xlPortHandle; 
  XLaccess      xlChannelMask;
  XLhandle      hMsgEvent;
} TStruct;

////////////////////////////////////////////////////////////////////////////
// functions 
////////////////////////////////////////////////////////////////////////////
static XLstatus initDriver (void); 
static XLstatus initFlexray (void);
static XLstatus readRxFifo (void); 
static XLstatus CreateFrRxThread(BOOL bNoPrints);
static XLstatus frStartupAndSync(int syncSlotEray, int syncSlotFujitsu, BOOL bNoPrints);
static void printHelp (void); 
static void printConfig (void);
static XLstatus readFrConfig(const char *filename, XLfrClusterConfig &pxlFrConfig);
static int printFrConfig(XLfrClusterConfig *pxlFrConfig);
static void viewFrEvent(XLfrEvent *pxlFrEvent);

static char fooCounter = 0;

DWORD  WINAPI RxThread      (PVOID par);


////////////////////////////////////////////////////////////////////////////
// globals
////////////////////////////////////////////////////////////////////////////

static XLdriverConfig g_driver_config; 
static XLportHandle   g_port_handle[MAX_PORT]; 
static XLaccess       g_channel_mask[MAX_PORT]; 
static XLaccess       g_init_mask; 
static XLhandle       g_notification_handle[MAX_PORT];
static int            g_use_notification_handle; 
static int            g_maxChannels;
static int            g_activeFrChannel;

static char           g_app_name[XL_MAX_LENGTH + 1] = "xlFlexDemoCmdLine"; 

static unsigned int   low_level_retries = 0;

HANDLE                g_hRXThread[MAX_PORT];   
int                   g_RXThreadRun; 
int                   g_TXThreadRun;                                        //!< flag to start/stop the TX thread (for the transmission burst)
HANDLE                g_hTXThread;    //!< thread handle (TX)
TStruct               g_th[MAX_PORT];               
XLfrClusterConfig     g_xlFrConfig;
BOOL                  g_bConfig = FALSE;
unsigned int          g_syncSlotEray, g_syncSlotFujitsu;
int                   erayOffset, erayRepetition, erayChannel;
int                   erayFrameflags, fujitsuFrameflags;
BOOL                  g_bFRaySynced;
BOOL                  g_bFRayThreadTerminated;

static FILE           *g_logFile = NULL;
static bool           g_Logging = false;
unsigned int          g_msgCount = 0;

static bool           g_view = true;
static bool           g_bDriverOpen;

static BOOL           bFlexray = FALSE;

static const char     g_defaultFibexFilename[] = "FIBEX_example.xml";

static CRITICAL_SECTION  critSection;

// ------------------------------------
void enterCritSection(void) {
  EnterCriticalSection(&critSection);  
}

void leaveCritSection(void) {
  LeaveCriticalSection(&critSection);  
}
// ------------------------------------


void printData(const char *data) {
  if (g_view == true)  printf(data);
  if (g_Logging==true) fprintf(g_logFile, data);
}


// ---------------------------------------------------------------------------------------------------------------
// Function     : main
// Description  : Main entry point of the application
// ---------------------------------------------------------------------------------------------------------------
int main (int argc, char* argv[]) {
  UNREFERENCED_PARAMETER(argc);
  UNREFERENCED_PARAMETER(argv);

  XLstatus         xlStatus; 
  int              stop; 
  char             c;
  int              i;
  struct           tm systime;
  time_t           sec;
  errno_t          err;

  printf("----------------------------------------------------------\n");
  printf("-                   xlFlexDemoCmdLine                    -\n");
  printf("-           Vector Informatik GmbH, " __DATE__"          -\n");
  printf("----------------------------------------------------------\n");
  printf("\n");

  // Do some initialization
  g_use_notification_handle = FALSE;
  g_bDriverOpen = FALSE;
  for (i = 0; i < MAX_PORT; i++) {
    g_port_handle[i] = XL_INVALID_PORTHANDLE;
  }
  memset(&g_driver_config, 0x00, sizeof(g_driver_config));

  __try {
    xlStatus = initDriver(); 
    if (xlStatus != XL_SUCCESS) {
      printf("Initializing driver failed! (errorcode=%d)\n", xlStatus);
      __leave; 
    }
    g_bDriverOpen = true;

    printConfig();

    xlStatus = initFlexray(); 
    if (xlStatus) { 
      printData("NO Flexray support\n");
      bFlexray = FALSE;
    } 
    else {
      printf("Flexray: active FR CM=0x%I64x\n", g_channel_mask[g_activeFrChannel]);
      bFlexray = TRUE;
    }

    //InitializeCriticalSection(&critSection);
    printf("#main> "); // Print command prompt

    stop = FALSE; 
    while (!stop) { // checking keyboard and rx fifo 

      Sleep(10);

      if (_kbhit()) { 
        c = (char)_getch(); 
        _putch(c);
        _putch('\n');

        switch (c) { 

          case 'p':
            {
              // Print Flexray cluster configuration
              if (g_bConfig) printFrConfig(&g_xlFrConfig);
              else {
                printf("no config available..., call 'c' at first\n");
              }
              break;
            }

          case 'd':
            {
              // Deactivate Flexray channel
              if (bFlexray) {
                xlStatus = xlDeactivateChannel(g_port_handle[g_activeFrChannel], g_channel_mask[g_activeFrChannel]); 
                printf("Flexray: xlDeactivateChannel CM=0x%I64x, status: %d\n", g_channel_mask[g_activeFrChannel], xlStatus); 
              }
              break; 
            } 

          case 'g':
            {
              // Startup and Sync Flexray bus
              printf("  Please enter sync slot for E-Ray: ");
              scanf_s("%d", &g_syncSlotEray);
              if (g_syncSlotEray) {
                erayFrameflags = 0x20;
                erayFrameflags = XL_FR_FRAMEFLAG_STARTUP | XL_FR_FRAMEFLAG_SYNC | erayFrameflags;
              } else {
                erayFrameflags = 0;
              }
              printf("  Please enter sync slot for Fujitsu: ");
              scanf_s("%d", &g_syncSlotFujitsu);
              if (g_syncSlotFujitsu) {
                fujitsuFrameflags = 0x20;
                fujitsuFrameflags = XL_FR_FRAMEFLAG_STARTUP | XL_FR_FRAMEFLAG_SYNC | fujitsuFrameflags;
              } else {
                fujitsuFrameflags = 0;
              }

              xlStatus = frStartupAndSync(g_syncSlotEray, g_syncSlotFujitsu, false);
              break;
            }

          case 't':
            {
              // Transmit additional Flexray frame
              XLfrEvent     xlFrEvent;
              int           txMode;
              int           frameflags;
              char          payload[254];
              int           offset;
              int           repetition;
              unsigned int  data;
              int           frChannel;
              int           size;
              unsigned int  slot;

              memset(&xlFrEvent, 0, sizeof(xlFrEvent));

              printf("  Please enter slot: ");
              scanf_s("%d", &slot);
              if (slot != g_syncSlotEray) {
                printf("  Please enter offset: ");
                scanf_s("%d", &offset);
                printf("  Please enter repetition: ");
                scanf_s("%d", &repetition);
              }
              else {
                offset = erayOffset;
                repetition = erayRepetition;
              }
              if (slot > g_xlFrConfig.gNumberOfStaticSlots) {
                printf("  Please enter size of dynamic frame (in bytes): ");
                scanf_s("%d", &size);
              }
              else {
                size = g_xlFrConfig.gPayloadLengthStatic << 1;
              }
              printf("  Please enter payload data (byte): 0x");
              scanf_s("%x", &data);
              if (slot != g_syncSlotEray) {
                printf("  Please enter ch (1,2,3): ");
                scanf_s("%d", &frChannel);
                printf("    XL_FR_TX_MODE_CYCLIC              = 0x01\n");
                printf("    XL_FR_TX_MODE_SINGLE_SHOT         = 0x02\n");
                printf("    XL_FR_TX_MODE_NO_NF_IN_USED_CYCLE = 0x04\n");
                printf("    XL_FR_TX_MODE_NO_NF_IN_FREE_CYCLE = 0x08\n");
                printf("    XL_FR_TX_MODE_DATA_UPDATE         = 0x10\n"); 
                printf("    XL_FR_TX_MODE_TX_REQ              = 0x20\n"); 
                printf("    XL_FR_TX_MODE_NONE                = 0xff\n");
                printf("  Please enter txMode: 0x");
                scanf_s("%x", &txMode);
                frameflags = XL_FR_FRAMEFLAG_REQ_TXACK;
              }
              else {
                frChannel = erayChannel;
                txMode = XL_FR_TX_MODE_CYCLIC;
                frameflags = erayFrameflags | XL_FR_FRAMEFLAG_REQ_TXACK;
              }

              xlFrEvent.tag                             = XL_FR_TX_FRAME;
              xlFrEvent.size                            = 254;
              xlFrEvent.flagsChip                       = (unsigned short)frChannel;
              xlFrEvent.userHandle                      = data + 1;
              xlFrEvent.tagData.frTxFrame.flags         = (unsigned short)frameflags; // | erayFrameflags;
              xlFrEvent.tagData.frTxFrame.offset        = (unsigned char)offset;
              xlFrEvent.tagData.frTxFrame.repetition    = (unsigned char)repetition;
              xlFrEvent.tagData.frTxFrame.payloadLength = (unsigned char)((size + 1) >> 1);
              xlFrEvent.tagData.frTxFrame.slotID        = (unsigned short)slot;
              xlFrEvent.tagData.frTxFrame.txMode        = (unsigned char)txMode;
              xlFrEvent.tagData.frTxFrame.incrementOffset = 0;
              xlFrEvent.tagData.frTxFrame.incrementSize   = XL_FR_PAYLOAD_INCREMENT_16BIT;

              memset(payload, data, sizeof(payload));
              payload[0] = (unsigned char)slot;
              payload[size - 1] = (unsigned char)(0xFF - slot + 1);
              memcpy(xlFrEvent.tagData.frTxFrame.data, payload, sizeof(payload));

              xlStatus = xlFrTransmit(g_port_handle[g_activeFrChannel], g_channel_mask[g_activeFrChannel], &xlFrEvent); 
              if (xlStatus != XL_SUCCESS) { 
                printf("Flexray: xlFrTransmit failed, status: %d\n", xlStatus); 
              } 
              else {
                printf("Flexray: transmit message to CM=0x%I64x, status: %d\n", 
                  g_channel_mask[g_activeFrChannel], xlStatus);
              }
              break;
            }

          case 'c':
            {
              // Read cluster configuration from Fibex file
              const unsigned int filename_len = 100;
              char filename[filename_len];

              printf("xlFrSetConfiguration, filename (#='FIBEX_example.xml'): ");

              scanf_s("%s", &filename, filename_len);

              if (filename[0] == '#') {
                // use default
                strcpy_s(filename, filename_len, g_defaultFibexFilename);
              }

              if (readFrConfig(filename, g_xlFrConfig) == XL_SUCCESS) {

                xlStatus = xlFrSetConfiguration(g_port_handle[g_activeFrChannel], g_channel_mask[g_activeFrChannel], &g_xlFrConfig);
                if (xlStatus != XL_SUCCESS) { 
                  printf("xlFrSetConfiguration failed, status: 0x%x\n", xlStatus); 
                } 
                printf("xlFrSetConfiguration, CM=0x%I64x, stat: %d\n", g_channel_mask[g_activeFrChannel], xlStatus);
                g_bConfig = TRUE;
              }
              else g_bConfig = FALSE;
              break;
            }

          case '+':
            {
              // Flexray channel (device) selection
              g_activeFrChannel--;

              if (g_activeFrChannel < 0) g_activeFrChannel = g_maxChannels-1;

              printf("FlexRay: active channel set to CM=0x%I64x\n", 
                g_channel_mask[g_activeFrChannel]);

              break;
            }

          case '-':
            {
              // Flexray channel (device) selection   
              g_activeFrChannel++;

              if (g_activeFrChannel >= g_maxChannels) g_activeFrChannel = 0;

              printf("FlexRay: active channel set to CM=0x%I64x\n", 
                g_channel_mask[g_activeFrChannel]);

              break;
            }

          case '!':
            {
              // Start/Stop logging to file
              if (g_Logging==true) {
                printf("Logging stopped\n");
                time(&sec);
                err = localtime_s(&systime, &sec);
                if (err) {printf("Invalid argument to localtime_s!\n"); exit(1);}
                fprintf(g_logFile,"--------------------------------------------------------------\n");
                fprintf(g_logFile,"     xlFlexDemoCmdLine logging stopped ---- %s", asctime(&systime));
                fprintf(g_logFile,"--------------------------------------------------------------\n");
                fprintf(g_logFile,"%d message received\n", g_msgCount);
                if (g_logFile) {
                  fclose(g_logFile);
                  g_logFile = NULL;
                }
                g_Logging = false; 

              }
              else {
                g_logFile = fopen("xlFlexDemoCmdLine.log", "w");
                if(g_logFile==NULL) {
                  printf("Unable to open log-file\n");
                  break;
                }
                g_Logging = true; 
                printf("Logging started...\n");
                time(&sec);
                err = localtime_s(&systime, &sec);
                if (err) {printf("Invalid argument to localtime_s!\n"); exit(1);}
                fprintf(g_logFile,"--------------------------------------------------------------\n");
                fprintf(g_logFile,"     xlFlexDemoCmdLine logging started ---- %s", asctime(&systime));
                fprintf(g_logFile,"--------------------------------------------------------------\n");
                g_msgCount = 0;
              }
              break; 
            } 

          case 'v':
            {
              // Toggle rx-event display view on/off
              if (g_view == true) {
                printf("Display OFF!\n");
                g_view = false;
              }
              else {
                printf("Display ON!\n");
                g_view = true;
              }
              break;
            }

          case 'h':
            {
              // Print help
              printHelp(); 
              break; 
            } 

          case 13:
            {
              // Print carriage return...
              printf("\n");
              break;
            }

          case 27:
            {
              // on escape key we exit this application
              if (g_Logging==true) {
                printf("Logging stopped\n");
                time(&sec);
                err = localtime_s(&systime, &sec);
                if (err) {printf("Invalid argument to localtime_s!\n"); exit(1);}
                fprintf(g_logFile,"--------------------------------------------------------------\n");
                fprintf(g_logFile,"     xlFlexDemoCmdLine logging stopped ---- %s", asctime(&systime));
                fprintf(g_logFile,"--------------------------------------------------------------\n");
                if (g_logFile) {
                  fclose(g_logFile);
                  g_logFile = NULL;
                }
                g_Logging = false; 
              } 
              stop = TRUE; 
              // Stop the threads...
              g_RXThreadRun = 0;
              xlDeactivateChannel(g_port_handle[g_activeFrChannel],g_channel_mask[g_activeFrChannel]);
              xlClosePort(g_port_handle[g_activeFrChannel]);
              break; 
            } 

          default: {
            printf("Unknown: %c\n", c);
            break; 
                   } 
        } // switch (c) 
        printf("#main> "); // Print command prompt
      } // if (kbhit()) 
    }
  } 

  __finally {
    if (g_bDriverOpen) {
      // deactivate channel, close port and driver
      if (g_port_handle[g_activeFrChannel] != XL_INVALID_PORTHANDLE) {
        xlClosePort(g_port_handle[g_activeFrChannel]);
      }
      (void)xlCloseDriver();
      g_bDriverOpen = false;
    }
  } 

  return XL_SUCCESS; 
} //main()


// ---------------------------------------------------------------------------------------------------------------
// Function     : initDriver
// Description  : Initializes the XLAPI DLL
// ---------------------------------------------------------------------------------------------------------------
static XLstatus initDriver (void) { 

  XLstatus         xlStatus; 

  printf("xlOpenDriver...\n"); 
  xlStatus = xlOpenDriver(); 
  if (xlStatus != XL_SUCCESS) { 
    printf("xlOpenDriver failed, xlStatus: 0x%x\n", xlStatus); 
    return xlStatus; 
  }

  printf("xlGetDriverConfig...\n"); 
  xlStatus = xlGetDriverConfig(&g_driver_config); 
  if (xlStatus != XL_SUCCESS) { 
    printf("xlGetDriverConfig failed, xlStatus: 0x%x\n", xlStatus); 
    return xlStatus; 
  } 
  printf("DLL Version: V%d.%d.%d\n",
    ((g_driver_config.dllVersion) >> 24) & 0xff,
    ((g_driver_config.dllVersion) >> 16) & 0xff,
    (g_driver_config.dllVersion) & 0xffff);

  return xlStatus;
}

// ---------------------------------------------------------------------------------------------------------------
// Function     : initFlexray
// Description  : Initializes the driver of Flexray channels
// ---------------------------------------------------------------------------------------------------------------
static XLstatus initFlexray (void) { 

  XLaccess         init_mask; 
  XLstatus         xlStatus = XL_SUCCESS; 
  unsigned int     channel, initAccessGranted; 
  int              port=0;
  unsigned int     nbrOfBoxes;
  unsigned int     boxMask;
  unsigned int     boxSerial;
  unsigned int     i;
  XLuint64         licInfo[4], tmpLicInfo[4];

  g_maxChannels = 0;

  memset(licInfo, 0, sizeof(licInfo));
  xlStatus = xlGetKeymanBoxes(&nbrOfBoxes);
  if (xlStatus == XL_SUCCESS) {
    printf("xlGetKeymanBoxes: %d Keyman License Dongle(s) found!\n", nbrOfBoxes);
    for (i = 0; i<nbrOfBoxes; i++) {
      memset(tmpLicInfo, 0, sizeof(tmpLicInfo));
      xlStatus = xlGetKeymanInfo(i, &boxMask, &boxSerial, tmpLicInfo);
      if (xlStatus == XL_SUCCESS) {
        printf("xlGetKeymanInfo: Keyman Dongle (%d) with SerialNumber: %d-%d\n", i, boxMask, boxSerial);
        licInfo[0] |= tmpLicInfo[0];
        licInfo[1] |= tmpLicInfo[1];
        licInfo[2] |= tmpLicInfo[2];
        licInfo[3] |= tmpLicInfo[3];
      }
    }
    printf("xlGetKeymanInfo: licInfo[0]=0x%I64x, licInfo[1]=0x%I64x, licInfo[2]=0x%I64x, licInfo[3]=0x%I64x\n", 
      licInfo[0], licInfo[1], licInfo[2], licInfo[3]);
  }


  for (channel = 0; channel < g_driver_config.channelCount; channel++) {
    /*
    printf("Flexray: found hwType: %d, channelBusCapabilities: 0x%x\n", 
    g_driver_config.channel[channel].hwType,
    g_driver_config.channel[channel].channelBusCapabilities); 
    */
    if ( g_driver_config.channel[channel].channelBusCapabilities & XL_BUS_ACTIVE_CAP_FLEXRAY) { 

#ifdef FR_ONE_PORT
      g_channel_mask[port] |= g_driver_config.channel[channel].channelMask;
#else
      g_channel_mask[port] = g_driver_config.channel[channel].channelMask;
      port++;
#endif
      g_maxChannels++;
    } 
  } 

  printf("Flexray: found %d Flexray channels\n", g_maxChannels); 

  if (0 == g_maxChannels) return XL_ERROR;

  g_activeFrChannel = 0;

  // Open the ports...
  initAccessGranted = 0;
#ifndef FR_ONE_PORT
  for (port = 0; port<g_maxChannels; port++) {
#endif

    if (initAccessGranted == 0) {
      init_mask = g_channel_mask[port];
    } else {
      init_mask = 0;
    }

    xlStatus = xlOpenPort(&g_port_handle[port], 
      g_app_name, 
      g_channel_mask[port], 
      &init_mask, 
      RX_FIFO_SIZE, 
      XL_INTERFACE_VERSION_V4, 
      XL_BUS_TYPE_FLEXRAY); 
    if (init_mask != 0) {
      initAccessGranted = 1;
    }

    printf("Flexray: xlOpenPort, PH: %d, CM: 0x%I64x, InitM: 0x%I64x, stat: %d\n", 
      g_port_handle[port], g_channel_mask[port], init_mask, xlStatus); 

    if (xlStatus != XL_SUCCESS) { 
      printf("xlOpenPort failed, xlStatus: %d\n", xlStatus); 
      return xlStatus; 
    } 
#ifndef FR_ONE_PORT
  } 
#endif

  xlStatus = CreateFrRxThread(FALSE);

  return xlStatus; 
} // init_driver 


// ---------------------------------------------------------------------------------------------------------------
// Function     : printHelp
// Description  : Print a help screen
// ---------------------------------------------------------------------------------------------------------------
static void printHelp (void) { 

  printf("Keyboard commands:\n");
  printf(" --- General ---\n");
  printf(" h       Help\n");
  printf(" v       View. Toggle rx-event display view on/off\n");
  printf(" !       Switch On/Off file logging\n");
  printf(" p       Print current Flexray cluster configuration\n");
  printf(" --- Flexray ---\n");
  printf(" -       Change the FR channel 'up'\n");
  printf(" +       Change the FR channel 'down'\n");
  printf(" g       Go. Init the E-Ray/Coldstart CC and activate channel\n");
  printf(" t       Transmit additional FlexRay frame\n");
  printf(" d       Deactivate (stop) channel\n");
  printf(" c       Load FlexRay config from Fibex file\n");

  return; 
} // print_help 

// ---------------------------------------------------------------------------------------------------------------
// Function     : printConfig
// Description  : Print the current hardware configuration
// ---------------------------------------------------------------------------------------------------------------
void printConfig() {
  unsigned int i;
  char         str[XL_MAX_LENGTH + 1]="";

  printf("----------------------------------------------------------\n");
  printf("- %02d channels       Hardware Configuration               -\n", g_driver_config.channelCount);
  printf("----------------------------------------------------------\n");

  for (i=0; i < g_driver_config.channelCount; i++) {

    printf("- Ch:%02d, CM:0x%03I64x,",
      g_driver_config.channel[i].channelIndex, g_driver_config.channel[i].channelMask);

    strncpy(str, g_driver_config.channel[i].name, 23);
    printf(" %23s,", str);

    memset(str, 0, sizeof(str));
    
    if (g_driver_config.channel[i].transceiverType != XL_TRANSCEIVER_TYPE_NONE) {
      strncpy(str, g_driver_config.channel[i].transceiverName, 13);
      printf("%13s -\n", str);
    }
    else printf("    no Cab!   -\n");
    
  }
  printf("----------------------------------------------------------\n\n");
} //printConfig()

// ---------------------------------------------------------------------------------------------------------------
// Function     : printFrConfig
// Description  : Print the current Flexray cluster configuration
// ---------------------------------------------------------------------------------------------------------------
static int printFrConfig(XLfrClusterConfig *pxlFrConfig) {
  printf("-----------------------------\n");
  printf("------ FR Configuration -----\n");
  printf("-----------------------------\n");

  printf("busGuardianEnable:                  %d\n", pxlFrConfig->busGuardianEnable);
  printf("baudrate:                           %d\n", pxlFrConfig->baudrate);                         
  printf("busGuardianTick:                    %d\n", pxlFrConfig->busGuardianTick);                  
  printf("externalClockCorrectionMode:        %d\n", pxlFrConfig->externalClockCorrectionMode);      
  printf("gColdStartAttempts:                 %d\n", pxlFrConfig->gColdStartAttempts);               
  printf("gListenNoise:                       %d\n", pxlFrConfig->gListenNoise);                     
  printf("gMacroPerCycle:                     %d\n", pxlFrConfig->gMacroPerCycle);                   
  printf("gMaxWithoutClockCorrectionFatal:    %d\n", pxlFrConfig->gMaxWithoutClockCorrectionFatal);  
  printf("gMaxWithoutClockCorrectionPassive:  %d\n", pxlFrConfig->gMaxWithoutClockCorrectionPassive);
  printf("gNetworkManagementVectorLength:     %d\n", pxlFrConfig->gNetworkManagementVectorLength);   
  printf("gNumberOfMinislots:                 %d\n", pxlFrConfig->gNumberOfMinislots);               
  printf("gNumberOfStaticSlots:               %d\n", pxlFrConfig->gNumberOfStaticSlots);             
  printf("gOffsetCorrectionStart:             %d\n", pxlFrConfig->gOffsetCorrectionStart);           
  printf("gPayloadLengthStatic:               %d\n", pxlFrConfig->gPayloadLengthStatic);             
  printf("gSyncNodeMax:                       %d\n", pxlFrConfig->gSyncNodeMax);                     
  printf("gdActionPointOffset:                %d\n", pxlFrConfig->gdActionPointOffset);              
  printf("gdDynamicSlotIdlePhase:             %d\n", pxlFrConfig->gdDynamicSlotIdlePhase);           
  printf("gdMacrotick:                        %d\n", pxlFrConfig->gdMacrotick);                      
  printf("gdMinislot:                         %d\n", pxlFrConfig->gdMinislot);                       
  printf("gdMiniSlotActionPointOffset:        %d\n", pxlFrConfig->gdMiniSlotActionPointOffset);      
  printf("gdNIT:                              %d\n", pxlFrConfig->gdNIT);                            
  printf("gdStaticSlot:                       %d\n", pxlFrConfig->gdStaticSlot);                     
  printf("gdSymbolWindow:                     %d\n", pxlFrConfig->gdSymbolWindow);                   
  printf("gdTSSTransmitter:                   %d\n", pxlFrConfig->gdTSSTransmitter);                 
  printf("gdWakeupSymbolRxIdle:               %d\n", pxlFrConfig->gdWakeupSymbolRxIdle);             
  printf("gdWakeupSymbolRxLow:                %d\n", pxlFrConfig->gdWakeupSymbolRxLow);              
  printf("gdWakeupSymbolRxWindow:             %d\n", pxlFrConfig->gdWakeupSymbolRxWindow);           
  printf("gdWakeupSymbolTxIdle:               %d\n", pxlFrConfig->gdWakeupSymbolTxIdle);             
  printf("gdWakeupSymbolTxLow:                %d\n", pxlFrConfig->gdWakeupSymbolTxLow);              
  printf("pAllowHaltDueToClock:               %d\n", pxlFrConfig->pAllowHaltDueToClock);             
  printf("pAllowPassiveToActive:              %d\n", pxlFrConfig->pAllowPassiveToActive);            
  printf("pChannels:                          %d\n", pxlFrConfig->pChannels);                        
  printf("pClusterDriftDamping:               %d\n", pxlFrConfig->pClusterDriftDamping);             
  printf("pDecodingCorrection:                %d\n", pxlFrConfig->pDecodingCorrection);              
  printf("pDelayCompensationA:                %d\n", pxlFrConfig->pDelayCompensationA);              
  printf("pDelayCompensationB:                %d\n", pxlFrConfig->pDelayCompensationB);              
  printf("pExternOffsetCorrection:            %d\n", pxlFrConfig->pExternOffsetCorrection);          
  printf("pExternRateCorrection:              %d\n", pxlFrConfig->pExternRateCorrection);            
  printf("pKeySlotUsedForStartup:             %d\n", pxlFrConfig->pKeySlotUsedForStartup);           
  printf("pKeySlotUsedForSync:                %d\n", pxlFrConfig->pKeySlotUsedForSync);              
  printf("pLatestTx:                          %d\n", pxlFrConfig->pLatestTx);                        
  printf("pMacroInitialOffsetA:               %d\n", pxlFrConfig->pMacroInitialOffsetA);             
  printf("pMacroInitialOffsetB:               %d\n", pxlFrConfig->pMacroInitialOffsetB);             
  printf("pMaxPayloadLengthDynamic:           %d\n", pxlFrConfig->pMaxPayloadLengthDynamic);         
  printf("pMicroInitialOffsetA:               %d\n", pxlFrConfig->pMicroInitialOffsetA);             
  printf("pMicroInitialOffsetB:               %d\n", pxlFrConfig->pMicroInitialOffsetB);             
  printf("pMicroPerCycle:                     %d\n", pxlFrConfig->pMicroPerCycle);                   
  printf("pMicroPerMacroNom:                  %d\n", pxlFrConfig->pMicroPerMacroNom);                
  printf("pOffsetCorrectionOut:               %d\n", pxlFrConfig->pOffsetCorrectionOut);             
  printf("pRateCorrectionOut:                 %d\n", pxlFrConfig->pRateCorrectionOut);               
  printf("pSamplesPerMicrotick:               %d\n", pxlFrConfig->pSamplesPerMicrotick);             
  printf("pSingleSlotEnabled:                 %d\n", pxlFrConfig->pSingleSlotEnabled);               
  printf("pWakeupChannel:                     %d\n", pxlFrConfig->pWakeupChannel);                   
  printf("pWakeupPattern:                     %d\n", pxlFrConfig->pWakeupPattern);                   
  printf("pdAcceptedStartupRange:             %d\n", pxlFrConfig->pdAcceptedStartupRange);           
  printf("pdListenTimeout:                    %d\n", pxlFrConfig->pdListenTimeout);                  
  printf("pdMaxDrift:                         %d\n", pxlFrConfig->pdMaxDrift);                       
  printf("pdMicrotick:                        %d\n", pxlFrConfig->pdMicrotick);                      
  printf("gdCASRxLowMax:                      %d\n", pxlFrConfig->gdCASRxLowMax);                    
  printf("gChannels:                          %d\n", pxlFrConfig->gChannels);                        
  printf("vExternOffsetControl:               %d\n", pxlFrConfig->vExternOffsetControl);             
  printf("vExternRateControl:                 %d\n", pxlFrConfig->vExternRateControl);               
  printf("pChannelsMTS:                       %d\n", pxlFrConfig->pChannelsMTS);                    

  return 0;
} //printFrConfig()


// ---------------------------------------------------------------------------------------------------------------
// Function     : readFrConfig
// Description  : Read Flexray cluster configuration from Fibex file
// ---------------------------------------------------------------------------------------------------------------
static XLstatus readFrConfig(const char *filename, XLfrClusterConfig &pxlFrConfig) {
  XLstatus rc;

  if (g_view) printf("read Fibex file: %s\n", filename);

  rc = FbxRd_LoadFibexConfig(filename, pxlFrConfig);
  if (rc != XL_SUCCESS) {
    printf("*Error in open XML file!\n");
    return -1;
  }
  else {
    printf("*Fibex successfully read.\n");
  }
  return XL_SUCCESS;
} //readFrConfig


// ---------------------------------------------------------------------------------------------------------------
// Function     : CreateRxThread
// Description  : Creates the receive thread
// ---------------------------------------------------------------------------------------------------------------
XLstatus CreateFrRxThread(BOOL bNoPrints) {
  XLstatus      xlStatus = XL_ERROR;
  DWORD         ThreadId=0;
  int           port=0;

#ifndef FR_ONE_PORT
  for (port=0; port<g_maxChannels; port++) {
#endif
    if (g_port_handle[port]!= XL_INVALID_PORTHANDLE) {
      // Send a event for each Msg!!!
      xlStatus = xlSetNotification (g_port_handle[port], &g_notification_handle[port], 1);
      if (xlStatus) { 
        printf("xlSetNotificationHandle failed, xlStatus: %d\n", xlStatus); 
        return XL_ERROR; 
      } 
      if (!bNoPrints) printf("FlexRay: xlSetNotificationHandle, xlHandle: %p, xlStatus: %d\n", g_notification_handle[port], xlStatus); 

      g_th[port].xlPortHandle      = g_port_handle[port];
      g_th[port].xlChannelMask     = g_channel_mask[port];
      g_th[port].hMsgEvent         = g_notification_handle[port];  

      g_hRXThread[port] = CreateThread(0, 0x1000, RxThread, (LPVOID) &g_th[port], 0, &ThreadId);

    }
#ifndef FR_ONE_PORT
  }
#endif 
  return xlStatus;
} //CreateFrRxThread()


// ---------------------------------------------------------------------------------------------------------------
// Function     : viewFrEvent
// Description  : Print received events to screen
// ---------------------------------------------------------------------------------------------------------------
void viewFrEvent(XLfrEvent *pxlFrEvent) {
  const unsigned int outData_len = 256;
  char outData[outData_len];
  int i;
  bool localView;

  if (pxlFrEvent->flagsChip & XL_FR_QUEUE_OVERFLOW) {
    localView = g_view;
    g_view = true;
    sprintf_s(outData, outData_len,"XL_FR_QUEUE_OVERFLOW occured\n");
    printData(outData);
    g_view = localView;
  }

  switch (pxlFrEvent->tag)
  {
  case XL_FR_RX_FRAME: 
  case XL_FR_TXACK_FRAME:
  case XL_FR_INVALID_FRAME:
    {

      if (XL_FR_TXACK_FRAME == pxlFrEvent->tag) printData("XL_FR_TXACK_FRAME: ");
      else if (XL_FR_INVALID_FRAME == pxlFrEvent->tag) printData("XL_FR_INVALID_FRAME: ");
      else printData("XL_FR_RX_FRAME: ");

      if (pxlFrEvent->tagData.frRxFrame.flags) {
        if (pxlFrEvent->tagData.frRxFrame.flags & XL_FR_FRAMEFLAG_NULLFRAME) {
          printData("NULL,");
        }
        if (pxlFrEvent->tagData.frRxFrame.flags & XL_FR_FRAMEFLAG_SYNC) {
          printData("SYNC,");
        }
        if (pxlFrEvent->tagData.frRxFrame.flags & XL_FR_FRAMEFLAG_STARTUP) {
          printData("STARTUP,");
        }
        if (pxlFrEvent->tagData.frRxFrame.flags & XL_FR_FRAMEFLAG_PAYLOAD_PREAMBLE) {
          printData("PAYLOAD PREAMBLE,");
        }
        if (pxlFrEvent->tagData.frRxFrame.flags & XL_FR_FRAMEFLAG_NEW_DATA_TX) {
          printData("NEW DATA,");
        }
        if (pxlFrEvent->tagData.frRxFrame.flags & XL_FR_FRAMEFLAG_DATA_UPDATE_LOST) {
          printData("DATA LOST,");
        }
      }
      else printData("    ");

      if (pxlFrEvent->tagData.frRxFrame.flags & XL_FR_FRAMEFLAG_FR_RESERVED) {
        printData("RESERVED");
      }
      if (pxlFrEvent->tagData.frRxFrame.flags & XL_FR_FRAMEFLAG_REQ_TXACK) {
        printData("TXACK ");
      }

      sprintf_s(outData, outData_len," ci: %d, fCh: 0x%x, fl: 0x%02x, cy:%02d, sl:%04d\n                 pl: %d, hCRC: 0x%x, uH: %d\n",
        pxlFrEvent->channelIndex,
        pxlFrEvent->flagsChip,
        pxlFrEvent->tagData.frRxFrame.flags,
        pxlFrEvent->tagData.frRxFrame.cycleCount, pxlFrEvent->tagData.frRxFrame.slotID,
        pxlFrEvent->tagData.frRxFrame.payloadLength, pxlFrEvent->tagData.frRxFrame.headerCRC,
        pxlFrEvent->userHandle);
      printData(outData);

      sprintf_s(outData, outData_len, "                 t:%I64d, tSync: %I64d\n", pxlFrEvent->timeStamp, pxlFrEvent->timeStampSync);
      printData(outData);

      printData("          data: ");
      for (i=0;i<(pxlFrEvent->tagData.frRxFrame.payloadLength*2);i++) {
        sprintf_s(outData, outData_len, " %02x", pxlFrEvent->tagData.frRxFrame.data[i]);
        printData(outData);
        if (!((i + 1) % 8))printData("\n                "); 
      }

      printData("\n");

      // increase the message counter
      g_msgCount++;
      break;
    }

  case XL_FR_STATUS:
    {
      if (pxlFrEvent->tagData.frStatus.statusType == XL_FR_STATUS_NORMAL_ACTIVE) {
        g_bFRaySynced = TRUE;
      }
      sprintf_s(outData, outData_len, "XL_FR_STATUS, ci: %d, fC:0x%x, statusType: 0x%x, t:%I64d, tSync:%I64d\n", 
        pxlFrEvent->channelIndex, pxlFrEvent->flagsChip, pxlFrEvent->tagData.frStatus.statusType, 
        pxlFrEvent->timeStamp, pxlFrEvent->timeStampSync);
      printData(outData);
      break;
    }

  case XL_SYNC_PULSE:
    {
      sprintf_s(outData, outData_len, "XL_SYNC_PULSE, tSource: %d, uH: 0x%x, t:%I64d, tSync:%I64d\n", 
        pxlFrEvent->tagData.frSyncPulse.triggerSource, pxlFrEvent->userHandle, 
        pxlFrEvent->timeStamp, pxlFrEvent->timeStampSync);
      printData(outData);
      break;
    }

  case XL_FR_ERROR:
    {
      sprintf_s(outData, outData_len, "XL_FR_ERROR, ci: %d, errorTag: 0x%x, t:%I64d, tSync:%I64d\n", 
        pxlFrEvent->channelIndex, pxlFrEvent->tagData.frError.tag, 
        pxlFrEvent->timeStamp, pxlFrEvent->timeStampSync);
      printData(outData);
      break;
    }

  case XL_FR_START_CYCLE:
    {
      sprintf_s(outData, outData_len, "XL_FR_START_CYCLE, ci: %d, fC:0x%x, cycleCount: %d, offsetC: %d, rateC: %d, t:%I64d, tSync:%I64d\n", 
        pxlFrEvent->channelIndex, pxlFrEvent->flagsChip, pxlFrEvent->tagData.frStartCycle.cycleCount,
        pxlFrEvent->tagData.frStartCycle.vOffsetCorrection, pxlFrEvent->tagData.frStartCycle.vRateCorrection,
        pxlFrEvent->timeStamp, pxlFrEvent->timeStampSync);
      printData(outData);
      break;
    }

  case XL_FR_SPY_FRAME:
    {
      sprintf_s(outData, outData_len, "XL_FR_SPY_FRAME ci: %d, fC: 0x%x, cy:%02d, sl:%04d\n                 pl: %d, hCRC: 0x%x, uH: %d, frmErr: 0x%x\n",
        pxlFrEvent->channelIndex,
        pxlFrEvent->flagsChip,
        pxlFrEvent->tagData.frSpyFrame.cycleCount,
        pxlFrEvent->tagData.frSpyFrame.slotID,
        pxlFrEvent->tagData.frSpyFrame.payloadLength, pxlFrEvent->tagData.frSpyFrame.headerCRC,
        pxlFrEvent->userHandle,
        pxlFrEvent->tagData.frSpyFrame.frameError);
      printData(outData);

      sprintf_s(outData, outData_len, "                 t:%I64d, tSync: %I64d\n", pxlFrEvent->timeStamp, pxlFrEvent->timeStampSync);
      printData(outData);

      printData("          data: ");
      for (i=0;i<(pxlFrEvent->tagData.frSpyFrame.payloadLength*2);i++) {
        sprintf_s(outData, outData_len, " %02x", pxlFrEvent->tagData.frSpyFrame.data[i]);
        printData(outData);
        if (!((i + 1) % 8))printData("\n                "); 
      }

      printData("\n");

      break;
    }

  case XL_FR_SPY_SYMBOL:
    {
      sprintf_s(outData, outData_len, "XL_FR_SPY_SYMBOL, ci: %d, fC:0x%x, lowLength: %d, t:%I64d, tSync:%I64d\n", 
        pxlFrEvent->channelIndex, pxlFrEvent->flagsChip, pxlFrEvent->tagData.frSpySymbol.lowLength, 
        pxlFrEvent->timeStamp, pxlFrEvent->timeStampSync);
      printData(outData);
      break;
    }

  case XL_FR_WAKEUP:
    {
      sprintf_s(outData, outData_len, "XL_FR_WAKEUP, ci: %d, fC:0x%x, cy:%02d, wakeUpS:%d, t:%I64d, tSync:%I64d\n", 
        pxlFrEvent->channelIndex, pxlFrEvent->flagsChip, pxlFrEvent->tagData.frWakeup.cycleCount, 
        pxlFrEvent->tagData.frWakeup.wakeupStatus, 
        pxlFrEvent->timeStamp, pxlFrEvent->timeStampSync);
      printData(outData);
      break;
    }

  case XL_FR_SYMBOL_WINDOW:
    {
      sprintf_s(outData, outData_len, "XL_FR_SYMBOL_WINDOW, ci: %d, fC:0x%x, cycleCount: %d, t:%I64d, tSync:%I64d\n", 
        pxlFrEvent->channelIndex, pxlFrEvent->flagsChip, pxlFrEvent->tagData.frSymbolWindow.cycleCount, 
        pxlFrEvent->timeStamp, pxlFrEvent->timeStampSync);
      printData(outData);
      break;
    }

  default:
    {
      sprintf_s(outData, outData_len, "UNKNOWN event, tag: 0x%x\n", pxlFrEvent->tag);
      printData(outData);
    }
  }
} //viewFrEvent()


///////////////////////////////////////////////////////////////////////////

//! frStartupAndSync

//! Startup and sync Flexray network
//!
////////////////////////////////////////////////////////////////////////////
// ---------------------------------------------------------------------------------------------------------------
// Function     : frStartupAndSync
// Description  : Setup Flexray startup and sync frames and startup&sync the bus
// ---------------------------------------------------------------------------------------------------------------
XLstatus frStartupAndSync(int syncSlotEray, int syncSlotFujitsu, BOOL bNoPrints) {
  XLfrEvent       xlFrConfigFrame;
  XLfrMode	      xlFrMode;
  XLstatus        xlStatus;
  char            payload[254]; // here we have bytes

  //////////////////////////////////////////////////////////////////////////////

  if (readFrConfig(g_defaultFibexFilename, g_xlFrConfig) == XL_SUCCESS) {

    xlStatus = xlFrSetConfiguration(g_port_handle[g_activeFrChannel], g_channel_mask[g_activeFrChannel], &g_xlFrConfig);
    if (xlStatus != XL_SUCCESS) { 
      printf("xlFrSetConfiguration failed, status: 0x%x\n", xlStatus);
      return xlStatus;
    } 
    if (!bNoPrints) printf("xlFrSetConfiguration, stat: %d\n", xlStatus);
    g_bConfig = TRUE;
  }
  else {
    g_bConfig = FALSE;
    return XL_ERR_INVALID_ACCESS;
  }

  erayOffset = 0;
  erayRepetition = 2;

  memset(&xlFrConfigFrame, 0, sizeof(xlFrConfigFrame));

  //////////////////////////////////////////////////////////////////////////////

  xlFrConfigFrame.tag                             = XL_FR_TX_FRAME;
  xlFrConfigFrame.flagsChip                       = XL_FR_CHANNEL_A;
  xlFrConfigFrame.size                            = 254;
  xlFrConfigFrame.userHandle                      = 101;
  xlFrConfigFrame.tagData.frTxFrame.flags         = (unsigned short)erayFrameflags;
  xlFrConfigFrame.tagData.frTxFrame.offset        = (unsigned char)erayOffset;
  xlFrConfigFrame.tagData.frTxFrame.repetition    = (unsigned char)erayRepetition;
  xlFrConfigFrame.tagData.frTxFrame.payloadLength = (unsigned char)g_xlFrConfig.gPayloadLengthStatic; // -> FR calculates in WORDS
  xlFrConfigFrame.tagData.frTxFrame.slotID        = (unsigned short)syncSlotEray;
  xlFrConfigFrame.tagData.frTxFrame.txMode        = XL_FR_TX_MODE_CYCLIC;
  xlFrConfigFrame.tagData.frTxFrame.incrementOffset = 0;
  xlFrConfigFrame.tagData.frTxFrame.incrementSize   = 0;
  memset(payload, 0x88, sizeof(payload));
  payload[0] = (unsigned char)syncSlotEray;
  payload[g_xlFrConfig.gPayloadLengthStatic * 2 - 1] = (unsigned char)(0xFF - syncSlotEray + 1);
  memcpy(xlFrConfigFrame.tagData.frTxFrame.data, payload, sizeof(payload));

  if (syncSlotEray) {
    xlStatus = xlFrInitStartupAndSync(g_port_handle[g_activeFrChannel], g_channel_mask[g_activeFrChannel], &xlFrConfigFrame);
    if (!bNoPrints) printf("xlFrInitStartupAndSync (for XL_FR_CHANNEL_A), stat: %d\n", xlStatus);
  }

  //////////////////////////////////////////////////////////////////////////////

  xlFrConfigFrame.tag                             = XL_FR_TX_FRAME;
  xlFrConfigFrame.flagsChip                       = XL_FR_CHANNEL_B;
  xlFrConfigFrame.size                            = 254;
  xlFrConfigFrame.userHandle                      = 101;
  xlFrConfigFrame.tagData.frTxFrame.flags         = (unsigned short)erayFrameflags;
  xlFrConfigFrame.tagData.frTxFrame.offset        = (unsigned char)erayOffset;
  xlFrConfigFrame.tagData.frTxFrame.repetition    = (unsigned char)erayRepetition;
  xlFrConfigFrame.tagData.frTxFrame.payloadLength = (unsigned char)g_xlFrConfig.gPayloadLengthStatic; // -> FR calculates in WORDS
  xlFrConfigFrame.tagData.frTxFrame.slotID        = (unsigned short)syncSlotEray;
  xlFrConfigFrame.tagData.frTxFrame.txMode        = XL_FR_TX_MODE_CYCLIC;
  xlFrConfigFrame.tagData.frTxFrame.incrementOffset = 0;
  xlFrConfigFrame.tagData.frTxFrame.incrementSize   = 0;
  memset(payload, 0x88, sizeof(payload));
  payload[0] = (unsigned char)syncSlotEray;
  payload[g_xlFrConfig.gPayloadLengthStatic * 2 - 1] = (unsigned char)(0xFF - syncSlotEray + 1);
  memcpy(xlFrConfigFrame.tagData.frTxFrame.data, payload, sizeof(payload));

  if (syncSlotEray) {
    //xlStatus = xlFrInitStartupAndSync(g_port_handle[g_activeFrChannel], g_channel_mask[g_activeFrChannel], &xlFrConfigFrame);
    //printf("xlFrInitStartupAndSync (for XL_FR_CHANNEL_B), stat: %d\n", xlStatus);
  }

  //////////////////////////////////////////////////////////////////////////////

  xlFrConfigFrame.tag                             = XL_FR_TX_FRAME;
  xlFrConfigFrame.flagsChip                       = XL_FR_CC_COLD_A;
  xlFrConfigFrame.size                            = 254;
  xlFrConfigFrame.userHandle                      = 101;
  xlFrConfigFrame.tagData.frTxFrame.flags         = (unsigned short)fujitsuFrameflags;
  xlFrConfigFrame.tagData.frTxFrame.offset        = 0;
  xlFrConfigFrame.tagData.frTxFrame.repetition    = 1;
  xlFrConfigFrame.tagData.frTxFrame.payloadLength = (unsigned char)g_xlFrConfig.gPayloadLengthStatic; // -> FR calculates in WORDS
  xlFrConfigFrame.tagData.frTxFrame.slotID        = (unsigned short)syncSlotFujitsu;
  xlFrConfigFrame.tagData.frTxFrame.txMode        = XL_FR_TX_MODE_CYCLIC;

  memset(payload, 0xFF, sizeof(payload));
  memcpy(xlFrConfigFrame.tagData.frTxFrame.data, payload, sizeof(payload));

  if (syncSlotFujitsu) {
    xlStatus = xlFrInitStartupAndSync(g_port_handle[g_activeFrChannel], g_channel_mask[g_activeFrChannel], &xlFrConfigFrame);
    if (!bNoPrints) printf("xlFrInitStartupAndSync (for XL_FR_CC_COLD_A), stat: %d\n", xlStatus);
  }

  //////////////////////////////////////////////////////////////////////////////

  xlFrConfigFrame.tag                             = XL_FR_TX_FRAME;
  xlFrConfigFrame.flagsChip                       = XL_FR_CC_COLD_B;
  xlFrConfigFrame.size                            = 254;
  xlFrConfigFrame.userHandle                      = 101;
  xlFrConfigFrame.tagData.frTxFrame.flags         = (unsigned short)fujitsuFrameflags;
  xlFrConfigFrame.tagData.frTxFrame.offset        = 0;
  xlFrConfigFrame.tagData.frTxFrame.repetition    = 1;
  xlFrConfigFrame.tagData.frTxFrame.payloadLength = (unsigned char)g_xlFrConfig.gPayloadLengthStatic; // -> FR calculates in WORDS
  xlFrConfigFrame.tagData.frTxFrame.slotID        = (unsigned short)syncSlotFujitsu;
  xlFrConfigFrame.tagData.frTxFrame.txMode        = XL_FR_TX_MODE_CYCLIC;

  memset(payload, 0xFF, sizeof(payload));
  memcpy(xlFrConfigFrame.tagData.frTxFrame.data, payload, sizeof(payload));

  if (syncSlotFujitsu) {
    xlStatus = xlFrInitStartupAndSync(g_port_handle[g_activeFrChannel], g_channel_mask[g_activeFrChannel], &xlFrConfigFrame);
    if (!bNoPrints) printf("xlFrInitStartupAndSync (for XL_FR_CC_COLD_B), stat: %d\n", xlStatus);
  }

  //////////////////////////////////////////////////////////////////////////////

  xlFrMode.frMode               = XL_FR_MODE_NORMAL;
  if (syncSlotEray) {
    xlFrMode.frStartupAttributes  = XL_FR_MODE_COLDSTART_LEADING; // XL_FR_MODE_WAKEUP_AND_COLDSTART_LEADING
    //xlFrMode.frStartupAttributes  = XL_FR_MODE_WAKEUP;
  } else {
    xlFrMode.frStartupAttributes  = XL_FR_MODE_NONE;
  }

  xlStatus = xlFrSetMode(g_port_handle[g_activeFrChannel], g_channel_mask[g_activeFrChannel], &xlFrMode);
  if (!bNoPrints) printf("xlFrSetMode, (for XL_FR_MODE_NORMAL, coldstart leading), stat: %d\n", xlStatus);
  //printf("xlFrSetMode, (for XL_FR_MODE_NORMAL, wakeup + coldstart leading), stat: %d\n", xlStatus);

  //////////////////////////////////////////////////////////////////////////////

  xlFrMode.frMode               = XL_FR_MODE_COLD_NORMAL;
  xlFrMode.frStartupAttributes  = XL_FR_MODE_COLDSTART_LEADING;

  if (syncSlotFujitsu) {
    xlStatus = xlFrSetMode(g_port_handle[g_activeFrChannel], g_channel_mask[g_activeFrChannel], &xlFrMode);
    if (!bNoPrints) printf("xlFrSetMode, (for XL_FR_MODE_COLD_NORMAL), stat: %d\n", xlStatus);
  }

  //////////////////////////////////////////////////////////////////////////////

  xlStatus = xlActivateChannel(g_port_handle[g_activeFrChannel], g_channel_mask[g_activeFrChannel], XL_BUS_TYPE_FLEXRAY, 0); 
  if (!bNoPrints) printf("xlActivateChannel, CM=0x%I64x, stat: %d\n", g_channel_mask[g_activeFrChannel], xlStatus); 

  return xlStatus;
} //frStartupAndSync


// ---------------------------------------------------------------------------------------------------------------
// Function     : RxThread
// Description  : Receive thread for receiving events
// ---------------------------------------------------------------------------------------------------------------
DWORD WINAPI RxThread(LPVOID par) {
  XLstatus        xlStatus;
  DWORD           wfso;


  XLfrEvent       xlFrEvent; 
  TStruct         *pTh;

  pTh = (TStruct*) par;

  g_RXThreadRun = 1;
  g_bFRayThreadTerminated = FALSE;
  g_bFRaySynced = FALSE;

  while (g_RXThreadRun) { 

    wfso = WaitForSingleObject(pTh->hMsgEvent,20);
    //if (!wfso) printf("got event: 0x%x\n", wfso);
    xlStatus = XL_SUCCESS;

    while (!xlStatus) {

      xlStatus = xlFrReceive(pTh->xlPortHandle, &xlFrEvent);    
      //printf("xlFrReceive, status: %d\n", xlStatus);

      if ( (xlStatus != XL_ERR_QUEUE_IS_EMPTY) && (xlStatus != XL_ERROR)) {
        viewFrEvent(&xlFrEvent);
      }  

    }   

  }
  g_bFRayThreadTerminated = TRUE;
  return NO_ERROR; 
} //RxThread
