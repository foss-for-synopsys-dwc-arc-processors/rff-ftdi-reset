# rff-ftdi-reset
Reset AXS101, AXS103, HSDK and other compatible boards.

Main features:
 * It natively runs on linux (it uses libusb and libftdi)
   It can be used on windows too, but you need to install libusb driver.
 * Board to reset can be specified via usb device serial number.
   Serial number is unique for all digilent devices so it is good for determining
   specific device.
 * It can list all ftdi device with their serial numbers and other fields
   like manufacturer, product and pid/vid.

How to build:

You need to install libusb and libftdi libs and headers.
You can do it on fedora with next comand:

```
sudo dnf install libftdi-devel libusb-devel
```

After that simply run make:

```
make
```

AXS101, AXS103, HSDK and other compatible boards have following
FTDI bindings:
 * ACBUS_6 - reset output
 * ADBUS_0 - TXD
 * ADBUS_1 - RXD

