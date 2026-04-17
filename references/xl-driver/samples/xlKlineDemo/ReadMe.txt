--------------------------------------------------------------------------------

                                xlKlineDemo

                      Vector Informatik GmbH, Stuttgart

--------------------------------------------------------------------------------

Vector Informatik GmbH
Ingersheimer Straﬂe 24
70499 Stuttgart, Germany

Phone: ++49 - 711 - 80670 - 0
Fax:   ++49 - 711 - 80670 - 111


--------------------------------------------------------------------------------

xlKlineDemo is a small test application for the K-Line functionality on Vector
hardware devices. 
 
For further information look into the 'XL Driver Library - Description.pdf' 
document.

--------------------------------------------------------------------------------

Function:

You can use the xlKlineDemo sample to transmit and receive K-Line frames.
This sample utilizes all important functions from the 'XL Family Driver Library'.

You can start the program with command line arguments to change the 
initialization parameters. Type 'xlKlineDemo.exe -?' for further information.

Usage: xlKlineDemo [<function>] [<baudrate>] [<AppName>]
        <function>:   tester
        <function>:   ecufi
        <function>:   ecu5bd

Tip: If xlKlineDemo shall control both ends of the communication, open two
command windows and and launch e.g. 'xlKlineDemo tester' in the first and
'xlKlineDemo ecufi' (or ecu5bd) in the second command window.

While the program is running you can change the options by pressing the 
appropriate key. Press the key 'h' to list all options.

Keyboard commands:
 'v'      Toggle verbose mode
 't'      Transmit a message
 'T'      Transmit a long message
 'i'      Switch tester resistor
 'a'      Toggle timer rate
 'r'      Reset clock
 'G'      Generate sync pulse - K-Line
 'f'      Exec fastInit (only tester)
 '5'      Exec 5BD Init (only tester)
 'H'      Switch highspeed mode
 's'      Activate/deactivate channels
 'p'      Show hardware configuration
 'h'      Help
 'ESC'    Exit