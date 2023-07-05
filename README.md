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

## Install pre-built RPM package

* RHEL/CentOS 7.x
  ```
  yum install -y epel-release
  yum install -y yum-plugin-copr
  yum copr enable -y abrodkin/rff-ftdi-reset
  yum install -y rff-ftdi-reset
  ```
* RHEL/Alma/Rocky 8.x
  ```
  dnf install -y epel-release
  dnf copr enable -y abrodkin/rff-ftdi-reset
  dnf install -y rff-ftdi-reset
  ```

## Manual build

### Install `libusb` and `libftdi` libs and headers.

* RHEL/CentOS 7.x:
  ```
  yum install -y epel-release
  yum install -y gcc make libftdi-devel
  ```
* RHEL/Alma/Rocky 8.x:
  ```
  dnf install -y epel-release
  dnf -y install gcc make libftdi-devel
  ```
* Ubuntu 18.04+:
  ```
  apt update
  apt install -y pkg-config libftdi-dev libusb-dev libusb-1.0-0-dev libftdi1-dev
  ```

### Build

As simple as it could be: `make`

## Using the utility

```
# rff-reset --help
Usage: rff-reset --serial 25164200005c --t-hold 100000
Usage: rff-reset --dev 1 --t-hold 100000
Options:
 --dev <device index>     - specify ftdi device by index (shown by rff-reset --list)
 --serial <serial number> - specify ftdi device by serial number
 --only-one               - if only one ftdi device is connected reset it. Otherwise fail with error
 --t-hold <time in uS>    - specify hold time. Default one is 1000000uS
 --list                   - find and list all connected ftdi devices
 --help                   - print this info
 --verbose                - print debug info and warnings
```

Example of running the utility on host where connected only one development board:

```
# rff-reset --only-one
```

AXS101, AXS103, HSDK and other compatible boards have following
FTDI bindings:
 * ACBUS_6 - reset output
 * ADBUS_0 - TXD
 * ADBUS_1 - RXD
