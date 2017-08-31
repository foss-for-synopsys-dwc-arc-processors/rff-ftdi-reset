/* rff-reset.c
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

#include <libusb-1.0/libusb.h>
//#include <libusb.h>

#define RESET_PIN_MASK		0x40		//ACBUS6
#define IO_BUFF_SIZE		3
#define T_HOLD			(1 * 1000000)

struct g_args_t {
	unsigned long	device;
	unsigned long	t_hold;
	bool		verbose;
};

struct g_args_t g_args = {
	.device = 0,
	.t_hold = T_HOLD,
	.verbose = false,
};

#define pr_fmt(fmt) "rff: " fmt
#define rff_info(fmt, ...)	printf(pr_fmt(fmt), ##__VA_ARGS__)
#define rff_err(fmt, ...)	fprintf(stderr, pr_fmt(fmt), ##__VA_ARGS__)
#define rff_warn(fmt, ...)	({if (g_args.verbose) rff_err(fmt, ##__VA_ARGS__);})
#define rff_dbg(fmt, ...)	({if (g_args.verbose) rff_info(fmt, ##__VA_ARGS__);})

int find_all_ftdev(void)
{
	struct ftdi_context *ftdi;
	struct ftdi_device_list *devlist, *curdev;
	char manufacturer[128], description[128];
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

	rff_info("number of FTDI devices found: %d\n", ret);

	for (i = 0, curdev = devlist; curdev != NULL; i++) {
		if (libusb_get_device_descriptor(curdev->dev, &desc) < 0)
			rff_err("device [%d] libusb_get_device_descriptor() failed", i);
		else
			rff_info("device [%d] idVendor: %#x, idProduct %#x\n", i, desc.idVendor, desc.idProduct);

		ret = ftdi_usb_get_strings(ftdi, curdev->dev, manufacturer, 128, description, 128, NULL, 0);
		if (ret < 0) {
			rff_err("device [%d] ftdi_usb_get_strings failed: %d (%s)\n", i, ret, ftdi_get_error_string(ftdi));
		} else {
			rff_info("device [%d] manufacturer: %s, description: %s\n\n", i, manufacturer, description);
		}
		curdev = curdev->next;
	}
}

void print_help(void)
{
	rff_info("Usage: rff-reset --dev 0 --t-hold 100000\n");
	rff_info("Options:\n");
	rff_info("--dev <device number>  - specify ftdi device\n");
	rff_info("--t-hold <time in uS>  - specify hold time\n");
	rff_info("--list-dev             - find and list all connected ftdi devices\n");
	rff_info("--help                 - print this info\n");
	rff_info("--verbose              - print debug info and warnings\n");
}

int parse_options(int argc, char **argv)
{
	int opt;
	long tmp;

	static const char *short_options = "d:t:h?l";

	const struct option long_options[] = {
		{"dev", required_argument, NULL, 'd'},
		{"t-hold", required_argument, NULL, 't'},
		{"help", no_argument, NULL, 'h'},
		{"list-dev", no_argument, NULL, 'l'},
		{"verbose", no_argument, NULL, 'v'},
		{}
	};

	while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
		switch (opt) {
			case 'd':
				tmp = strtol(optarg, NULL, 10);
				if (tmp > 0)
					g_args.device = tmp;
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
				exit(0);
			break;

			case 'h':
			case '?':
				print_help();
				exit(0);
			break;

			default:
			break;
		}
	}
}

int main(int argc, char **argv)
{
	struct ftdi_device_list *devlist, *curdev;
	struct ftdi_context *ftdi;
	uint8_t iobuff[IO_BUFF_SIZE];
	bool reset_ok = true;
	int ret, i;

	parse_options(argc, argv);

	ftdi = ftdi_new();
	if (ftdi == NULL) {
		rff_err("ftdi_new failed\n");
		exit(-1);
	}

	ret = ftdi_usb_find_all(ftdi, &devlist, 0, 0);
	if (ret < 0) {
		rff_err("ftdi_usb_find_all fail: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
		exit(-1);
	}

	rff_info("number of FTDI devices found: %d\n", ret);

	if (ret <= g_args.device) {
		rff_err("ftdi: didn't found %lu device\n", g_args.device);
		exit(-1);
	}

	rff_info("try to use device number %d\n", g_args.device);

	for (i = 0, curdev = devlist; (curdev != NULL) && (i < g_args.device); i++)
		curdev = curdev->next;

	ret = ftdi_set_interface(ftdi, INTERFACE_A);
	if (ret)
		rff_err("fail to set interface: %d (%s)\n", ret, ftdi_get_error_string(ftdi));

	ret = ftdi_usb_open_dev(ftdi, curdev->dev);
	if (ret < 0 && ret != -5) {
		rff_err("unable to open ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
		ftdi_free(ftdi);
		exit(-1);
	}

	ret = ftdi_set_bitmode(ftdi, 0xFF, BITMODE_MPSSE);
	if (ret)
		rff_err("ftdi: failed to set MPSSE mode, iface: %d, err: %d\n", ftdi->interface, ret);

	iobuff[i = 0] = SET_BITS_HIGH;	//Set Data Bits High Byte
	iobuff[++i] = 0x00;		//Write LO
	iobuff[++i] = RESET_PIN_MASK;	//Output mode for RESET_PIN

	ret = ftdi_write_data(ftdi, iobuff, i + 1);
	if (ret < 0) {
		reset_ok = false;
		rff_err("write failed on channel A, error %d (%s)\n", ret, ftdi_get_error_string(ftdi));
	}
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

	rff_info("reset: %s\n", reset_ok ? "OK" : "FAIL");

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
	ftdi_free(ftdi);
	ftdi_list_free(&devlist);

	exit(reset_ok ? 0 : -1);
}
