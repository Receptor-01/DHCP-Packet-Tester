
# DHCP Flooder

![alt text](digital-flood.jpg)

## Overview

This is a DHCP flooding tool that generates and sends DHCP Discover packets at a high rate to test IPv4 network response and DHCP server behavior. This tool lets you analyze how their network handles large volumes of DHCP requests.


## Features

- Multi-threaded performance: Utilizes multiple CPU cores to send packets efficiently

- Random MAC address generation: Ensures each packet appears unique 

- UDP Broadcast Mode: Sends packets to the network's broadcast address 

- Real-time Packet Rate Display: Shows packets sent per second (pps)

- Easy Shutdown: Stops execution upon user input


### How It Works

- The script initializes a pool of DHCP Discover packets

- Multiple threads send packets using UDP broadcasts to the DHCP server on port 67

- The program tracks the packet transmission rate and displays it to the user

- Pressing enter stops the execution 


### Disclaimer
This tool was created for research and educational purposes only 😎
Feel free to contribute

