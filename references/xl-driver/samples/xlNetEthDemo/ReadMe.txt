--------------------------------------------------------------------------------

           xlNetEthDemo - Demo application for the 'XL Family Driver Library'

                        Vector Informatik GmbH, Stuttgart

--------------------------------------------------------------------------------

Vector Informatik GmbH
Ingersheimer Straße 24
D-70499 Stuttgart

Tel.:  0711-80670-200
Fax.:  0711-80670-555
EMAIL: support@vector.com
WEB:   www.vector.com

--------------------------------------------------------------------------------

xlNetEthDemo is a small test application for the Ethernet functionality on
Vector devices. xlNetEthDemo utilizes Network-Based Mode on Vector devices.

For further information look into the 'XL Driver Library - Description.pdf' 
document.

--------------------------------------------------------------------------------

Function:

You can use the xlNetEthDemo sample to transmit and receive Ethernet packets.
This sample utilizes all important functions from the 'XL Family Driver Library'.
While the program is running you can change the options by pressing the 
appropriate key.

Options:
   ===========================================================
    XL-API Demo for Network-Based Mode Ethernet Configuration 
   ===========================================================
                      xlNetEthDemo - HELP
   -----------------------------------------------------------
    Keyboard commands:
    '1'..'9' Select Ethernet port          [MP]
    '+'      Select next Ethernet port     [MP]
    '-'      Select previous Ethernet port [MP]
    't'      Transmit single packet        [MP]
    '*'      Select next Ethernet port     [VP]
    '/'      Select previous Ethernet port [VP]
    'T'      Transmit single packet        [VP]
    's'      Current port link status
    'p'      Print INFO of last received Eth message
    'G'      Request new MAC address
    'R'      Release taken MAC address
    'm'      Set receiver MAC address
    'i'      Set receiver IP  address
    'w'      Show driver configuration
    'r'      Reset clocks on network
    'h'/'?'  Help
    'q'      Quit
   -----------------------------------------------------------

   xlNetEthDemo - Application Demo implemented in CPP to show basic usage of Vector Hardware through XL-API
                - Demo walks through needed operation: i.e. opening communication with the driver,
                  reading available devices, available networks on those devices, sending receiving data on 
                  VPs or MPs, etc.
                - As a demo, there is a control/help menu, as shown above, that provides demo for specific
                  demo function: i.e. transmit a single packet over MP / VP, check Link status,
                  show config, etc.

--------------------------------------------------------------------------------

Description:

> To compile a new exe file you will need source and header file as well as to
  import the library (vxlapi.lib). The device itself needs to be configured to Network-Based Mode.
> Demo App Flow chart:
     [1] - Initialization Phase
     [2] - Choose Network for the main part of the application (Sending/Receiving Frames)
         - Event Handling is processed only for the chosen network
           (i.e. Link Status changed, Eth Frame sent, received)
     [3] - Choose Segment in the chosen Network to add new VP
     [4] - Create and add new Virtual Port to the chosen segment
         - Demonstrated how to add Virtual Port using code,
           without use of external application (i.e. Vector Hardware Configuration application)
     [5] - Main to test Demo with options listed above
           (i.e. change MP/VP from which to send an Eth Frame)

--------------------------------------------------------------------------------