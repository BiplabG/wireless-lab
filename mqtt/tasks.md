### TODO: Generate Session Key and share session key between IOT device and the server

### TODO: To plot the data with the timestamps.

### TODO: To fix the issue when some data is corrupted or lost in between.

### TODO: Put the hash of the data.

### TODO: Create a bidirectional acknowledgement setup from the server to the IOT device. So, it knows if it has to reconnect or start/stop sending the data.

- In the initial start phase, IOT device should send some start message. And wait for the server to respond back. Only when it gets an acknowledgement from the server, it should start sending the data to the server.
- Server should send a heartbeat signal every 30s that it is receiving data from the IOT device.
- If the IOT device does not get the heartbeat signal after 30s, it should disconnect and reconnect to the server.
- For the data flow from server to IOT device, use a different topic in the MQTT broker.
