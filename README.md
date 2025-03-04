
# DHCP (IPV4) Flooding Tool

## Overview

This is a DHCP flooding tool that generates and sends DHCP Discover packets at a high rate to test IPv4 network response and DHCP server behavior. This tool lets you analyze how their network handles large volumes of DHCP requests.


## Features

- Multi-threaded performance: Utilizes multiple CPU cores to send packets efficiently.

- Random MAC address generation: Ensures each packet appears unique.

- UDP Broadcast Mode: Sends packets to the network's broadcast address.

- Real-time Packet Rate Display: Shows packets sent per second.

- Graceful Shutdown: Stops execution cleanly upon user input.


### How It Works

- The script initializes a pool of DHCP Discover packets.

- Multiple threads send packets using UDP broadcasts to the DHCP server on port 67.

- The program tracks the packet transmission rate and displays it to the user.

- Pressing Enter stops the execution 


### Network Testing & Usage Scenarios

- Ensure your network allows broadcast traffic. Some routers and firewalls block broadcast packets by default.


### Use Cases:

- Stress testing DHCP servers: You can evaluate how well a DHCP server handles large-scale DHCP requests.

- Network monitoring: Detect network bottlenecks or limitations.

- Security auditing: Detect DHCP starvation vulnerabilities and rogue DHCP servers.


### Disclaimer
This tool was created for research and educational purposes only 😎
Feel free to contribute

