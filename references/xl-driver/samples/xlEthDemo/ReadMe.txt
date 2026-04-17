--------------------------------------------------------------------------------

             xlEthDemo - Demo application for the 'XL Family Driver Library'

                        Vector Informatik GmbH, Stuttgart

--------------------------------------------------------------------------------

Vector Informatik GmbH
Ingersheimer Straﬂe 24
D-70499 Stuttgart

Tel.:  0711-80670-200
Fax.:  0711-80670-555
EMAIL: support@vector.com
WEB:   www.vector.com

--------------------------------------------------------------------------------

xlEthdemo is a small test application for the Ethernet functionality on Vector 
devices.

For further information look into the 'XL Driver Library - Description.pdf' 
document.

--------------------------------------------------------------------------------

Function:

You can use the xlEthdemo sample to transmit and receive Ethernet packets.
This sample utilizes all important functions from the 'XL Family Driver Library'.
While the program is running you can change the options by pressing the 
appropriate key.

Options:

  xlEthDemo:
  ----------

  Keyboard commands: 

  '1..(max eth channels)' Select a Ethernet channel
  '+'                     Select next Ethernet channel
  '-'                     Select previous Ethernet channel
  'a'                     Activate current channel
  'd'                     Deactivate current channel
  'c'                     Set channel configuration
  't'                     Transmit single packet
  'b'                     Start burst transmission (needs active receiver)
  's'                     Stop burst transmission
  'r'                     Receive
  'e'                     Set Ethertype to use
  'p'                     Set packet payload size
  'l'                     Set burst data length
  'm'                     Set receiver MAC address
  'k'                     Twinkle status LED of device
  'w'                     Show driver configuration
  'v'                     Toggle verbose output
  'h'/'?'                 Help
  'ESC'                   Exit

--------------------------------------------------------------------------------

To compile a new exe file you will need source and header file as well as the 
import library.



