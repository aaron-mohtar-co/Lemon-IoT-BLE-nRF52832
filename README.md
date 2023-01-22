# Lemon IoT - BLE-nRF52832 module
This repository is the primary location for support files for the Lemon IoT - BLE nRF52832 module. 

The module is based on a [nRF52832 versatile bluetooth 5.3 SoC](https://www.nordicsemi.com/products/nrf52832) from Nordic Semiconductor.

## Board Files

Before you can target the Lemon IoT BLE module in Zephyr (nRF Connect SDK), board files will need to be downloaded from the [Zephyr board files](https://github.com/aaron-mohtar-co/Lemon-IoT-BLE-nRF52832/tree/main/Zephyr board files/arm) folder and placed in the \Nordic\<SDK version>\zephyr\boards\arm folder.

One target exists
* Lemon IoT NRF52832

## Serial Bootloader

If you don’t have access to an ARM programmer (Segger J-Link, Nordic Development Kit etc), then the most cost-effective method of programming the Lemon IoT BLE module is via the pre-installed serial bootloader. Information on the serial bootloader can be found in the [bootloader/](https://github.com/aaron-mohtar-co/emon-IoT-BLE-nRF52832/Bootloader/) directory.

## Examples

Both the [zephyr OS](https://github.com/zephyrproject-rtos/zephyr/tree/main/samples) and [Nordic nRF SDK](https://github.com/nrfconnect/sdk-nrf/tree/main/samples/nrf52832) contain a wide variety of examples to base your next product on. 

In addition to these examples, we have some specific examples for the Lemon IoT BLE module in the [Examples](https://github.com/aaron-mohtar-co/Lemon-IoT-BLE-nRF52832/tree/main/Examples/) folder.
