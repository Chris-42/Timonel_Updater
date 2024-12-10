<h1>Timonel Updater</h1>
based on Timonel from <a href="https://github.com/casanovg">casanovg</a><br>
improved bootloader: <a href="https://github.com/Port-Net/Timonel-bootloader">https://github.com/Port-Net/Timonel-bootloader</a><br>
The lib implements a html update point for (in my case) attiny85 based I2C modules. It allows in system updates of the micro controller code via I2C.<br>
On the AVR the Timonel bootloader is required. The micro controller code has to contain two functions from NB TWI Command Set:<br>
 GETTMNLV 0x82 /* Command Get Timonel Version<br>
 ACKTMNLV 0x7D /* Acknowledge Get Timonel Version command<br>
 the payload may contain an Application Magic character as second return like the 'T' in Timonel bootloader.<br>
 EXITTMNL 0x86 /* Command Jump to Timonel (Jump To App in Timonel)<br>
 ACKEXITT 0x79 /* Acknowledge Jump Timonel (Jump To App in Timonel) command<br>
 This command just executes a jump to reset vector: ((void (*)(void))0)();<br>
example: <a href="https://github.com/Port-Net/tiny85_i2c_hx711">https://github.com/Port-Net/tiny85_i2c_hx711</a>
