/*-------------------------------------------------------------------------------------------
| File        : xlEthDemo.cpp
| Project     : Vector XL-API Command Line Example for Ethernet functions
|
| Description : Shows how to configure a Vector Ethernet device (VN56xx) for frame transmission
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2020 by Vector Informatik GmbH.  All rights reserved.
|--------------------------------------------------------------------------------------------*/

#include "windows.h"
#include "stdio.h"
//#define DYNAMIC_XLDRIVER_DLL
#include "vxlapi.h"
//#undef DYNAMIC_XLDRIVER_DLL


/* Packet structure for transmit and acknowledge packets */
#define TX_PACKET_PAYLOAD_FILEOFFSET         0
#define TX_PACKET_PAYLOAD_PACKETSIZE         4
#define TX_PACKET_PAYLOAD_DATA_OFFSET        6
#define ACK_PACKET_PAYLOAD_FILEOFFSET        0
#define ACK_PACKET_PAYLOAD_PACKETSIZE        4

#define MAX_RETRANSMISSIONS                  5

//Macros to convert to/from network byte order
#define HTONL(x)      ((((x)&0x000000ff) << 24) | (((x)&0x0000ff00) << 8) | (((x)&0x00ff0000) >> 8) | (((x)&0xff000000) >> 24))
#define NTOHL(x)      ((((x)&0x000000ff) << 24) | (((x)&0x0000ff00) << 8) | (((x)&0x00ff0000) >> 8) | (((x)&0xff000000) >> 24))
#define HTONS(x)      ((((x)&0x00ff) << 8) | (((x)&0xff00) >> 8))
#define NTOHS(x)      ((((x)&0x00ff) << 8) | (((x)&0xff00) >> 8))


/*********/
/* Types */
/*********/

//Struct of options configurable via command line
typedef struct {
  bool                 showHelp;               //If true, show help and exit
  bool                 twinkleLED;             //If true, only twinkle the status LED and exit
  unsigned int         chIndex;                //Channel index to use (1, 2, ...). 0 means "first available"
  unsigned int         chConfig;               //Channel configuration.
  bool                 transmitMode;           //True if sending requested
  char                 fileToTransmit[MAX_PATH+1]; //Name of file to transmit (empty to send test data)
  unsigned int         bytesToTransmit;        //Number of bytes to send (0 = unlimited or file size)
  bool                 receiveMode;            //True if receiving requested
  char                 fileToReceive[MAX_PATH+1]; //Name of file to receive (empty to print summary only)
  unsigned short       etherTypeTx;            //Ethertype value to use for TX packets
  unsigned short       etherTypeAck;           //Ethertype value to use for ACK packets
  unsigned char        remoteMAC[XL_ETH_MACADDR_OCTETS]; //Remote MAC address to receive from
  unsigned int         payloadSize;            //Size of packets to be sent (excluding MAC addresses/Ethertype)
  unsigned int         responseTimeout;         //Maximum time to wait for packet or packet acknowledge
  unsigned int         linkTimeout;            //Maximum time to wait for a link to come up
  bool                 forceOverwrite;         //If true, any existing receive file will be silently overwritten
  bool                 verbose;
  bool                 quitAfterTransmitReceive;//True if demo shall exit after transmission
} T_CMDLINE_OPTIONS;

//Local context for sending
typedef struct {
  bool                 burstMode;           //True while transmitting in burst mode
  unsigned int         channel;             //Ethernet channel to send on
  unsigned char        receiverMAC[XL_ETH_MACADDR_OCTETS]; //MAC address to send packets to
  unsigned int         bytesToTransmit;     //Number of bytes to send in burst mode (0 = unlimited or file size)
  unsigned int         payloadSize;         //Number of bytes to put in a packet, excluding the transmission header
  unsigned int         dataOffset;          //Current offset in input file
  HANDLE               transmitFile;        //NULL if only test data should be sent
  char                 transmitFilename[MAX_PATH+1]; //Name of file to transmit
  XLuserHandle         txHandle;            //Current user handle for TX packets
  unsigned short       etherTypeTx;         //Ethertype value to use for TX packets
  unsigned short       etherTypeAck;        //Ethertype value to use for ACK packets
  unsigned int         txError;             //Number of transmission errors
  unsigned int         retransmissions;     //Number of performed retransmissions of the current packets
  unsigned int         maxRetransmissions;  //Maximum number of retransmissions per packet
  ULONGLONG            lastTxTimestamp;     //Time when the last packet was sent
  bool                 allSent;             //All data to be sent have been sent
} T_TRANSMITCONTEXT;

//Local context for receiving
typedef struct {
  bool                 receiveMode;         //True while receiving
  unsigned int         channel;             //Ethernet channel to receive on
  HANDLE               receiveFile;         //NULL if data should not be written to a file
  char                 receiveFilename[MAX_PATH+1]; //Name of file to write received data to
  unsigned int         dataOffset;          //Expected offset of next data packet in file
  XLuserHandle         txAckHandle;         //Current user handle for ACK packets
  unsigned short       etherTypeTx;         //Ethertype value to use for TX packets
  unsigned short       etherTypeAck;        //Ethertype value to use for ACK packets
  ULONGLONG            lastRxTimestamp; //Time when the last packet was received
} T_RECVCONTEXT;

typedef struct {
  unsigned int         channelIndex;        //Index in XL-API driver config
  unsigned char        macAddr[6];          //MAC address (starting with MSB!)
  XLaccess             accessMask;          //Access mask of this channel

  bool                 linkUp;              //true while link is up
  bool                 phyBypassActive;     //true if PHY bypass is currently active (prevents sending of packets)
  unsigned int         selectedConfigMode;  //Config mode selected by user
  ULONGLONG            configTimeout;       //Time when channel config was started
} T_CHANNEL_INFO;

typedef struct {
  XLportHandle         port;                //XL-API port handle
  unsigned int         currentChannel;      //Currently selected Ethernet channel
  unsigned int         ethChannelCount;     //Number of channels in channel context
  T_CHANNEL_INFO       ethChannelInfo[XL_CONFIG_MAX_CHANNELS]; //List of Ethernet channels found
  unsigned int         ethChannelMap[XL_CONFIG_MAX_CHANNELS];  //Maps XL-API channel number to ethChannelInfo index
  unsigned int         totalChannelCount;
  XLaccess             allEthChannelMask;   //Mask of all Ethernet channels found
  XLaccess             allEthChannelInitMask;//Mask of all Ethernet channels we have init access to
  XLaccess             activeChannelMask;   //Mask of active channels
  bool                 exitOnFailure;       //True if application should exit on errors
  bool                 exitAfterTransfer;   //True if application should exit after sending or receiving completes
  bool                 verboseOutput;
} T_APP_CONTEXT;

/*************/
/* Constants */
/*************/

// Parameter defaults
static const unsigned int   defaultChannel = 0;
static const unsigned int   defaultConfig = 0;
static const unsigned int   defaultTransmitLen = 65536;
static const unsigned short defaultTxEtherType = 0x0ffe;
static const unsigned short defaultAckEtherType = 0x0fff;
static const unsigned short defaultPacketSize = XL_ETH_PAYLOAD_SIZE_MAX - TX_PACKET_PAYLOAD_DATA_OFFSET - 2;
static const unsigned char  defaultDestMAC[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; //Broadcast MAC
static const unsigned int   defaultWindowSize = 1;
static const unsigned int   defaultRetransmissions = 5;
static const unsigned int   defaultLinkTimeout = 20000;
static const unsigned int   defaultAckTimeout = 200; //ACK timeout in milliseconds
static const unsigned int   defaultResponseTimeout = 1000; //Packet receive timeout in milliseconds (maximum packet interval)
static const unsigned char  commandTransmit = 0;
static const unsigned char  commandTransmitLast = 1;
static const unsigned char  commandReceiveAck = 2;    //Receive acknowledge




//Channel configurations. 
static const struct {
  const char       *name;
  T_XL_ETH_CONFIG   config;
} ethConfigModes[] = {
  /* speed                            duplex                   connector                   phy                           clockMode                    mdiMode               brPairs */ 
  { "IEEE 100/1000 MBit/s on RJ45 connector",
    {XL_ETH_MODE_SPEED_AUTO_100_1000, XL_ETH_MODE_DUPLEX_AUTO, XL_ETH_MODE_CONNECTOR_RJ45, XL_ETH_MODE_PHY_IEEE_802_3,   XL_ETH_MODE_CLOCK_AUTO,      XL_ETH_MODE_MDI_AUTO, XL_ETH_MODE_BR_PAIR_DONT_CARE}},
  { "IEEE 100 MBit/s on RJ45 connector",
    {XL_ETH_MODE_SPEED_AUTO_100,      XL_ETH_MODE_DUPLEX_AUTO, XL_ETH_MODE_CONNECTOR_RJ45, XL_ETH_MODE_PHY_IEEE_802_3,   XL_ETH_MODE_CLOCK_DONT_CARE, XL_ETH_MODE_MDI_AUTO, XL_ETH_MODE_BR_PAIR_DONT_CARE}},
  { "IEEE 1000 MBit/s on RJ45 connector",
    {XL_ETH_MODE_SPEED_AUTO_1000,     XL_ETH_MODE_DUPLEX_AUTO, XL_ETH_MODE_CONNECTOR_RJ45, XL_ETH_MODE_PHY_IEEE_802_3,   XL_ETH_MODE_CLOCK_AUTO,      XL_ETH_MODE_MDI_AUTO, XL_ETH_MODE_BR_PAIR_DONT_CARE}},

  { "BroadR-Reach 100 MBit/s Master on D-Sub connector",
    {XL_ETH_MODE_SPEED_FIXED_100,     XL_ETH_MODE_DUPLEX_DONT_CARE, XL_ETH_MODE_CONNECTOR_DSUB, XL_ETH_MODE_PHY_BROADR_REACH, XL_ETH_MODE_CLOCK_MASTER,    XL_ETH_MODE_MDI_AUTO, XL_ETH_MODE_BR_PAIR_1PAIR}},
  { "BroadR-Reach 100 MBit/s Slave on D-Sub connector",
    {XL_ETH_MODE_SPEED_FIXED_100,     XL_ETH_MODE_DUPLEX_DONT_CARE, XL_ETH_MODE_CONNECTOR_DSUB, XL_ETH_MODE_PHY_BROADR_REACH, XL_ETH_MODE_CLOCK_SLAVE,     XL_ETH_MODE_MDI_AUTO, XL_ETH_MODE_BR_PAIR_1PAIR}},
};




/*******************/
/* Local functions */
/*******************/
static void ethDemoShowCmdLineHelp() {
  printf("Packet transmit/receive example for Vector Ethernet devices.\n\n");
  printf("Command line arguments:\n");
  printf("/h or /? - This help\n");
  printf("/dn      - XLAPI Ethernet device channel n to use (n = 1,2,...). Default: %u (0=use first Ethernet channel).\n", defaultChannel+1);
  printf("/cX      - Use channel configuration mode X (see below). Default: %u.\n", defaultConfig+1);
  printf("/t       - Transmit test pattern\n");
  printf("/t\"Name\" - Transmit content of file.\n");
  printf("/r       - Receive data\n");
  printf("/r\"Name\" - Receive data and write to file; the file must not exist.\n");
  printf("/eX,Y    - Use Ethertype X for transmission, Y for acknowledge. Default: 0x%04X,0x%04x.\n", defaultTxEtherType, defaultAckEtherType);
  printf("/pX      - Maximum transmit packet payload in bytes (42..1500). Default: %u\n", defaultPacketSize);
  printf("/lX      - Maximum transmit length (0=no limit/file size). Default: %u byte.\n", defaultTransmitLen);
  printf("/mX      - Receiver MAC address X (format: aa:bb:cc:dd:ee:ff). Default: %02X:%02X:%02X:%02X:%02X:%02X\n", defaultDestMAC[0], defaultDestMAC[1], defaultDestMAC[2], defaultDestMAC[3], defaultDestMAC[4], defaultDestMAC[5]);
  printf("/oX      - Transmit/receive timeout in milliseconds (0=Disable timeout). Default: %u.\n", defaultResponseTimeout);
  printf("/v       - verbose output\n");
  printf("/f       - force overwriting existing receive file\n");
  printf("/k       - twinkle status LED of device owning the given XLAPI channel and exit\n");
  printf("/q       - quit after transmit/receive\n");
  printf("\n");
  printf("\n");
  printf("Config modes:\n");
  for(unsigned int mode = 0; mode < _countof(ethConfigModes); ++mode) {
    printf("%2u - %s\n", mode, ethConfigModes[mode].name);
  }
  printf("\n");
}


static void ethDemoShowMenuHelp(unsigned int numChannels) {
  printf("\n");
  printf("-------------------------------------------------------------\n");
  printf("-                   xlEthDemo - HELP                        -\n");
  printf("-------------------------------------------------------------\n");
  printf("- Keyboard commands:                                        -\n");
  if(numChannels > 1) {
    printf("- '1'..'%u' Select Ethernet channel                          -\n", min(numChannels,9));
    printf("- '+'      Select next Ethernet channel                     -\n");
    printf("- '-'      Select previous Ethernet channel                 -\n");
  }
  printf("- 'a'      Activate current channel                         -\n");
  printf("- 'd'      Deactivate current channel                       -\n");
  printf("- 'c'      Set channel configuration                        -\n");
  printf("- 't'      Transmit single packet                           -\n");
  printf("- 'b'      Start burst transmission (needs active receiver) -\n");
  printf("- 's'      Stop burst transmission                          -\n");
  printf("- 'r'      Receive                                          -\n");
  printf("- 'e'      Set Ethertype to use                             -\n");
  printf("- 'p'      Set packet payload size                          -\n");
  printf("- 'l'      Set burst data length                            -\n");
  printf("- 'm'      Set receiver MAC address                         -\n");
  printf("- 'k'      Twinkle status LED of device                     -\n");
  printf("- 'w'      Show driver configuration                        -\n");
  printf("- 'v'      Toggle verbose output                            -\n");
  printf("- 'h'/'?'  Help                                             -\n");
  printf("- 'ESC'    Exit                                             -\n");
  printf("-------------------------------------------------------------\n");
  printf("\n");
}


static void ethDemoShowSettings(const T_CMDLINE_OPTIONS *options) {
  printf("Default settings:\n");
  printf("- Channel index: %u\n", options->chIndex);
  if(options->transmitMode) {
    printf("- Transmit file: %s\n", strlen(options->fileToTransmit)>0?options->fileToTransmit:"(use test pattern)");
  }
  if(options->receiveMode) {
    printf("- Receive file: %s\n", strlen(options->fileToReceive)>0?options->fileToReceive:"(check only)");
  }
  printf("- Ethertypes: 0x%04x for TX, 0x%04x for ACK\n", options->etherTypeTx, options->etherTypeAck);
  printf("- Remote MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n", 
    options->remoteMAC[0], options->remoteMAC[1], options->remoteMAC[2], 
    options->remoteMAC[3], options->remoteMAC[4], options->remoteMAC[5]);
  printf("- Payload size: %u\n", options->payloadSize);
  printf("- Transmit length: %u\n", options->bytesToTransmit);
  printf("- Receive Timeout: %ums\n", options->responseTimeout);
  printf("- Link timeout: %ums\n", options->linkTimeout);
  printf("- Overwrite mode: %s\n", options->forceOverwrite?"overwrite existing":"no overwrite");
  printf("- Verbose output: %s\n", options->verbose?"On":"Off");
}

static const char *channelStatusSpeedName(unsigned int speed) {
  const char *name = NULL;

  // values from T_XL_ETH_CHANNEL_STATUS event
  switch(speed) {
  case XL_ETH_STATUS_SPEED_UNKNOWN:     name = "unknown";         break;
  case XL_ETH_STATUS_SPEED_100:         name = "100 MBit";        break;
  case XL_ETH_STATUS_SPEED_1000:        name = "1 GBit";          break;
  default:                              name = "invalid";         break;
  }

  return name;
} //channelStatusSpeedName

static const char *channelStatusConnectorName(unsigned int connector) {
  const char *name = NULL;

  switch(connector) {
  case XL_ETH_STATUS_CONNECTOR_DSUB:    name = "DSub";            break;
  case XL_ETH_STATUS_CONNECTOR_RJ45:    name = "RJ45";            break;
  default:                              name = "inv";             break;                          
  }

  return name;
} //channelConnectorName


static const char *channelStatusClockModeName(unsigned int mode) {
  const char *name = NULL;

  switch(mode) {
  case XL_ETH_STATUS_CLOCK_DONT_CARE:   name = "DC";              break;
  case XL_ETH_STATUS_CLOCK_MASTER:      name = "Ma";              break;
  case XL_ETH_STATUS_CLOCK_SLAVE:       name = "Sl";              break;
  default:                              name = "inv";             break;                          
  }

  return name;
}

static void ethDemoShowDriverConfig() {
  XLstatus        result = XL_SUCCESS;
  XLdriverConfig  driverConfig;
  struct {
    unsigned short  type;
    const char     *name;
  } transceiverTable[XL_CONFIG_MAX_CHANNELS+1];
  memset(transceiverTable, 0, sizeof(transceiverTable));

  result = xlGetDriverConfig(&driverConfig);

  if(result != XL_SUCCESS) {
    printf("Error, failed to get driver config! (%s)\n", xlGetErrorString(result));
  }
  else {
    printf("-------------------------------------------------------------------------------\n");
    printf("%2u channels       Hardware Configuration\n", driverConfig.channelCount);
    printf("-------------------------------------------------------------------------------\n");

    printf("Ch|Mask|HwNr|HwCh|Name  |Serial  |Trcv|MAC              |Link State\n");
    printf("--+----+----+----+------+--------+----+-----------------+----------------------\n");
    for (unsigned int chIndex=0; chIndex < driverConfig.channelCount; chIndex++) {
      const XLchannelConfig *channelConfig = &driverConfig.channel[chIndex];
      char                   linkState[100];

      switch(channelConfig->busParams.busType) {
      case XL_BUS_TYPE_ETHERNET:
        switch(channelConfig->busParams.data.ethernet.link) {
        case XL_ETH_STATUS_LINK_UNKNOWN:
          strcpy_s(linkState, sizeof(linkState), "Unknown state");
          break;
        case XL_ETH_STATUS_LINK_DOWN:
          strcpy_s(linkState, sizeof(linkState), "Link down");
          break;
        case XL_ETH_STATUS_LINK_UP:
          switch(channelConfig->busParams.data.ethernet.phy) {
          case XL_ETH_STATUS_PHY_UNKNOWN:
            strcpy_s(linkState, sizeof(linkState), "Unknown PHY");
            break;
          case XL_ETH_STATUS_PHY_IEEE_802_3:
            sprintf_s(linkState, sizeof(linkState), "IEEE %s, %s",
                      channelStatusSpeedName(channelConfig->busParams.data.ethernet.speed),
                      channelStatusConnectorName(channelConfig->busParams.data.ethernet.connector));
            break;
          case XL_ETH_STATUS_PHY_BROADR_REACH:
            sprintf_s(linkState, sizeof(linkState), "BR %s (%s), %s",
                      channelStatusSpeedName(channelConfig->busParams.data.ethernet.speed),
                      channelStatusClockModeName(channelConfig->busParams.data.ethernet.clockMode),
                      channelStatusConnectorName(channelConfig->busParams.data.ethernet.connector));
            break;
          default:
            strcpy_s(linkState, sizeof(linkState), "Internal error");
            break;
          }
          break;
        case XL_ETH_STATUS_LINK_ERROR:
          strcpy_s(linkState, sizeof(linkState), "Link error");
          break;
        default:
          strcpy_s(linkState, sizeof(linkState), "Internal error");
          break;
        }

        printf("%2u|%04I64x|%4u|%4u|%-6.6s|%08u|%04x|%02x:%02x:%02x:%02x:%02x:%02x|%s\n", 
          (unsigned int)channelConfig->channelIndex+1, 
          channelConfig->channelMask,
          channelConfig->hwIndex, channelConfig->hwChannel,
          channelConfig->name,
          channelConfig->serialNumber,
          channelConfig->transceiverType, 
          channelConfig->busParams.data.ethernet.macAddr[0], channelConfig->busParams.data.ethernet.macAddr[1],
          channelConfig->busParams.data.ethernet.macAddr[2], channelConfig->busParams.data.ethernet.macAddr[3],
          channelConfig->busParams.data.ethernet.macAddr[4], channelConfig->busParams.data.ethernet.macAddr[5],
          linkState
        );

        transceiverTable[chIndex].type = channelConfig->transceiverType;
        transceiverTable[chIndex].name = channelConfig->transceiverName;
        break;
      }
    }
    printf("--+----+----+----+------+--------+----+-----------------+----------------------\n");
    printf("\n"); 

    //Sort transceiver list by increasing type number, and remove duplicate entries
    for(unsigned int chIndex = 0; chIndex < driverConfig.channelCount; ++chIndex) {
      //Swap current index with any other index that is lower
      for(unsigned int chCompare = chIndex+1; chCompare < driverConfig.channelCount; ++chCompare) {
        if(transceiverTable[chCompare].type == XL_TRANSCEIVER_TYPE_NONE) {
        }
        else if(transceiverTable[chIndex].type == transceiverTable[chCompare].type) {
          transceiverTable[chCompare].type = XL_TRANSCEIVER_TYPE_NONE;
          transceiverTable[chCompare].name = NULL;
        }
        else if(transceiverTable[chIndex].type == XL_TRANSCEIVER_TYPE_NONE) {
          transceiverTable[chIndex].type = transceiverTable[chCompare].type;
          transceiverTable[chIndex].name = transceiverTable[chCompare].name;
          transceiverTable[chCompare].type = XL_TRANSCEIVER_TYPE_NONE;
          transceiverTable[chCompare].name = NULL;
        }
        else if(transceiverTable[chCompare].type < transceiverTable[chIndex].type) {
          unsigned short  type = transceiverTable[chCompare].type;
          const char     *name = transceiverTable[chCompare].name;

          transceiverTable[chCompare].type = transceiverTable[chIndex].type;
          transceiverTable[chCompare].name = transceiverTable[chIndex].name;
          transceiverTable[chIndex].type = type;
          transceiverTable[chIndex].name = name;
        }
      }
    }

    printf("Transceiver types:\n");
    for (unsigned int chIndex=0; chIndex < driverConfig.channelCount; chIndex++) {
      if(transceiverTable[chIndex].type != XL_TRANSCEIVER_TYPE_NONE) {
        printf("%04x: %s\n", transceiverTable[chIndex].type, transceiverTable[chIndex].name);
      }
    }

    printf("\n"); 
  }
}


static const char *linkSpeed(const T_XL_ETH_CHANNEL_STATUS *status) {
  static char  result[80];
  const char  *phy = "Unknown";
  const char  *speed = "";
  const char  *duplex = "";
  const char  *connector = "";
  const char  *clock = "";
  const char  *brPair = "";

  switch(status->activePhy) {
  case XL_ETH_STATUS_PHY_IEEE_802_3:    phy = "IEEE ";  break;
  case XL_ETH_STATUS_PHY_BROADR_REACH:  phy = "BR ";    break;
  default:                              phy = NULL;     break;
  }

  switch(status->speed) {
  case XL_ETH_STATUS_SPEED_100:         speed = "100 MBit/s";    break;
  case XL_ETH_STATUS_SPEED_1000:        speed = "1000 MBit/s";   break;
  default:                              speed = "unknown speed"; break;
  }

  switch(status->duplex) {
  case XL_ETH_STATUS_DUPLEX_FULL:       duplex = " FD";    break;
  default:                              duplex = NULL;     break;
  }

  switch(status->activeConnector) {
  case XL_ETH_STATUS_CONNECTOR_RJ45:    connector = "RJ45";    break;
  case XL_ETH_STATUS_CONNECTOR_DSUB:    connector = "DSub";    break;
  default:                              connector = "unknown"; break;
  }

  switch(status->clockMode) {
  case XL_ETH_STATUS_CLOCK_DONT_CARE:   clock = NULL;           break;
  case XL_ETH_STATUS_CLOCK_MASTER:      clock = "master";       break;
  case XL_ETH_STATUS_CLOCK_SLAVE:       clock = "slave";        break;
  default:                              clock = NULL;           break;
  }

  switch(status->brPairs) {
  case XL_ETH_STATUS_BR_PAIR_1PAIR:     brPair = "1-pair"; break;
  case XL_ETH_STATUS_BR_PAIR_DONT_CARE: brPair = NULL;     break;
  }

  _snprintf_s(result, sizeof(result), _TRUNCATE, "%s%s%s, on %s%s%s%s%s%s",
    phy?phy:"", speed, duplex?duplex:"", connector,
    clock || brPair?" (":"", clock?clock:"", clock && brPair?", ":"", brPair?brPair:"", clock || brPair?")":"");

  return result;
}


static XLstatus ethDemoInitContext(const T_CMDLINE_OPTIONS &options, T_APP_CONTEXT *appContext, T_TRANSMITCONTEXT *txContext, T_RECVCONTEXT *rxContext) {
  txContext->burstMode = options.transmitMode; //Transmit from command line is always in burst mode
  txContext->channel = (unsigned)-1;
  memcpy(txContext->receiverMAC, options.remoteMAC, XL_ETH_MACADDR_OCTETS);
  txContext->bytesToTransmit = options.bytesToTransmit;
  txContext->payloadSize = options.payloadSize;
  txContext->dataOffset = 0;
  txContext->transmitFile = INVALID_HANDLE_VALUE;
  strcpy_s(txContext->transmitFilename, sizeof(txContext->transmitFilename), options.fileToTransmit);
  txContext->txHandle = 1000; //Just an arbitrary start value
  txContext->etherTypeTx = options.etherTypeTx;
  txContext->etherTypeAck = options.etherTypeAck;
  txContext->txError = 0;
  txContext->retransmissions = 0;
  txContext->maxRetransmissions = MAX_RETRANSMISSIONS;
  txContext->lastTxTimestamp = 0;

  rxContext->receiveMode = options.receiveMode;
  rxContext->channel = (unsigned)-1;
  rxContext->receiveFile = INVALID_HANDLE_VALUE;
  strcpy_s(rxContext->receiveFilename, sizeof(rxContext->receiveFilename), options.fileToReceive);
  rxContext->dataOffset = 0;
  rxContext->txAckHandle = 2000; //Just an arbitrary start value
  rxContext->etherTypeTx = options.etherTypeTx;
  rxContext->etherTypeAck = options.etherTypeAck;
  rxContext->lastRxTimestamp = 0;

  appContext->port = XL_INVALID_PORTHANDLE;
  for(unsigned int chIndex = 0; chIndex < appContext->ethChannelCount; ++chIndex) {
    T_CHANNEL_INFO *channelInfo = &appContext->ethChannelInfo[chIndex];

    channelInfo->selectedConfigMode = options.chConfig; //Default value
    channelInfo->configTimeout = 0;
  }

  if(0 == options.chIndex) {
    appContext->currentChannel = 0; //First channel
  }
  else {
    appContext->currentChannel = options.chIndex-1;
  }

  if(options.quitAfterTransmitReceive && (options.transmitMode || options.receiveMode)) {
    appContext->exitAfterTransfer = true;
    appContext->exitOnFailure = true;
  }
  appContext->verboseOutput = options.verbose;

  return XL_SUCCESS;
}

static XLstatus ethDemoGetDriverConfig(T_APP_CONTEXT *appContext) {
  XLstatus           result = XL_SUCCESS;
  XLdriverConfig     driverConfig;

  if(XL_SUCCESS != (result = xlGetDriverConfig(&driverConfig))) {
    printf("Error, could not read driver config. Reason: %s.\n", xlGetErrorString(result));
  }
  else {
    appContext->ethChannelCount = 0;

    for(unsigned int ch = 0; 
        ch < driverConfig.channelCount && appContext->ethChannelCount < _countof(appContext->ethChannelInfo); 
        ++ch) {
      if(driverConfig.channel[ch].channelBusCapabilities & XL_BUS_ACTIVE_CAP_ETHERNET) {
        T_CHANNEL_INFO *ethChannel = &appContext->ethChannelInfo[appContext->ethChannelCount];

        ethChannel->channelIndex = ch;
        ethChannel->accessMask = driverConfig.channel[ch].channelMask;
        appContext->ethChannelMap[ch] = appContext->ethChannelCount;

        switch(driverConfig.channel[ch].busParams.busType) {
        case XL_BUS_TYPE_ETHERNET:
          memcpy(ethChannel->macAddr, driverConfig.channel[ch].busParams.data.ethernet.macAddr, XL_ETH_MACADDR_OCTETS);
          ethChannel->linkUp = (driverConfig.channel[ch].busParams.data.ethernet.link == XL_ETH_STATUS_LINK_UP);
          ethChannel->phyBypassActive = (driverConfig.channel[ch].busParams.data.ethernet.bypass == XL_ETH_BYPASS_PHY);
          break;
        }
        appContext->ethChannelCount++;
        appContext->allEthChannelMask |= driverConfig.channel[ch].channelMask;
      }
      else {
        appContext->ethChannelMap[ch] = (unsigned int)-1;
      }
    }
    appContext->totalChannelCount = driverConfig.channelCount;
  }

  return result;
}

static XLstatus readCmdLineOptions(T_CMDLINE_OPTIONS *options, int argc, char* argv[]) {
  XLstatus result = XL_SUCCESS;

  for(int argIndex = 1; argIndex < argc && result == XL_SUCCESS; ++argIndex) {
    const char *arg = argv[argIndex];

    switch(arg[0]) {
    case '/':
    case '-':
      arg++;
      break;
    }

    switch(arg[0]) {
    case 'h':
    case '?':
      options->showHelp = true;
      break;
    case 'd': //device channel number
      {
        unsigned int device = 0;

        switch(sscanf_s(arg+1, "%u", &device)) {
        case 1:
          options->chIndex = device;
          break;
        default:
          result = XL_ERR_WRONG_PARAMETER;
          break;
        }
      }
      break;
    case 'c': //channel configuration
      {
        unsigned int config = 0;

        switch(sscanf_s(arg+1, "%u", &config)) {
        case 1:
          if(config < _countof(ethConfigModes)) {
            options->chConfig = config;
          }
          else {
            printf("Error, unsupported configuration mode specified!\n");
            result = XL_ERR_WRONG_PARAMETER;
          }
          break;
        default:
          result = XL_ERR_WRONG_PARAMETER;
          break;
        }
      }
      break;
    case 't': //transmit filename
      {
        options->transmitMode = true;      

        const char *filename = arg+1;
        int scanResult = 0;
        if('"' == *filename) {
          scanResult = sscanf_s(filename+1, "%[^\"]", options->fileToTransmit, sizeof(options->fileToTransmit));
        }
        else {
          scanResult = sscanf_s(filename, "%s", options->fileToTransmit, sizeof(options->fileToTransmit));
        }
      }
      break;
    case 'r': //receive filename
      {
        options->receiveMode = true;      

        const char *filename = arg+1;
        int scanResult = 0;
        if('"' == *filename) {
          scanResult = sscanf_s(filename+1, "%[^\"]", options->fileToReceive, sizeof(options->fileToReceive));
        }
        else {
          scanResult = sscanf_s(filename, "%s", options->fileToReceive, sizeof(options->fileToReceive));
        }
      }
      break;
    case 'e': //Ethertype values for TX and ACK
      {
        int ethertypeTx = 0;
        int ethertypeAck = 0;

        switch(sscanf_s(arg+1, "%i,%i", &ethertypeTx, &ethertypeAck)) {
        case 2:
          //0x05DC (=1500 dez) is the minimum value for Ethernet II frames
          if(ethertypeTx < 0x05DC || ethertypeTx > 0xffff || ethertypeAck < 0x05DC || ethertypeAck > 0xffff ||
            ethertypeTx == ethertypeAck) {
            printf("Error, invalid Ethertype given!\n");
            result = XL_ERR_WRONG_PARAMETER;
          }
          else {
            options->etherTypeTx = (unsigned short)ethertypeTx;
            options->etherTypeAck = (unsigned short)ethertypeAck;
          }
          break;
        case 1:
          //0x05DC (=1500 dez) is the minimum value for Ethernet II frames
          if(ethertypeTx < 0x05DC || ethertypeTx > 0xffff) {
            printf("Error, invalid Ethertype given!\n");
            result = XL_ERR_WRONG_PARAMETER;
          }
          else {
            options->etherTypeTx = (unsigned short)ethertypeTx;
            options->etherTypeAck = options->etherTypeTx+1;
          }
          break;
        default:
          result = XL_ERR_WRONG_PARAMETER;
          break;
        }
      }
      break;
    case 'p': //Maximum packet payload size
      {
        unsigned int payloadSize = 0;

        switch(sscanf_s(arg+1, "%u", &payloadSize)) {
        case 1:
          options->payloadSize = payloadSize;
          break;
        default:
          result = XL_ERR_WRONG_PARAMETER;
          break;
        }
      }
      break;
    case 'l': //Maximum number of bytes to transmit
      {
        unsigned int sendLen = 0;

        switch(sscanf_s(arg+1, "%u", &sendLen)) {
        case 1:
          options->bytesToTransmit = sendLen;
          break;
        default:
          result = XL_ERR_WRONG_PARAMETER;
          break;
        }
      }
      break;
    case 'm':
      {
        int          remoteMAC[XL_ETH_MACADDR_OCTETS];
        int          scanResult = 0;

        scanResult = sscanf_s(arg+1, "%2x%*1[:-]%2x%*1[:-]%2x%*1[:-]%2x%*1[:-]%2x%*1[:-]%2x", &remoteMAC[0], &remoteMAC[1], &remoteMAC[2], &remoteMAC[3], &remoteMAC[4], &remoteMAC[5]);
        switch(scanResult) {
        case 6:
          for(unsigned int octet = 0; octet < XL_ETH_MACADDR_OCTETS; ++octet) {
            if(remoteMAC[octet] <= 255) {
              options->remoteMAC[octet] = (unsigned char)remoteMAC[octet];
            }
            else {
              printf("Error, given destination MAC is invalid!\n");
              result = XL_ERR_WRONG_PARAMETER;
              break;
            }
          }
          break;
        default:
          printf("Error, destination MAC \"%s\" has wrong format!\n", arg+1);
          result = XL_ERR_WRONG_PARAMETER;
          break;
        }
      }
      break;
    case 'o': //receive timeout
      {
        unsigned int receiveTimeout = 0;

        switch(sscanf_s(arg+1, "%u", &receiveTimeout)) {
        case 1:
          options->responseTimeout = receiveTimeout;
          if(0 == options->responseTimeout) {
            options->responseTimeout = (unsigned int)-1;
          }
          break;
        default:
          result = XL_ERR_WRONG_PARAMETER;
          break;
        }
      }
      break;
    case 'f':
      options->forceOverwrite = true;
      break;
    case 'v':
      options->verbose = true;
      break;
    case 'k':
      options->twinkleLED = true;
      break;
    case 'q':
      options->quitAfterTransmitReceive = true;
      break;
    default:
      printf("Error, unknown option \"%s\"!\n", argv[argIndex]);
      result = XL_ERR_WRONG_PARAMETER;
      break;
    }
  }

  return result;
}


static XLstatus ethDemoInputString(char *resultString, const char *defaultString, const char *abortString, 
                                   unsigned int minLength, unsigned int maxLength) {
  XLstatus     result = XL_SUCCESS;
  bool         inputOk = false;
  unsigned int inputLen = 0;

  do {
    unsigned long numberOfEventsRead;
    INPUT_RECORD ir;

    ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &numberOfEventsRead);

    if(numberOfEventsRead > 0 && ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown)  {
      CHAR inputChar = ir.Event.KeyEvent.uChar.AsciiChar;
      switch(inputChar) {
     case 0:
       //Control character, ignore
       break;
     case VK_RETURN:
       if(0 == inputLen) {
         strcpy_s(resultString, maxLength, defaultString);
         inputOk = true;
       }
       else if(inputLen >= minLength && inputLen <= maxLength) {
         resultString[inputLen] = 0;
         inputOk = true;
       }
       break;
     case VK_BACK:
       if(inputLen > 0) {
         printf("%c %c", inputChar, inputChar);
         inputLen--;
       }
       break;
     case VK_ESCAPE:
       strcpy_s(resultString, maxLength, abortString);
       inputOk = true;
       break;
     default:
       if(inputLen < maxLength) {
         printf("%c", inputChar);
         resultString[inputLen] = inputChar;
         inputLen++;
       }
       break;
      } //switch(inputChar)
    }
  } while(!inputOk);

  return result;
}

static unsigned int ethDemoInputNumberGet(unsigned int defaultValue, unsigned int abortValue, unsigned int minValue, unsigned int maxValue) {
  unsigned int number = defaultValue;
  bool         inputOk = false;
  unsigned int numberRead = 0;
  unsigned int inputLen = 0;

  do {
    unsigned long numberOfEventsRead;
    INPUT_RECORD ir;

    ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &numberOfEventsRead);

    if(numberOfEventsRead > 0 && ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown)  {
      CHAR inputChar = ir.Event.KeyEvent.uChar.AsciiChar;
      switch(inputChar) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        {
          unsigned int digit = inputChar - '0';

          if(numberRead * 10 + digit <= maxValue) {
            printf("%c", inputChar);
            numberRead = numberRead * 10 + digit;
            inputLen++;
          }
        }
        break;
      case VK_RETURN:
        if(0 == inputLen) {
          number = defaultValue;
          inputOk = true;
        }
        else if(numberRead >= minValue && numberRead <= maxValue) {
          number = numberRead;
          inputOk = true;
        }
        break;
      case VK_BACK:
        if(inputLen > 0) {
          printf("%c %c", inputChar, inputChar);
          numberRead /= 10;
          inputLen--;
        }
        break;
      case VK_ESCAPE:
        number = abortValue;
        inputOk = true;
        break;
      }
    }
  } while(!inputOk);

  return number;
}

static unsigned int ethDemoInputHexNumberGet(unsigned int defaultValue, unsigned int abortValue, unsigned int minValue, unsigned int maxValue) {
  unsigned int number = defaultValue;
  bool         inputOk = false;
  unsigned int numberRead = 0;
  unsigned int inputLen = 0;

  do {
    unsigned long numberOfEventsRead;
    INPUT_RECORD ir;

    ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &numberOfEventsRead);

    if(numberOfEventsRead > 0 && ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown)  {
      CHAR inputChar = ir.Event.KeyEvent.uChar.AsciiChar;
      switch(inputChar) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        {
          unsigned int digit = inputChar - '0';

          if(numberRead * 16 + digit <= maxValue) {
            printf("%c", inputChar);
            numberRead = numberRead * 16 + digit;
            inputLen++;
          }
        }
        break;
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
        {
          unsigned int digit = inputChar - 'a' + 0x0A;

          if(numberRead * 16 + digit < maxValue) {
            printf("%c", inputChar);
            numberRead = numberRead * 16 + digit;
            inputLen++;
          }
        }
        break;
      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
        {
          unsigned int digit = inputChar - 'A' + 0x0A;

          if(numberRead * 16 + digit < maxValue) {
            printf("%c", inputChar);
            numberRead = numberRead * 16 + digit;
            inputLen++;
          }
        }
        break;
      case VK_RETURN:
        if(0 == inputLen) {
          number = defaultValue;
          inputOk = true;
        }
        else if(numberRead >= minValue && numberRead <= maxValue) {
          number = numberRead;
          inputOk = true;
        }
        break;
      case VK_BACK:
        if(inputLen > 0) {
          printf("%c %c", inputChar, inputChar);
          numberRead /= 16;
          inputLen--;
        }
        break;
      case VK_ESCAPE:
        number = abortValue;
        inputOk = true;
        break;
      }
    }
  } while(!inputOk);

  return number;
}

static ULONGLONG GetTicks() {
#ifdef _WIN64 
  return GetTickCount64();
#else
  static DWORD timerWraps = 0;
  static DWORD lastTS = 0;

  DWORD ticks = GetTickCount();
  if(ticks < lastTS) {
    timerWraps++;
  }
  lastTS = ticks;

  return (((ULONGLONG)timerWraps) << 32 | ticks);
#endif
}

static XLstatus ethDemoDisableBypass(T_APP_CONTEXT *appContext) {
  XLstatus      result = XL_SUCCESS;
  XLuserHandle  userHandle = 0x333;

  //Disable bypass for all channels where we have init access
  for(unsigned int channel = 0; channel < appContext->ethChannelCount; ++channel) {
    T_CHANNEL_INFO *channelInfo = &appContext->ethChannelInfo[channel];

    if(appContext->allEthChannelInitMask & channelInfo->accessMask) {
      XLstatus rc = xlEthSetBypass(appContext->port, channelInfo->accessMask, userHandle, XL_ETH_BYPASS_INACTIVE);

      if(rc != XL_SUCCESS) {
        printf("Error, failed to disable bypass on channel %u! (%s)", channel+1, xlGetErrorString(rc));
        result = rc;
      }
      else {
        printf("Bypass on channel %u deactivated.\n", channel+1);
        channelInfo->phyBypassActive = false;
      }
    }
  }

  return result;
}

static XLstatus ethDemoSetChannelConfig(T_APP_CONTEXT *appContext, XLaccess channelMask) {
  XLstatus      result = XL_SUCCESS;
  XLuserHandle  userHandle = 0x222;

  for(unsigned int chIndex = 0; chIndex < appContext->ethChannelCount; ++chIndex) {
    const T_CHANNEL_INFO *channelInfo = &appContext->ethChannelInfo[chIndex];

    if(channelInfo->accessMask & channelMask) {
      printf("Configuring channel %u to %s...\n", chIndex+1, ethConfigModes[channelInfo->selectedConfigMode].name);
      XLstatus rc = xlEthSetConfig(appContext->port, appContext->ethChannelInfo[chIndex].accessMask, 
                                   userHandle, &ethConfigModes[channelInfo->selectedConfigMode].config);

      if(rc != XL_SUCCESS) {
        printf("Error, failed to set config mode to channel %u! (%s)\n", chIndex+1, xlGetErrorString(rc));
        result = rc;
      }
    }
  }
  return result;
}

static XLstatus transmitAck(T_APP_CONTEXT *appContext, T_RECVCONTEXT *rxContext, const T_XL_ETH_DATAFRAME_RX *rxFrame) {
  XLstatus               result = XL_SUCCESS;
  T_XL_ETH_DATAFRAME_TX  ackFrame;

  memset(&ackFrame, 0, sizeof(ackFrame));
  ackFrame.flags = 0;
  memcpy(ackFrame.destMAC, rxFrame->sourceMAC, XL_ETH_MACADDR_OCTETS);
  //Source MAC will be set by device since no XL_ETH_DATAFRAME_FLAGS_USE_SOURCE_MAC flag set
  ackFrame.frameData.ethFrame.etherType = HTONS(rxContext->etherTypeAck);
  ackFrame.dataLen += 2;
  *((unsigned int*)&ackFrame.frameData.ethFrame.payload[ACK_PACKET_PAYLOAD_FILEOFFSET]) = *((unsigned int*)&rxFrame->frameData.ethFrame.payload[TX_PACKET_PAYLOAD_FILEOFFSET]); //Copy file offset
  ackFrame.dataLen += 4;
  *((unsigned short*)&ackFrame.frameData.ethFrame.payload[ACK_PACKET_PAYLOAD_PACKETSIZE]) = *((unsigned short*)&rxFrame->frameData.ethFrame.payload[TX_PACKET_PAYLOAD_PACKETSIZE]); //Copy packet length
  ackFrame.dataLen += 2;

  //Add padding to reach minimum frame length
  if(ackFrame.dataLen < XL_ETH_PAYLOAD_SIZE_MIN+2) {
    ackFrame.dataLen = XL_ETH_PAYLOAD_SIZE_MIN+2; //Two octets required for Ethertype
  }

  switch(result = xlEthTransmit(appContext->port, appContext->ethChannelInfo[rxContext->channel].accessMask, 
                                rxContext->txAckHandle, &ackFrame)) {
  case XL_SUCCESS:
    printf("Channel %u: Transmit ACK at offset %u, len %u (frame size %u)\n", rxContext->channel+1,
      NTOHL(*((unsigned int*)&ackFrame.frameData.ethFrame.payload[ACK_PACKET_PAYLOAD_FILEOFFSET])),
      NTOHS(*((unsigned short*)&rxFrame->frameData.ethFrame.payload[TX_PACKET_PAYLOAD_PACKETSIZE])),
      ackFrame.dataLen);
    break;
  case XL_ERR_QUEUE_IS_FULL:
    //Do nothing - the sender will repeat this packet
    result = XL_SUCCESS;
    break;
  default:
    printf("Error, could not send acknowledge (reason: %u).\n", result);
    break;
  }

  return result;
}

static XLstatus writeReceiveData(T_RECVCONTEXT *rxContext, unsigned int offset, const unsigned char *payload, unsigned int length) {
  XLstatus                result = XL_SUCCESS;

  if(rxContext->receiveFile != INVALID_HANDLE_VALUE && rxContext->dataOffset == offset) {
    DWORD  bytesWritten = 0;

    if(!WriteFile(rxContext->receiveFile, payload, length, &bytesWritten, NULL)) {
      printf("Error, failed to write data at offset %u to file. Cause: %u\n", offset, GetLastError());
      result = XL_ERROR;
    }
    else if(bytesWritten < length) {
      printf("Error, failed to write all %u bytes (written: %u bytes).\n", length, bytesWritten);
      result = XL_ERROR;
      LONG   distanceToMoveHigh = 0;
      if(((INVALID_SET_FILE_POINTER == SetFilePointer(rxContext->receiveFile, offset, &distanceToMoveHigh, FILE_BEGIN))) && 
        (NO_ERROR != GetLastError())) {
          printf("Error, failed to set file pointer to offset %u. Cause: %u\n", offset, GetLastError());
          result = XL_ERROR;
      }
    }
    else {
      rxContext->dataOffset += length;
    }
  }

  return result;
}

static XLstatus handleTransmit(T_APP_CONTEXT *appContext, T_TRANSMITCONTEXT *txContext, unsigned int channel) {
  XLstatus               result = XL_SUCCESS;

  if(!appContext->ethChannelInfo[txContext->channel].linkUp) {
    printf("Waiting for link on channel %u to come up...\n", channel+1);
    txContext->lastTxTimestamp = GetTicks();
    txContext->retransmissions++;
  }
  else if((0 == txContext->bytesToTransmit || txContext->dataOffset <= txContext->bytesToTransmit)) {
    T_XL_ETH_DATAFRAME_TX      txFrame;
    DWORD                      bytesRead = 0;  //Bytes read from file (or generated)
    unsigned int               payloadLen = 0; //Effective number of bytes from file transmitted

    memset(&txFrame, 0, sizeof(txFrame));
    if(txContext->payloadSize + TX_PACKET_PAYLOAD_DATA_OFFSET + 2 > sizeof(txFrame.frameData.ethFrame.payload)) {
      payloadLen = sizeof(txFrame.frameData.ethFrame.payload) - TX_PACKET_PAYLOAD_DATA_OFFSET - 2;
    }
    else {
      payloadLen = txContext->payloadSize;
    }

    if(0 == txContext->bytesToTransmit) {
    }
    else if(txContext->dataOffset >= txContext->bytesToTransmit) {
      //Send a termination packet only
      payloadLen = 0;
      txContext->allSent = true;
    }
    else if(payloadLen > txContext->bytesToTransmit - txContext->dataOffset) {
      payloadLen = txContext->bytesToTransmit - txContext->dataOffset;
    }

    if(txContext->transmitFile != INVALID_HANDLE_VALUE) {
      if(!ReadFile(txContext->transmitFile, &txFrame.frameData.ethFrame.payload[TX_PACKET_PAYLOAD_DATA_OFFSET], 
                   payloadLen, &bytesRead, NULL)) {
        printf("Error, failed to read from input file (cause %u)!\n", GetLastError());
        result = XL_ERROR;
      }
      else {
        //End of file
        payloadLen = bytesRead;
        if(0 == bytesRead) {
          txContext->allSent = true;
        }
        txFrame.dataLen = (unsigned short)(TX_PACKET_PAYLOAD_DATA_OFFSET + payloadLen + 2);
      }
    }
    else {
      for(bytesRead = 0; bytesRead < payloadLen; ++bytesRead) {
        txFrame.frameData.ethFrame.payload[TX_PACKET_PAYLOAD_DATA_OFFSET + bytesRead] = (unsigned char)((txContext->dataOffset + bytesRead) & 0xff);
      }
      txFrame.dataLen = (unsigned short)(TX_PACKET_PAYLOAD_DATA_OFFSET + payloadLen + 2);
    }
    *((unsigned int*)&txFrame.frameData.ethFrame.payload[TX_PACKET_PAYLOAD_FILEOFFSET]) = HTONL(txContext->dataOffset);
    *((unsigned short*)&txFrame.frameData.ethFrame.payload[TX_PACKET_PAYLOAD_PACKETSIZE]) = HTONS(payloadLen);

    //For short frames add necessary padding
    if(txFrame.dataLen < XL_ETH_PAYLOAD_SIZE_MIN + 2) {
      txFrame.dataLen = XL_ETH_PAYLOAD_SIZE_MIN + 2; 
    }

    if(XL_SUCCESS == result) {
      printf("Channel %u: Transmit data at offset %u, len %u (frame size %u)\n", txContext->channel+1,
        txContext->dataOffset, bytesRead, txFrame.dataLen);

      txFrame.flags = 0;
      memcpy(&txFrame.destMAC, &txContext->receiverMAC, XL_ETH_MACADDR_OCTETS);
      txFrame.frameData.ethFrame.etherType = HTONS(txContext->etherTypeTx);

      switch(result = xlEthTransmit(appContext->port, appContext->ethChannelInfo[channel].accessMask, 
                                    txContext->txHandle, &txFrame)) {
      case XL_SUCCESS:
        txContext->retransmissions++;
        break;
      case XL_ERR_QUEUE_IS_FULL:
        //Wait for timeout to try again
        break;
      default:
        printf("Error, could not send data (reason: %u).\n", result);
        break;
      }

    }
  }

  return result;
}

static XLstatus startTransmit(T_APP_CONTEXT *appContext, T_TRANSMITCONTEXT *txContext, unsigned int channel) {
  XLstatus result = XL_SUCCESS;

  if(!txContext->burstMode) {
    txContext->channel = channel;
    if(txContext->dataOffset >= txContext->bytesToTransmit || txContext->allSent) {
      txContext->dataOffset = 0;
      if(txContext->transmitFile != INVALID_HANDLE_VALUE) {
        //Same filename as before -> just reset length to zero
        if(((INVALID_SET_FILE_POINTER == SetFilePointer(txContext->transmitFile, txContext->dataOffset, NULL, FILE_BEGIN))) && 
            (NO_ERROR != GetLastError())) {
          printf("Error, failed to set file pointer to offset 0. Cause: %u\n", GetLastError());
          result = XL_ERROR;
        }
      }
    }
    txContext->retransmissions = 0;
    txContext->allSent = false;

    result = handleTransmit(appContext, txContext, channel);
  }
  else {
    printf("Cannot start transmit, burst transmit is still active!\n");
  }

  return result;
}

static XLstatus startBurstTransmit(T_APP_CONTEXT *appContext, T_TRANSMITCONTEXT *txContext, unsigned int channel, 
                                   const char *transmitFilename) {
  XLstatus result = XL_SUCCESS;

  if(!txContext->burstMode) {
    HANDLE   transmitFile = INVALID_HANDLE_VALUE;

    //Check if file exists and is readable
    if(0 == strlen(transmitFilename)) {
      //Use test pattern
      if(txContext->transmitFile != INVALID_HANDLE_VALUE) {
        CloseHandle(txContext->transmitFile);
        txContext->transmitFile = INVALID_HANDLE_VALUE;
      }
    }
    else if(0 == _stricmp(transmitFilename, txContext->transmitFilename) && 
            txContext->transmitFile != INVALID_HANDLE_VALUE) {
      //Same filename as before -> just reset length to zero
      if(((INVALID_SET_FILE_POINTER == SetFilePointer(txContext->transmitFile, 0, NULL, FILE_BEGIN))) && 
          (NO_ERROR != GetLastError())) {
        printf("Error, failed to set file pointer to offset 0. Cause: %u\n", GetLastError());
        result = XL_ERROR;
      }
    }
    else if(strlen(transmitFilename) > 0 &&
            INVALID_HANDLE_VALUE == (transmitFile = CreateFileA(transmitFilename,
                                                                GENERIC_READ,
                                                                FILE_SHARE_READ,
                                                                NULL,
                                                                OPEN_EXISTING,
                                                                FILE_FLAG_SEQUENTIAL_SCAN,
                                                                NULL))) {
      printf("Error, failed to open file \"%s\" for reading (cause %u).\n", transmitFilename, GetLastError());
      result = XL_ERROR;
    }
    else {
      if(txContext->transmitFile != INVALID_HANDLE_VALUE) {
        CloseHandle(txContext->transmitFile);
      }
      txContext->transmitFile = transmitFile;

      strcpy_s(txContext->transmitFilename, sizeof(txContext->transmitFilename), transmitFilename);
    }

    txContext->burstMode = true;
    txContext->channel = channel;
    txContext->dataOffset = 0;
    txContext->retransmissions = 0;
    txContext->allSent = false;

    result = handleTransmit(appContext, txContext, channel);
  }
  else {
    printf("Burst transmit is already active!\n");
  }

  return result;
}

static XLstatus stopBurstTransmit(T_TRANSMITCONTEXT *txContext) {
  XLstatus result = XL_SUCCESS;

  if(txContext->burstMode) {
    txContext->burstMode = false;
    printf("Burst transmission stopped.\n");
  }
  return result;
}

static XLstatus startReceive(T_RECVCONTEXT *rxContext, unsigned int channel,
                             const char *receiveFilename) {
  XLstatus result = XL_SUCCESS;
  HANDLE   receiveFile = INVALID_HANDLE_VALUE;

  //Check if file exists and is readable
  if((receiveFilename == NULL) || (0 == strlen(receiveFilename))) {
    //Do not write received data
    if(rxContext->receiveFile != INVALID_HANDLE_VALUE) {
      CloseHandle(rxContext->receiveFile);
      rxContext->receiveFile = INVALID_HANDLE_VALUE;
    }
  }
  else if(0 == _stricmp(receiveFilename, rxContext->receiveFilename) && rxContext->receiveFile != INVALID_HANDLE_VALUE) {
    //Same filename as before -> just reset length to zero
    if(((INVALID_SET_FILE_POINTER == SetFilePointer(rxContext->receiveFile, 0, NULL, FILE_BEGIN))) && 
       (NO_ERROR != GetLastError())) {
      printf("Error, failed to set file pointer to offset 0. Cause: %u\n", GetLastError());
      result = XL_ERROR;
    }
    else if(!SetEndOfFile(rxContext->receiveFile)) {
      printf("Error, failed to reset file length. Cause: %u\n", GetLastError());
      result = XL_ERROR;
    }
  }
  else if(INVALID_HANDLE_VALUE == (receiveFile = CreateFileA(receiveFilename,
                                                             GENERIC_WRITE,
                                                             0,
                                                             NULL,
                                                             CREATE_ALWAYS,
                                                             FILE_ATTRIBUTE_NORMAL,
                                                             NULL))) {
    printf("Error, failed to open file \"%s\" for writing (cause %u).\n", receiveFilename, GetLastError());
    result = XL_ERROR;
  }
  else {
    if(rxContext->receiveFile != INVALID_HANDLE_VALUE) {
      CloseHandle(rxContext->receiveFile);
    }
    rxContext->receiveFile = receiveFile;

    strcpy_s(rxContext->receiveFilename, sizeof(rxContext->receiveFilename), receiveFilename);
  }

  rxContext->receiveMode = true;
  rxContext->channel = channel;
  rxContext->dataOffset = 0;
  rxContext->lastRxTimestamp = 0;

  printf("Started receive on channel %u.\n", channel+1);

  return result;
}

static XLstatus handleEthernetEvent(const T_XL_ETH_EVENT *ethEvent, T_APP_CONTEXT *appContext, 
                                    T_TRANSMITCONTEXT *txContext, T_RECVCONTEXT *rxContext, bool *finished) {
  XLstatus result = XL_SUCCESS;
  unsigned int ethChannelIndex = appContext->ethChannelMap[ethEvent->channelIndex];

  switch(ethEvent->tag) {
  case XL_ETH_EVENT_TAG_FRAMERX:
    {
      const T_XL_ETH_DATAFRAME_RX *frameRxOk = &ethEvent->tagData.frameRxOk;

      if(appContext->verboseOutput) {
        printf("Got RX on Ch%u: %02X:%02X:%02X:%02X:%02X:%02X->%02X:%02X:%02X:%02X:%02X:%02X (type=0x%04x, len=%u)\n",
          ethChannelIndex+1,
          frameRxOk->sourceMAC[0], frameRxOk->sourceMAC[1], frameRxOk->sourceMAC[2], frameRxOk->sourceMAC[3], frameRxOk->sourceMAC[4], frameRxOk->sourceMAC[5], 
          frameRxOk->destMAC[0], frameRxOk->destMAC[1], frameRxOk->destMAC[2], frameRxOk->destMAC[3], frameRxOk->destMAC[4], frameRxOk->destMAC[5],
          NTOHS(frameRxOk->frameData.ethFrame.etherType), frameRxOk->dataLen - sizeof(frameRxOk->frameData.ethFrame.etherType));
      }

      if(NTOHS(frameRxOk->frameData.ethFrame.etherType) == rxContext->etherTypeTx) {
        //We got a transmit data packet
        if(!rxContext->receiveMode) {
          printf("Got transmit data packet without being receiving!\n");
        }
        else if(ethChannelIndex != rxContext->channel) {
          printf("Got transmit data packet on wrong channel %u!\n", ethChannelIndex+1);
        }
        else {
          const unsigned char  *payload = frameRxOk->frameData.ethFrame.payload;
          const unsigned int    payloadLen = frameRxOk->dataLen - 2; //dataLen includes the Ethertype field

          unsigned int offset = NTOHL(*((unsigned int*)&payload[TX_PACKET_PAYLOAD_FILEOFFSET]));
          unsigned int length = NTOHS(*((unsigned short*)&payload[TX_PACKET_PAYLOAD_PACKETSIZE]));

          if(0 == length) {
            //Last packet of transmission.
            if(XL_SUCCESS != (result = transmitAck(appContext, rxContext, frameRxOk))) {
              //Cannot send ACK. Wait for packet retransmission and try again.
              if(appContext->exitOnFailure && finished) {
                *finished = true;
              }
            }
            else {
              if(appContext->exitAfterTransfer && finished) {
                *finished = true;
              }
              rxContext->lastRxTimestamp = 0;
              printf("All data received successfully!\n");
            }
          }
          else if(payloadLen >= TX_PACKET_PAYLOAD_DATA_OFFSET + length) {
            printf("Channel %u: Receive data at offset %u, len %u (frame size %u)\n",
              ethChannelIndex+1, offset, length, frameRxOk->dataLen);
            if(XL_SUCCESS != (result = writeReceiveData(rxContext, offset, &payload[TX_PACKET_PAYLOAD_DATA_OFFSET], length))) {
              printf("Error, failed to write received data at offset %u (%u bytes). Error: %s\n",
                offset, length, xlGetErrorString(result));
              if(appContext->exitOnFailure && finished) {
                *finished = true;
              }
            }
            else if(XL_SUCCESS != (result = transmitAck(appContext, rxContext, frameRxOk))) {
              if(appContext->exitOnFailure && finished) {
                *finished = true;
              }
            }
          }
          else {
            printf("Data length error in received packet!\n");
            if(appContext->exitOnFailure && finished) {
              *finished = true;
            }
          }

        }
      }
      else if(NTOHS(frameRxOk->frameData.ethFrame.etherType) == rxContext->etherTypeAck) {
        if(ethChannelIndex != txContext->channel) {
          printf("Got ACK packet on wrong channel!\n");
        }
        else {
          const unsigned char  *payload = frameRxOk->frameData.ethFrame.payload;

          unsigned int offset = NTOHL(*((unsigned int*)&payload[ACK_PACKET_PAYLOAD_FILEOFFSET]));
          unsigned int length = NTOHS(*((unsigned short*)&payload[ACK_PACKET_PAYLOAD_PACKETSIZE]));

          //Check ACK
          if(offset == txContext->dataOffset) {
            if(length > txContext->payloadSize) {
              printf("Got ACK packet with invalid length!\n");
              txContext->retransmissions++;
            }
            else if(0 == length) {
              //End-of-transmission ACK
              if(txContext->allSent) {
                printf("All data sent successfully!\n");
                txContext->burstMode = false;
              }
              else {
                printf("Error, got ACK with invalid zero-length!\n");
              }
            }
            else {
              txContext->dataOffset += length;
              txContext->retransmissions = 0;
              printf("Channel %u: Receive ACK at offset %u, len %u (frame size %u)\n",
                ethChannelIndex+1, offset, length, frameRxOk->dataLen);
            }
          }
          else if(txContext->retransmissions > MAX_RETRANSMISSIONS) {
            //Failed too often -> abort
            printf("Error, got ACK for wrong data offset!\n");
            txContext->burstMode = false;
            if(finished){
              *finished = true;
            }
          }

          if(txContext->burstMode && !txContext->allSent) {
            if(XL_SUCCESS != (result = handleTransmit(appContext, txContext, txContext->channel))) {
            }
          }
          else {
            txContext->lastTxTimestamp = 0;
          }

        }
      }
    }
    break;
  case XL_ETH_EVENT_TAG_FRAMERX_ERROR:
    {
      const T_XL_ETH_DATAFRAME_RX_ERROR *frameRxError = &ethEvent->tagData.frameRxError;

      const unsigned char *srcMac = frameRxError->sourceMAC;
      const unsigned char *destMac = frameRxError->destMAC;
      unsigned short       ethType = NTOHS(frameRxError->frameData.ethFrame.etherType);
      unsigned short       payloadLen = frameRxError->dataLen;

      printf("Got RX-ERROR(%u) on Ch%u: %02X:%02X:%02X:%02X:%02X:%02X->%02X:%02X:%02X:%02X:%02X:%02X (type=0x%04x, len=%u)\n",
        frameRxError->errorFlags, ethChannelIndex+1,
        srcMac[0], srcMac[1], srcMac[2], srcMac[3], srcMac[4], srcMac[5],
        destMac[0], destMac[1], destMac[2], destMac[3], destMac[4], destMac[5],
        NTOHS(ethType), payloadLen - sizeof(ethType));
    }
    break;
  case XL_ETH_EVENT_TAG_FRAMETX_ERROR: 
    {
      const T_XL_ETH_DATAFRAME_TX_ERROR *frameTxError = &ethEvent->tagData.frameTxError;

      const unsigned char *srcMac = frameTxError->txFrame.sourceMAC;
      const unsigned char *destMac = frameTxError->txFrame.destMAC;
      unsigned short       ethType = NTOHS(frameTxError->txFrame.frameData.ethFrame.etherType);
      unsigned short       payloadLen = frameTxError->txFrame.dataLen;

      printf("Got TX-ERROR(%u) on Ch%u: %02X:%02X:%02X:%02X:%02X:%02X->%02X:%02X:%02X:%02X:%02X:%02X (type=0x%04x, len=%u)\n",
        frameTxError->errorType, ethChannelIndex+1,
        srcMac[0], srcMac[1], srcMac[2], srcMac[3], srcMac[4], srcMac[5],
        destMac[0], destMac[1], destMac[2], destMac[3], destMac[4], destMac[5],
        NTOHS(ethType), payloadLen - sizeof(ethType)); //

    }
    break;
  case XL_ETH_EVENT_TAG_FRAMETX_ACK:
    {
      txContext->lastTxTimestamp = GetTicks();
      txContext->txHandle++;
    }
    break;
  case XL_ETH_EVENT_TAG_FRAMETX_ACK_SWITCH:
  case XL_ETH_EVENT_TAG_FRAMETX_ERROR_SWITCH:
    //These events are only received in MAC bypass mode and indicate that a packet has passed the switch (and was or was not successfully sent).
    break;
  case XL_ETH_EVENT_TAG_CHANNEL_STATUS:
    {
      const T_XL_ETH_CHANNEL_STATUS *channelStatus = &ethEvent->tagData.channelStatus;

      switch(channelStatus->link) {
      case XL_ETH_STATUS_LINK_DOWN:
        printf("Link status: channel %u is DOWN.\n", ethChannelIndex+1);
        appContext->ethChannelInfo[ethChannelIndex].linkUp = false;
        break;
      case XL_ETH_STATUS_LINK_UP:
        printf("Link status: channel %u is UP with %s.\n", ethChannelIndex+1, linkSpeed(channelStatus));
        appContext->ethChannelInfo[ethChannelIndex].linkUp = true;
        appContext->ethChannelInfo[ethChannelIndex].configTimeout = 0;
        break;
      default:
        break;
      }
    }
    break;
  case XL_ETH_EVENT_TAG_CONFIGRESULT:
    {
      const T_XL_ETH_CONFIG_RESULT *configResult = &ethEvent->tagData.configResult;

      switch(configResult->result) {
      case 0:
        printf("Configuration for channel %u successful!\n", ethChannelIndex+1);
        break;
      default:
        printf("Configuration for channel %u failed - reason: %u\n", ethChannelIndex+1, configResult->result);
        *finished = true;
        break;
      }
    }
    break;
  case XL_ETH_EVENT_TAG_ERROR:
    printf("Unknown error for channel %u - abort\n", ethChannelIndex+1);
    *finished = true;
    break;
  }

  return result;
}

static XLstatus handleXLApiEvent(T_APP_CONTEXT *appContext,T_TRANSMITCONTEXT *txContext, T_RECVCONTEXT *rxContext, 
                                 bool *finished) {
  XLstatus result = XL_SUCCESS;
  bool     queueEmpty = false;

  do {
    T_XL_ETH_EVENT    ethEvent;

    result = xlEthReceive(appContext->port, &ethEvent);
    switch(result) {
    case XL_SUCCESS:
      if(ethEvent.channelIndex < appContext->totalChannelCount && 
         appContext->ethChannelMap[ethEvent.channelIndex] < appContext->ethChannelCount) {
        result = handleEthernetEvent(&ethEvent, appContext, txContext, rxContext, finished);
      }
      else {
        printf("Got invalid Ethernet event type 0x%04x on XLAPI channel %u\n", ethEvent.tag, ethEvent.channelIndex);
      }
      break;
    case XL_ERR_QUEUE_IS_EMPTY:
      queueEmpty = true;
      result = XL_SUCCESS;
      break;
    } //switch(result)
  } while(result == XL_SUCCESS && !queueEmpty);

  return result;
}

static XLstatus handleInputEvent(T_APP_CONTEXT *appContext, T_TRANSMITCONTEXT *txContext, 
                                 T_RECVCONTEXT *rxContext, bool *finished) {
  XLstatus       result = XL_SUCCESS;
  DWORD          numberOfEventsRead = 0;
  INPUT_RECORD   input;
  XLuserHandle   dummyUserHandle = 0x111;

  if(!ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &input, 1, &numberOfEventsRead)) {
    //Read failed
  }
  else if(numberOfEventsRead > 0 && input.EventType == KEY_EVENT && input.Event.KeyEvent.bKeyDown) {
    CHAR inputChar = input.Event.KeyEvent.uChar.AsciiChar;
    switch(inputChar) {
    case 0:      //Special keys (Ctrl, Shift etc.)
      break;
    case VK_ESCAPE: //Escape key
      if(finished) {
        *finished = true;
      }
      break;
    case '1': //Select Ethernet channel
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      if(inputChar - (unsigned)'0' < appContext->ethChannelCount + 1) {
        appContext->currentChannel = inputChar - '0' - 1;
        printf("Selected channel %u.\n", appContext->currentChannel+1);
      }
      break;
    case '+': //Select next Ethernet channel
      if(appContext->currentChannel+1 < appContext->ethChannelCount) {
        appContext->currentChannel++;
      }
      else {
        appContext->currentChannel = 0;
      }
      printf("Selected channel %u.\n", appContext->currentChannel+1);
      break;
    case '-': //Select previous Ethernet channel
      if(appContext->currentChannel > 0) {
        appContext->currentChannel--;
      }
      else {
        appContext->currentChannel = appContext->ethChannelCount-1;
      }
      printf("Selected channel %u.\n", appContext->currentChannel+1);
      break;
    case 'a': //Activate current channel
      if(!(appContext->activeChannelMask & appContext->ethChannelInfo[appContext->currentChannel].accessMask)) {
        result = xlActivateChannel(appContext->port, 
                                   appContext->ethChannelInfo[appContext->currentChannel].accessMask,
                                   XL_BUS_TYPE_ETHERNET, 
                                   XL_ACTIVATE_NONE);
        if(result != XL_SUCCESS) {
          printf("Error, could not activate channel (%s).\n", xlGetErrorString(result));
        }
        else {
          printf("Channel %u successfully activated.\n", appContext->currentChannel+1);
          appContext->activeChannelMask |= appContext->ethChannelInfo[appContext->currentChannel].accessMask;
        }
      }
      else {
        printf("Channel already activated.\n");
      }
      break;
    case 'd': //Deactivate current channel
      if(appContext->activeChannelMask & appContext->ethChannelInfo[appContext->currentChannel].accessMask) {
        result = xlDeactivateChannel(appContext->port, appContext->ethChannelInfo[appContext->currentChannel].accessMask);
        if(result != XL_SUCCESS) {
          printf("Error, could not deactivate channel (%s).\n", xlGetErrorString(result));
        }
        else {
          printf("Channel %u successfully deactivated.\n", appContext->currentChannel+1);
          appContext->activeChannelMask &= ~appContext->ethChannelInfo[appContext->currentChannel].accessMask;
        }
      }
      else {
        printf("Channel already activated.\n");
      }
      break;
    case 'c': //Set channel configuration
      {
        unsigned int selectedMode = appContext->ethChannelInfo[appContext->currentChannel].selectedConfigMode+1;

        printf("Select configuration mode (default %u):\n", selectedMode);
        for(unsigned int mode = 0; mode < _countof(ethConfigModes); ++mode) {
          printf("%u: %s%s\n", mode+1, ethConfigModes[mode].name, mode+1==selectedMode?" (default)":"");
        }
        printf("->");
        selectedMode = ethDemoInputNumberGet(selectedMode, 0, 1, _countof(ethConfigModes));
        printf("\n");
        if(selectedMode > 0) {
          appContext->ethChannelInfo[appContext->currentChannel].selectedConfigMode = selectedMode-1;
          ethDemoSetChannelConfig(appContext, appContext->ethChannelInfo[appContext->currentChannel].accessMask);
        }
        else {
          ethDemoShowMenuHelp(appContext->ethChannelCount);
        }
      }
      break;
    case 't': //Transmit
      result = startTransmit(appContext, txContext, appContext->currentChannel);
      if(result != XL_SUCCESS) {
        printf("Error, failed to start transmission (%s).\n", xlGetErrorString(result));
      }
      break;
    case 'b': //Burst Transmit
      char     transmitFilenameBuffer[MAX_PATH+1];

      printf("Select filename to to transmit, ENTER for default or ESC to use a test pattern.\n");
      printf("Default: %s\n", strlen(txContext->transmitFilename) > 0?txContext->transmitFilename:"(none)");
      printf("->");
      ethDemoInputString(transmitFilenameBuffer, txContext->transmitFilename, "", 1, MAX_PATH);
      printf("\n");
      result = startBurstTransmit(appContext, txContext, appContext->currentChannel, transmitFilenameBuffer);
      if(result != XL_SUCCESS) {
        printf("Error, failed to start burst transmission (%s).\n", xlGetErrorString(result));
      }
      break;
    case 's': //Stop burst transmission
      result = stopBurstTransmit(txContext);
      if(result != XL_SUCCESS) {
        printf("Error, failed to start transmission (%s).\n", xlGetErrorString(result));
      }
      break;
    case 'r': //Receive
      {
        char     receiveFilenameBuffer[MAX_PATH+1];

        printf("Select filename to write received data to, ENTER for default or ESC to continue without file.\n");
        printf("Default: %s\n", strlen(rxContext->receiveFilename) > 0?rxContext->receiveFilename:"(none)");
        printf("->");
        ethDemoInputString(receiveFilenameBuffer, rxContext->receiveFilename, "", 1, MAX_PATH);
        printf("\n");
        startReceive(rxContext, appContext->currentChannel, receiveFilenameBuffer);
      }
      break;
    case 'e': //Set Ethertype to use
      {
        //0x05DC is the minimum value for Ethernet II frames
        printf("Select Ethertype for TX packets (0x05DC..0xFFFF, default 0x%04x):  0x", 
               txContext->etherTypeTx);
        txContext->etherTypeTx = (unsigned short)ethDemoInputHexNumberGet(txContext->etherTypeTx, txContext->etherTypeTx, 
                                                                          0x05DC, 0xFFFF);
        rxContext->etherTypeTx = txContext->etherTypeTx;
        printf("\n");

        //0x05DC is the minimum value for Ethernet II frames
        printf("Select Ethertype for ACK packets (0x05DC..0xFFFF, default 0x%04x):  0x", 
          txContext->etherTypeAck);
        txContext->etherTypeAck = (unsigned short)ethDemoInputHexNumberGet(txContext->etherTypeAck, txContext->etherTypeAck, 
                                                                           0x05DC, 0xFFFF);
        rxContext->etherTypeAck = txContext->etherTypeAck;
        printf("\n");
        printf("Ethertype values set to 0x%04x (TX), 0x%04x (ACK)\n", txContext->etherTypeTx, rxContext->etherTypeAck);
      }
      break;
    case 'p': //Set packet payload size
      {
        printf("Select number of bytes to transmit (%u..%u; default %u): ", 
          XL_ETH_PAYLOAD_SIZE_MIN - TX_PACKET_PAYLOAD_DATA_OFFSET, 
          XL_ETH_PAYLOAD_SIZE_MAX - TX_PACKET_PAYLOAD_DATA_OFFSET, txContext->payloadSize);
        txContext->payloadSize = ethDemoInputNumberGet(txContext->payloadSize, txContext->payloadSize, 
                                                       XL_ETH_PAYLOAD_SIZE_MIN - TX_PACKET_PAYLOAD_DATA_OFFSET, 
                                                       XL_ETH_PAYLOAD_SIZE_MAX - TX_PACKET_PAYLOAD_DATA_OFFSET);
        printf("\n");
      }
      break;
    case 'l': //Set data length to send
      {
        printf("Select number of bytes to transmit (default %u, 0 for unlimited/file size): ", txContext->bytesToTransmit);
        txContext->bytesToTransmit = ethDemoInputNumberGet(txContext->bytesToTransmit, txContext->bytesToTransmit, 
                                                           0, (unsigned)-1);
        printf("\n");
      }
      break;
    case 'm': //Set receiver MAC address
      break;
    case 'k': //Twinkle status LED of device
      if(XL_SUCCESS != (result = xlEthTwinkleStatusLed(appContext->port, 
                                                       appContext->ethChannelInfo[appContext->currentChannel].accessMask, 
                                                       dummyUserHandle))) {
        printf("Error, failed to twinkle status LED! Reason: %s\n", xlGetErrorString(result));
      }
      break;
    case 'w':
      ethDemoShowDriverConfig();
      break;
    case 'v':
      appContext->verboseOutput = !appContext->verboseOutput;
      printf("Verbose output is now %s.\n", appContext->verboseOutput?"enabled":"disabled");
      break;
    case 'h': //Help
    case '?':
    case VK_RETURN:
      ethDemoShowMenuHelp(appContext->ethChannelCount);
      break;
    default:
      printf("Unknown command!\n");
      break;
    } //switch(AsciiChar)
  }

  return result;
}

static XLstatus handleConfigTimeout(T_APP_CONTEXT *appContext, unsigned int channel, bool *finished) {
  XLstatus       result = XL_SUCCESS;

  printf("Error, no link detected on channel %u!\n", channel+1);
  appContext->ethChannelInfo[channel].configTimeout = 0;

  if(appContext->exitOnFailure && finished) {
    *finished = true;
  }

  return result;
}

static XLstatus handleTxTimeout(T_APP_CONTEXT *appContext, T_TRANSMITCONTEXT *txContext, bool *finished) {
  XLstatus       result = XL_SUCCESS;

  if(appContext->ethChannelInfo[txContext->channel].linkUp) {
    if(txContext->retransmissions < txContext->maxRetransmissions) {
      result = handleTransmit(appContext, txContext, txContext->channel);
    }
    else {
      printf("Error, too many retransmissions! Transmit aborted.\n");
      txContext->lastTxTimestamp = 0;
      txContext->burstMode = false;
      txContext->allSent = true;
    }
  }
  else {
    printf("Error, TX timeout on channel %u!\n", txContext->channel+1);
    if(txContext->retransmissions < txContext->maxRetransmissions) {
      result = handleTransmit(appContext, txContext, txContext->channel);
    }
    else {
      txContext->lastTxTimestamp = 0;
      if(appContext->exitOnFailure && finished) {
        *finished = true;
      }
    }
  }


  return result;
}

static XLstatus handleRxTimeout(T_APP_CONTEXT *appContext, T_RECVCONTEXT *rxContext, bool *finished) {
  XLstatus       result = XL_SUCCESS;

  printf("Error, RX timeout on channel %u!\n", rxContext->channel+1);
  rxContext->lastRxTimestamp = 0;

  if(appContext->exitOnFailure && finished) {
    *finished = true;
  }

  return result;
}

static XLstatus handleTimeout(T_APP_CONTEXT *appContext, T_TRANSMITCONTEXT *txContext, 
                              T_RECVCONTEXT *rxContext, const T_CMDLINE_OPTIONS *options, bool *finished) {
  XLstatus       result = XL_SUCCESS;
  ULONGLONG      now = GetTicks();

  for(unsigned int chIndex = 0; chIndex < appContext->ethChannelCount; ++chIndex) {
    if(!appContext->ethChannelInfo[chIndex].linkUp && 
       appContext->ethChannelInfo[chIndex].configTimeout > 0 && appContext->ethChannelInfo[chIndex].configTimeout < now) {
      XLstatus rc = handleConfigTimeout(appContext, chIndex, finished);

      if(rc != XL_SUCCESS) {
        result = rc;
      }
    }
  }

  if(txContext->lastTxTimestamp > 0) {
    if(txContext->lastTxTimestamp + options->responseTimeout < now) {
      XLstatus rc = handleTxTimeout(appContext, txContext, finished);

      if(rc != XL_SUCCESS) {
        result = rc;
      }
    }
  }
  if(rxContext->lastRxTimestamp > 0) {
    if(rxContext->lastRxTimestamp + options->responseTimeout < now) {
      XLstatus rc = handleRxTimeout(appContext, rxContext, finished);

      if(rc != XL_SUCCESS) {
        result = rc;
      }
    }
  }

  return result;
}


int main (int argc, char* argv[]) {
  XLstatus           result = XL_SUCCESS;
  T_CMDLINE_OPTIONS          options;
  char              *appName = "xlEthSendDemo";
  XLhandle           notificationHandle = NULL;
  T_APP_CONTEXT      appContext;
  T_TRANSMITCONTEXT  txContext;
  T_RECVCONTEXT      rxContext;

  printf("------------------------------------------------------------------\n");
  printf("- xlEthDemo - Test Application for XL Family Driver API          -\n");
  printf("- (C) 2025 Vector Informatik GmbH                                -\n");
  printf("------------------------------------------------------------------\n");

  memset(&options, 0, sizeof(options));
  memset(&appContext, 0, sizeof(appContext));
  memset(&txContext, 0, sizeof(txContext));
  memset(&rxContext, 0, sizeof(rxContext));

  DWORD fdwMode;
  if (!GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &fdwMode)) {
    printf("GetConsoleMode() failed with cause %u (0x%x). Stop.\n", GetLastError(), GetLastError());
    result = XL_ERROR;
  }
  else if (!SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), fdwMode & ~ENABLE_MOUSE_INPUT)) {
    printf("SetConsoleMode() failed with cause %u (0x%x). Stop.\n", GetLastError(), GetLastError());
    result = XL_ERROR;
  }

  //Set default options
  options.showHelp = false;
  options.twinkleLED = false;
  options.chIndex = defaultChannel;
  options.chConfig = defaultConfig;
  options.transmitMode = false;
  options.fileToTransmit[0] = 0;
  options.bytesToTransmit = defaultTransmitLen;
  options.receiveMode = false;
  options.fileToReceive[0] = 0;
  options.etherTypeTx = defaultTxEtherType;
  options.etherTypeAck = defaultAckEtherType;
  memcpy(&options.remoteMAC, defaultDestMAC, XL_ETH_MACADDR_OCTETS);
  options.payloadSize = defaultPacketSize;
  options.responseTimeout = defaultResponseTimeout;
  options.linkTimeout = defaultLinkTimeout;
  options.forceOverwrite = false;
  options.verbose = true;
  options.quitAfterTransmitReceive = false;

  if(XL_SUCCESS != result) {
  }
  else if(XL_SUCCESS != (result = readCmdLineOptions(&options, argc, argv))) {
  }
  else if(options.showHelp) {
    ethDemoShowCmdLineHelp();
  }
  else if(options.transmitMode && options.receiveMode) {
    printf("Error, cannot send and receive simultaneously!\n");
  }
  else if(options.transmitMode && 0 == options.bytesToTransmit && 0 == strlen(options.fileToTransmit)) {
    printf("Error, cannot perform unlimited transmit with test data!\n");
  }
  else if(XL_SUCCESS != (result = xlOpenDriver())) {
    printf("Error, could not open driver. Reason: %s.\n", xlGetErrorString(result));
  }
  else if(XL_SUCCESS != (result = ethDemoGetDriverConfig(&appContext))) {
    if(XL_SUCCESS != (result = xlCloseDriver())) {
      printf("Error, failed to close driver! Reason: %u\n", result);
    }
  }
  else if(options.chIndex > appContext.ethChannelCount) {
    printf("Error, channel %u is not available!\n", options.chIndex);
  }
  else if(XL_SUCCESS != (result = ethDemoInitContext(options, &appContext, &txContext, &rxContext))) {
    printf("Error, failed to init application context!\n");
  }
  else {
    XLaccess           activateMask = 0;    //Mask of channels to automatically activate
    XLaccess           configMask = 0;      //Mask of channels to automatically set configuration for

    //When in transmit or receive mode
    if(options.transmitMode) {
      txContext.channel = appContext.currentChannel;
      configMask = appContext.ethChannelInfo[txContext.channel].accessMask; 
      activateMask = appContext.allEthChannelInitMask;
      appContext.allEthChannelInitMask = configMask;
    }
    else if(options.receiveMode) {
      rxContext.channel = appContext.currentChannel;
      configMask = appContext.ethChannelInfo[rxContext.channel].accessMask;
      activateMask = appContext.allEthChannelInitMask;
      appContext.allEthChannelInitMask = configMask;
    }
    else {
      //Neither transmit nor receive preselected. Use interactive mode with menu.
      configMask = appContext.allEthChannelMask; 
      activateMask = appContext.allEthChannelMask; //Activate all channels found
      appContext.allEthChannelInitMask = configMask;

      if(appContext.ethChannelCount >= 2) {
        //If we have more than one channel, start receiving on second channel (so a transmit on first channel will have a responder)
        startReceive(&rxContext, 1, NULL);
      }
    }

    if(XL_SUCCESS != (result = xlOpenPort(&appContext.port, 
                                          appName, 
                                          appContext.allEthChannelMask, 
                                          &appContext.allEthChannelInitMask, 
                                          8*1024*1024,              //8 MB receive buffer
                                          XL_INTERFACE_VERSION_V4,  //V4 is the required version for Ethernet
                                          XL_BUS_TYPE_ETHERNET))) {
      printf("Error, failed to open Ethernet port! Reason: %s\n", xlGetErrorString(result));
    }
    else if(options.twinkleLED) {
      XLaccess      twinkleMask = appContext.ethChannelInfo[appContext.currentChannel].accessMask;
      XLuserHandle  userHandle = 0x123;  //Arbitrary value for asynchronous requests, not used

      if(XL_SUCCESS != (result = xlEthTwinkleStatusLed(appContext.port, twinkleMask, userHandle))) {
        printf("Error, failed to twinkle status LED! Reason: %s\n", xlGetErrorString(result));
      }
    }
    //Set notification handle (required to use event handling via WaitForMultipleObjects)
    else if(XL_SUCCESS != (result = xlSetNotification(appContext.port, &notificationHandle, 1))) { //Notify for each single event in queue
      printf("Error, failed to set notification handle! Reason: %s\n", xlGetErrorString(result));
    }
    else if((appContext.allEthChannelInitMask & configMask) != configMask) {
      printf("Error, could not get init permission for all required channels!\n");
    }
    else if(strlen(options.fileToTransmit) > 0 &&
            INVALID_HANDLE_VALUE == (txContext.transmitFile = CreateFileA(options.fileToTransmit,
                                                                          GENERIC_READ,
                                                                          FILE_SHARE_READ,
                                                                          NULL,
                                                                          OPEN_EXISTING,
                                                                          FILE_FLAG_SEQUENTIAL_SCAN,
                                                                          NULL))) {
      printf("Error, failed to open file \"%s\" for reading (cause %u).\n", options.fileToTransmit, GetLastError());
      result = XL_ERROR;
    }
    else if(strlen(options.fileToReceive) > 0 &&
            INVALID_HANDLE_VALUE == (rxContext.receiveFile = CreateFileA(options.fileToReceive,
                                                                         GENERIC_WRITE,
                                                                         0,
                                                                         NULL,
                                                                         options.forceOverwrite?CREATE_ALWAYS:CREATE_NEW,
                                                                         FILE_ATTRIBUTE_NORMAL,
                                                                         NULL))) {
      printf("Error, failed to open file \"%s\" for writing (cause %u).\n", options.fileToReceive, GetLastError());
      result = XL_ERROR;
    }
    //Activate channel (required to receive events)
    else if(activateMask != 0 && XL_SUCCESS != (result = xlActivateChannel(appContext.port, activateMask, 
                                                                           XL_BUS_TYPE_ETHERNET, XL_ACTIVATE_NONE))) {
      printf("Error, failed to activate channel! Reason: %s\n", xlGetErrorString(result));
    }
    //Disable bypass if it is set (otherwise we might not be able to configure the port, or to transmit data)
    else if(XL_SUCCESS != (result = ethDemoDisableBypass(&appContext))) {
      appContext.activeChannelMask = activateMask;
    }
    //Set channel configuration
    else if(XL_SUCCESS != (result = ethDemoSetChannelConfig(&appContext, configMask))) {
      appContext.activeChannelMask = activateMask;
    }
    else {
      DWORD         numWaitHandles = 0;
      HANDLE        waitHandles[2] = {NULL, NULL};
      bool          demoFinished = false;

      appContext.activeChannelMask = activateMask;
      for(unsigned int chIndex = 0; chIndex < appContext.ethChannelCount; ++chIndex) {
        if((appContext.ethChannelInfo[chIndex].accessMask & appContext.allEthChannelInitMask & configMask) && 
           !appContext.ethChannelInfo[chIndex].linkUp) {
          appContext.ethChannelInfo[chIndex].configTimeout = GetTicks() + options.linkTimeout;
        }
      }

      if(!options.transmitMode && !options.receiveMode) {
        ethDemoShowSettings(&options);
        ethDemoShowDriverConfig();
        ethDemoShowMenuHelp(appContext.ethChannelCount);
      }


      waitHandles[numWaitHandles++] = GetStdHandle(STD_INPUT_HANDLE);
      waitHandles[numWaitHandles++] = notificationHandle;
      while(!demoFinished) {
        ULONGLONG     now = GetTicks();
        DWORD         waitResult = 0;
        DWORD         waitTime = INFINITE;

        //Search earliest link timeout time
        for(unsigned int chIndex = 0; chIndex < appContext.ethChannelCount; ++chIndex) {
          const ULONGLONG &configTimeout = appContext.ethChannelInfo[chIndex].configTimeout;
          
          if(configTimeout > 0) {
            if(configTimeout < now) {
              waitTime = 0;
            }
            else if(now - configTimeout < waitTime) {
              waitTime = (DWORD)(now - configTimeout);
            }
          }
        }

        //Search earliest send/acknowledge time
        if(txContext.lastTxTimestamp > 0) {
          if(txContext.lastTxTimestamp + options.responseTimeout < now) {
            waitTime = 0;
          }
          else if(txContext.lastTxTimestamp + options.responseTimeout - now < waitTime) {
            waitTime = (DWORD)(txContext.lastTxTimestamp + options.responseTimeout - now);
          }
        }
        if(rxContext.lastRxTimestamp > 0) {
          if(rxContext.lastRxTimestamp + options.responseTimeout < now) {
            waitTime = 0;
          }
          else if(rxContext.lastRxTimestamp + options.responseTimeout - now < waitTime) {
            waitTime = (DWORD)(rxContext.lastRxTimestamp + options.responseTimeout - now);
          }
        }

        waitResult = WaitForMultipleObjects(numWaitHandles, waitHandles, FALSE, waitTime);
        switch(waitResult) {
        case WAIT_OBJECT_0: //Input event (from console) received
          if(XL_SUCCESS != (result = handleInputEvent(&appContext, &txContext, &rxContext, &demoFinished))) {
          }
          break;
        case WAIT_OBJECT_0+1: //XL-API event received
          if(XL_SUCCESS != (result = handleXLApiEvent(&appContext, &txContext, &rxContext, &demoFinished))) {
          }
          break;
        case WAIT_TIMEOUT:  //The wait timeout expired. Abort test.
          if(XL_SUCCESS != (result = handleTimeout(&appContext, &txContext, &rxContext, &options, &demoFinished))) {
          }
          break;
        default:
          break;
        }
      }
    }

    if(txContext.transmitFile != INVALID_HANDLE_VALUE && !CloseHandle(txContext.transmitFile)) {
      printf("Error, failed to close file handle! Cause: %u\n", GetLastError());
      result = XL_ERROR;
    }
    if(rxContext.receiveFile != INVALID_HANDLE_VALUE && !CloseHandle(rxContext.receiveFile)) {
      printf("Error, failed to close file handle! Cause: %u\n", GetLastError());
      result = XL_ERROR;
    }
    if(XL_SUCCESS != (result = xlDeactivateChannel(appContext.port, appContext.activeChannelMask))) {
      printf("Error, failed to deactivate channel! Reason: %s\n", xlGetErrorString(result));
    }
    if(appContext.port != XL_INVALID_PORTHANDLE && XL_SUCCESS != (result = xlClosePort(appContext.port))) {
      printf("Error, failed to close port! Reason: %s\n", xlGetErrorString(result));
    }
    if(XL_SUCCESS != (result = xlCloseDriver())) {
      printf("Error, failed to close driver! Reason: %u\n", result);
    }
  }

  return result;
}