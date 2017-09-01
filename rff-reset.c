/*
 * rff-reset.c
 * Reset AXS101, AXS103, HSDK and other compatible boards.
 * This program is distributed under the GNU General Public License v2.0.
 * Author: RFF [palmur3] [Eugeniy Paltsev]
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ftdi.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>
#include <libusb.h>

#define RESET_PIN_MASK		0x40		//ACBUS6 pin mask
#define IO_BUFF_SIZE		3		//buff for ftdi_write_data: comand with 2 args
#define T_HOLD			(1 * 1000000)	//default hold time
#define USB_STR_PROP_SIZE	128		//buff for usb string properties

struct g_args_t {
	unsigned long	device;
	unsigned long	t_hold;
	unsigned long	dev_specified;
	char 		*serial;
	bool		verbose;
};

struct g_args_t g_args = {
	.t_hold = T_HOLD,
	.verbose = false,
};

#define pr_fmt(fmt) "rff: " fmt
#define rff_info(fmt, ...)	fprintf(stdout, pr_fmt(fmt), ##__VA_ARGS__)
#define rff_err(fmt, ...)	fprintf(stderr, pr_fmt(fmt), ##__VA_ARGS__)
#define rff_warn(fmt, ...)	({if (g_args.verbose) rff_err(fmt, ##__VA_ARGS__);})
#define rff_dbg(fmt, ...)	({if (g_args.verbose) rff_info(fmt, ##__VA_ARGS__);})

int find_all_ftdev(void)
{
	struct ftdi_context *ftdi;
	struct ftdi_device_list *devlist, *curdev;
	char manufacturer[USB_STR_PROP_SIZE];
	char description[USB_STR_PROP_SIZE];
	char serial_num[USB_STR_PROP_SIZE];
	struct libusb_device_descriptor desc;
	int i, ret;

	ftdi = ftdi_new();
	if (ftdi == NULL) {
		rff_err("ftdi_new failed\n");
		return -1;
	}

	ret = ftdi_usb_find_all(ftdi, &devlist, 0, 0);
	if (ret < 0) {
		rff_err("ftdi_usb_find_all fail: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
		return ret;
	}

	rff_info("number of FTDI devices found: %d\n\n", ret);

	for (i = 0, curdev = devlist; curdev != NULL; i++, curdev = curdev->next) {
		ret = libusb_get_device_descriptor(curdev->dev, &desc);
		if (ret < 0) {
			rff_err("device [%d] libusb_get_device_descriptor() failed: %d\n\n", i, ret);
			continue;
		}

		rff_info("device [%d] idVendor: %#x, idProduct %#x\n", i, desc.idVendor, desc.idProduct);

		ret = ftdi_usb_get_strings(ftdi, curdev->dev,
						manufacturer, USB_STR_PROP_SIZE,
						description, USB_STR_PROP_SIZE,
						serial_num, USB_STR_PROP_SIZE);
		if (ret < 0) {
			rff_err("device [%d] ftdi_usb_get_strings failed: %d (%s)\n", i, ret, ftdi_get_error_string(ftdi));
			continue;
		}

		rff_info("device [%d] manufacturer: %s, description: %s, serial: %s\n\n", i, manufacturer, description, serial_num);
	}
}

void print_help(void)
{
	printf("Usage: rff-reset --serial 25164200005c --t-hold 100000\n");
	printf("Usage: rff-reset --dev 1 --t-hold 100000\n");
	printf("Options:\n");
	printf(" --dev <device index>     - specify ftdi device by index (shown by rff-reset --list-dev)\n");
	printf(" --serial <serial number> - specify ftdi device by serial number\n");
	printf(" --t-hold <time in uS>    - specify hold time\n");
	printf(" --list                   - find and list all connected ftdi devices\n");
	printf(" --help                   - print this info\n");
	printf(" --verbose                - print debug info and warnings\n");
}

int parse_options(int argc, char **argv)
{
	int opt;
	long tmp;

	static const char *short_options = "d:s:t:h?vl";

	static const struct option long_options[] = {
		{"dev", required_argument, NULL, 'd'},
		{"serial", required_argument, NULL, 's'},
		{"t-hold", required_argument, NULL, 't'},
		{"help", no_argument, NULL, 'h'},
		{"list", no_argument, NULL, 'l'},
		{"verbose", no_argument, NULL, 'v'},
		{}
	};

	while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
		switch (opt) {
			case 'd':
				g_args.dev_specified++;
				tmp = strtol(optarg, NULL, 10);
				if (tmp > 0)
					g_args.device = tmp;
				break;

			case 's':
				g_args.dev_specified++;
				g_args.serial = optarg;
				break;

			case 't':
				tmp = strtol(optarg, NULL, 10);
				if (tmp > 0)
					g_args.t_hold = tmp;
				break;

			case 'v':
				g_args.verbose = true;
				break;

			case 'l':
				find_all_ftdev();
				exit(EXIT_SUCCESS);
				break;

			case 'h':
			case '?':
				print_help();
				exit(EXIT_SUCCESS);
				break;

			default:
				break;
		}
	}
}

int main(int argc, char **argv)
{
	struct ftdi_device_list *devlist, *curdev;
	char serial_num[USB_STR_PROP_SIZE] = "";
	struct ftdi_context *ftdi;
	uint8_t iobuff[IO_BUFF_SIZE];
	bool reset_ok = false;
	int ret, i, dev_index;

	parse_options(argc, argv);

	if (g_args.dev_specified == 0) {
		print_help();
		exit(EXIT_SUCCESS);
	}

	if (g_args.dev_specified > 1) {
		rff_err("You can't specify device via both index and serial number\n");
		print_help();
		exit(EXIT_FAILURE);
	}

	ftdi = ftdi_new();
	if (ftdi == NULL) {
		rff_err("ftdi_new failed\n");
		exit(EXIT_FAILURE);
	}

	ret = ftdi_usb_find_all(ftdi, &devlist, 0, 0);
	if (ret < 0) {
		rff_err("ftdi_usb_find_all fail: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
		goto err_ftdi_free;
	}

	rff_dbg("number of FTDI devices found: %d\n", ret);

	if (g_args.serial) { /* select device by serial number */
		rff_dbg("trying to use device with serial number: %s\n", g_args.serial);

		for (i = 0, curdev = devlist; curdev != NULL; curdev = curdev->next, i++) {
			ret = ftdi_usb_get_strings(ftdi, curdev->dev, NULL, 0, NULL, 0, serial_num, USB_STR_PROP_SIZE);
			if (ret)
				continue;

			ret = strncmp(g_args.serial, serial_num, USB_STR_PROP_SIZE);
			if (ret)
				continue;

			rff_dbg("ftdi: device found\n");
			break;
		}

		if (curdev == NULL) {
			rff_err("ftdi: didn't found device with serial number: %s\n", g_args.serial);
			goto err_ftdi_list_free;
		}

		dev_index = i;

	} else { /* select device by index */
		if (ret <= g_args.device) {
			rff_err("ftdi: didn't found device with index: %lu\n", g_args.device);
			goto err_ftdi_list_free;
		}

		for (i = 0, curdev = devlist; (curdev != NULL) && (i < g_args.device); i++)
			curdev = curdev->next;

		ret = ftdi_usb_get_strings(ftdi, curdev->dev, NULL, 0, NULL, 0, serial_num, USB_STR_PROP_SIZE);
		if (ret) {
			rff_err("ftdi_usb_get_strings fail: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
			goto err_ftdi_list_free;
		}

		dev_index = i;
		rff_dbg("trying to use device with index: %d, serial number: %s\n", g_args.device, serial_num);
	}

	rff_info("resetting device #%d (SN %s)...", dev_index, serial_num);
	fflush(stdout);

	ret = ftdi_usb_open_dev(ftdi, curdev->dev);
	if (ret < 0 && ret != -5) {
		rff_err("unable to open ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
		goto err_ftdi_list_free;
	}

	ret = ftdi_set_interface(ftdi, INTERFACE_A);
	if (ret)
		rff_err("fail to set interface: %d (%s)\n", ret, ftdi_get_error_string(ftdi));

	ret = ftdi_set_bitmode(ftdi, 0xFF, BITMODE_MPSSE);
	if (ret)
		rff_err("ftdi: failed to set MPSSE mode, iface: %d, err: %d\n", ftdi->interface, ret);

	iobuff[i = 0] = SET_BITS_HIGH;	//Set Data Bits High Byte
	iobuff[++i] = 0x00;		//Write LO
	iobuff[++i] = RESET_PIN_MASK;	//Output mode for RESET_PIN

	ret = ftdi_write_data(ftdi, iobuff, i + 1);
	if (ret < 0)
		rff_err("write failed on channel A, error %d (%s)\n", ret, ftdi_get_error_string(ftdi));
	else
		reset_ok = true;
	usleep(g_args.t_hold);

	iobuff[i = 0] = SET_BITS_HIGH;	//Set Data Bits High Byte
	iobuff[++i] = RESET_PIN_MASK;	//Write HI
	iobuff[++i] = RESET_PIN_MASK;	//Output mode for RESET_PIN

	/* On some boards ftdi reset output pin is connected to ftdi reset input
	 * so when we reset board we also reset ftdi chip. After reseting of
	 * ftdi chip rest fo ftdi* and libusb* funcrions will fail.
	 * But it is OK as chip and usb connection will be re-initialized after
	 * reset. So print error messagees only in verbose mode. */
	ret = ftdi_write_data(ftdi, iobuff, i + 1);
	if (ret < 0)
		rff_warn("write failed on channel A, error %d (%s)\n", ret, ftdi_get_error_string(ftdi));
	usleep(g_args.t_hold);

	printf(" %s\n", reset_ok ? "OK" : "FAIL");

	ret = ftdi_set_bitmode(ftdi, 0xFF, BITMODE_RESET);
	if (ret < 0)
		rff_warn("ftdi: failed to disable MPSSE mode, iface: %d, err: %d\n", ftdi->interface, ret);

	ret = libusb_release_interface(ftdi->usb_dev, ftdi->interface);
	if (ret < 0)
		rff_warn("libusb: failed to release interface, iface: %d, err: %d\n", ftdi->interface, ret);

	ret = libusb_attach_kernel_driver(ftdi->usb_dev, ftdi->interface);
	if (ret < 0)
		rff_warn("libusb: failed to attach device driver back, iface: %d, err: %d\n", ftdi->interface, ret);

	ret = libusb_kernel_driver_active(ftdi->usb_dev, ftdi->interface);
	if (ret > 0)
		rff_dbg("libusb: successfuly attached device driver back, iface: %d, driver number %d\n", ftdi->interface, ret);

	ftdi_usb_close(ftdi);
err_ftdi_list_free:
	ftdi_list_free(&devlist);
err_ftdi_free:
	ftdi_free(ftdi);

	exit(reset_ok ? 0 : -1);
}
