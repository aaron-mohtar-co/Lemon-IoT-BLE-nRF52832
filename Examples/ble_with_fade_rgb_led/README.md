# Lemon IoT BLE Test Board

This repository contains support files for testing the Lemon IoT BLE nRF52832 board. (https://github.com/aaron-mohtar-co/Lemon-IoT-BLE)

Many different BLE developer apps can be use to connect and view the device's BLE characterstics, however, we have found that "LightBlue" does a great job at displaying the characterstics including their descriptions, making it easier to navigate.

Power your nRF52832 device up using a 3.3V supply, or by using one of the supporting boards e.g. development board.
Open your "LightBlue" app.
Click on the "Scan" tab.
Click on "Lemon IoT - nRF52832" and click "connect".
<p align="center"><img src="https://user-images.githubusercontent.com/119821198/212781114-914425c3-bd05-4afb-b3d7-4f2a37c3afb3.jpg" width = 25% height=25%></p>

Once connected, scroll down to the bottom of the page to find the Lemon IoT BLE service and characteristics.

<p align="center"><img src="https://user-images.githubusercontent.com/119821198/212781295-8e4d53ec-064b-4ab1-b4b7-93d397702539.jpg" width = 25% height=25%></p>

To control the RGB LED, click on the RGB characteristic.
Once in, write an array of bytes which correspond to the three LED colors; Red, Green, Blue.

<p align="center"><img src="https://user-images.githubusercontent.com/119821198/212781401-7c3ac8dd-9f10-4205-8514-68c0e7bd6351.jpg" width = 25% height=25%></p>

To change the mode of the device, click on the "Device mode" characterstic.
At the time of writing, Mode = 1 has been implemented and corresponds to a test procedure, which go through all GPIO pins consectively turning them ON then OFF. This is performed five times, before going into normal mode again.
Note: P0.00 & P0.01 are conntect to a 32KHz xtal by default and not to the physical headers. P0.09 &P0.10 are connected to the NFC Antenna connector and not to the physical headers by default. P0.28 & P0.29 are UART pins and might not perform during this test as expected.

<p align="center"><img src="https://user-images.githubusercontent.com/119821198/212781785-538e0da4-f2d8-4e0a-9420-44859ca8e77a.jpg" width = 25% height=25%></p>
