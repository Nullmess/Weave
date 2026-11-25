/* Weave HID (FIDO report transport), CCID and HID+CCID composite device mode.
 * Only the USB transports live here; CTAP2, ISO 7816, APDUs and key handling
 * belong to FAPs, not to the firmware. No callbacks reference unloadable FAPs.
 */
#include <furi.h>
#include <string.h>
#include <furi_hal_usb_i.h>
#include <furi_hal_usb_weave.h>
#include "usb.h"
#include "usb_hid.h"

#define HID_OUT 0x01
#define HID_IN 0x81
#define CCID_OUT 0x02
#define CCID_IN 0x82
#define CCID_NOTIFY 0x83
#define CCID_CLASS 0x0B
#define CCID_DESC_TYPE 0x21
#define CCID_RX_BUFFER (FURI_HAL_WEAVE_USB_CCID_SIZE)
#define HID_RX_QUEUE 16U
#define USB_PACKET_SIZE 64U

/* CTAP HID report (64-byte input and output; no report ID). */
static const uint8_t hid_default_report[] = {
    0x06, 0xd0, 0xf1, 0x09, 0x01, 0xa1, 0x01,
    0x09, 0x20, 0x15, 0x00, 0x26, 0xff, 0x00,
    0x75, 0x08, 0x95, 0x40, 0x81, 0x02,
    0x09, 0x21, 0x15, 0x00, 0x26, 0xff, 0x00,
    0x75, 0x08, 0x95, 0x40, 0x91, 0x02, 0xc0,
};

static uint8_t hid_report[FURI_HAL_WEAVE_USB_HID_DESCRIPTOR_MAX];
static size_t hid_report_size;
static uint8_t hid_packet_size = USB_PACKET_SIZE;

struct WeaveHidDescriptor {
    struct usb_interface_descriptor iface;
    struct usb_hid_descriptor hid;
    struct usb_endpoint_descriptor in;
    struct usb_endpoint_descriptor out;
} FURI_PACKED;

/* USB CCID 1.1 class descriptor, short APDU exchange.
 * Not a credential store: the application must supply ATR/status and handle
 * all CCID command/response frames including slot/power/abort/error states.
 */
struct WeaveCcidClassDescriptor {
    uint8_t length;
    uint8_t type;
    uint16_t bcd_ccid;
    uint8_t max_slot;
    uint8_t voltages;
    uint32_t protocols;
    uint32_t default_clock;
    uint32_t maximum_clock;
    uint8_t num_clocks;
    uint32_t data_rate;
    uint32_t max_data_rate;
    uint8_t num_data_rates;
    uint32_t max_ifsd;
    uint32_t synch_protocols;
    uint32_t mechanical;
    uint32_t features;
    uint32_t max_message;
    uint8_t get_response;
    uint8_t envelope;
    uint16_t lcd_layout;
    uint8_t pin_support;
    uint8_t max_busy_slots;
} FURI_PACKED;

struct WeaveCcidDescriptor {
    struct usb_interface_descriptor iface;
    struct WeaveCcidClassDescriptor ccid;
    struct usb_endpoint_descriptor out;
    struct usb_endpoint_descriptor in;
    struct usb_endpoint_descriptor notify;
} FURI_PACKED;

struct WeaveHidConfig {
    struct usb_config_descriptor config;
    struct WeaveHidDescriptor hid;
} FURI_PACKED;
struct WeaveCcidConfig {
    struct usb_config_descriptor config;
    struct WeaveCcidDescriptor ccid;
} FURI_PACKED;
struct WeaveCompositeConfig {
    struct usb_config_descriptor config;
    struct WeaveHidDescriptor hid;
    struct WeaveCcidDescriptor ccid;
} FURI_PACKED;

#define WEAVE_HID_DESC(num) { \
    .iface = {sizeof(struct usb_interface_descriptor), USB_DTYPE_INTERFACE, num, 0, 2, \
        USB_CLASS_HID, 0, 0, NO_DESCRIPTOR}, \
    .hid = {sizeof(struct usb_hid_descriptor), USB_DTYPE_HID, VERSION_BCD(1,1,0), \
        0, 1, USB_DTYPE_HID_REPORT, sizeof(hid_default_report)}, \
    .in = {sizeof(struct usb_endpoint_descriptor), USB_DTYPE_ENDPOINT, HID_IN, \
        USB_EPTYPE_INTERRUPT, USB_PACKET_SIZE, 5}, \
    .out = {sizeof(struct usb_endpoint_descriptor), USB_DTYPE_ENDPOINT, HID_OUT, \
        USB_EPTYPE_INTERRUPT, USB_PACKET_SIZE, 5}, \
}

#define WEAVE_CCID_DESC(num) { \
    .iface = {sizeof(struct usb_interface_descriptor), USB_DTYPE_INTERFACE, num, 0, 3, \
        CCID_CLASS, 0, 0, NO_DESCRIPTOR}, \
    .ccid = { \
        sizeof(struct WeaveCcidClassDescriptor), CCID_DESC_TYPE, 0x0110, 0, 0x07, \
        0x00000003, 4000, 4000, 0, 9600, 9600, 0, 254, 0, 0, \
        0x000200BA, CCID_RX_BUFFER, 0, 0, 0, 0, 1, \
    }, \
    .out = {sizeof(struct usb_endpoint_descriptor), USB_DTYPE_ENDPOINT, CCID_OUT, \
        USB_EPTYPE_BULK, USB_PACKET_SIZE, 0}, \
    .in = {sizeof(struct usb_endpoint_descriptor), USB_DTYPE_ENDPOINT, CCID_IN, \
        USB_EPTYPE_BULK, USB_PACKET_SIZE, 0}, \
    .notify = {sizeof(struct usb_endpoint_descriptor), USB_DTYPE_ENDPOINT, CCID_NOTIFY, \
        USB_EPTYPE_INTERRUPT, 8, 32}, \
}

#define WEAVE_CONFIG_INIT(type, count) { \
    sizeof(struct usb_config_descriptor), USB_DTYPE_CONFIGURATION, sizeof(type), count, \
    1, NO_DESCRIPTOR, USB_CFG_ATTR_RESERVED | USB_CFG_ATTR_SELFPOWERED, USB_CFG_POWER_MA(100), \
}

static struct WeaveHidConfig hid_cfg = {
    .config = WEAVE_CONFIG_INIT(struct WeaveHidConfig, 1),
    .hid = WEAVE_HID_DESC(0),
};
static const struct WeaveCcidConfig ccid_cfg = {
    .config = WEAVE_CONFIG_INIT(struct WeaveCcidConfig, 1),
    .ccid = WEAVE_CCID_DESC(0),
};
static struct WeaveCompositeConfig composite_cfg = {
    .config = WEAVE_CONFIG_INIT(struct WeaveCompositeConfig, 2),
    .hid = WEAVE_HID_DESC(0),
    .ccid = WEAVE_CCID_DESC(1),
};

_Static_assert(sizeof(struct WeaveCcidClassDescriptor) == 54, "CCID 1.1 descriptor must be 54 bytes");
static const struct usb_string_descriptor manuf_desc = USB_STRING_DESC("Weave");
static const struct usb_string_descriptor hid_prod_desc = USB_STRING_DESC("Weave HID");
static const struct usb_string_descriptor ccid_prod_desc = USB_STRING_DESC("Weave CCID");
static const struct usb_string_descriptor composite_prod_desc = USB_STRING_DESC("Weave HID CCID");

/* Dedicated experimental PIDs; never spoof an existing Flipper product mode. */
#define WEAVE_DEVICE_DESCRIPTOR(pid) { \
    .bLength = sizeof(struct usb_device_descriptor), .bDescriptorType = USB_DTYPE_DEVICE, \
    .bcdUSB = VERSION_BCD(2,0,0), .bDeviceClass = 0, .bDeviceSubClass = 0, \
    .bDeviceProtocol = 0, .bMaxPacketSize0 = USB_EP0_SIZE, \
    .idVendor = 0x0483, .idProduct = pid, .bcdDevice = VERSION_BCD(1,0,2), \
    .iManufacturer = UsbDevManuf, .iProduct = UsbDevProduct, .iSerialNumber = 0, \
    .bNumConfigurations = 1, \
}
static const struct usb_device_descriptor hid_dev_desc = WEAVE_DEVICE_DESCRIPTOR(0x5751);
static const struct usb_device_descriptor ccid_dev_desc = WEAVE_DEVICE_DESCRIPTOR(0x5752);
static const struct usb_device_descriptor composite_dev_desc = WEAVE_DEVICE_DESCRIPTOR(0x5753);

/* At most one outstanding frame per class. USB ISR never calls FAP code. */
typedef struct {
    uint8_t rx[CCID_RX_BUFFER];
    uint16_t rx_length;
    uint16_t rx_accumulated;
    bool rx_ready;
    uint8_t tx[CCID_RX_BUFFER];
    uint16_t tx_length;
    uint16_t tx_offset;
    bool tx_busy;
} WeaveUsbPipe;

static usbd_device* usb_device = NULL;
static volatile bool usb_connected = false;
static volatile bool ccid_notification_busy;
static uint8_t ccid_notification[2];
static bool hid_enabled;
static bool ccid_enabled;
static uint8_t ccid_if_num;
/* Owned by the broker for the complete USB lease, including mode reinit.
 * No buffer storage in .bss when the device is in its normal CDC mode. */
typedef struct {
    WeaveUsbPipe hid_pipe;
    WeaveUsbPipe ccid_pipe;
    uint8_t hid_rx_frames[HID_RX_QUEUE][FURI_HAL_WEAVE_USB_HID_SIZE];
    uint8_t hid_rx_sizes[HID_RX_QUEUE];
} WeaveUsbBuffers;
static WeaveUsbBuffers* usb_buffers;

/* Private broker hooks; not declared in SDK headers. */
bool weave_usb_buffers_prepare(void);
void weave_usb_buffers_release(void);

bool weave_usb_buffers_prepare(void) {
    if(usb_buffers) return false;
    const int32_t lock = furi_kernel_lock();
    if(memmgr_heap_get_max_free_block() >= sizeof(WeaveUsbBuffers) + 1024U)
        usb_buffers = malloc(sizeof(WeaveUsbBuffers));
    if(lock == 0) furi_kernel_unlock();
    return usb_buffers != NULL;
}

void weave_usb_buffers_release(void) {
    /* The synchronous mode switch has detached all our callbacks. */
    furi_check(!usb_device);
    WeaveUsbBuffers* buffers = usb_buffers;
    usb_buffers = NULL;
    if(buffers) {
        memset(buffers, 0, sizeof(*buffers));
        free(buffers);
    }
}

#define hid_pipe (usb_buffers->hid_pipe)
#define ccid_pipe (usb_buffers->ccid_pipe)
#define hid_rx_frames (usb_buffers->hid_rx_frames)
#define hid_rx_sizes (usb_buffers->hid_rx_sizes)
static volatile uint8_t hid_rx_head;
static volatile uint8_t hid_rx_tail;
static volatile uint8_t hid_rx_count;

bool furi_hal_weave_usb_configure_hid(
    const uint8_t* descriptor, size_t size, uint8_t packet_size) {
    if(!descriptor) {
        descriptor = hid_default_report;
        size = sizeof(hid_default_report);
        packet_size = USB_PACKET_SIZE;
    }
    if(!size || size > sizeof(hid_report) || !packet_size ||
       packet_size > USB_PACKET_SIZE) return false;
    /* The broker calls this only while a different USB mode is active. */
    memcpy(hid_report, descriptor, size);
    hid_report_size = size;
    hid_packet_size = packet_size;
    hid_cfg.hid.hid.wDescriptorLength0 = size;
    composite_cfg.hid.hid.wDescriptorLength0 = size;
    hid_cfg.hid.in.wMaxPacketSize = packet_size;
    hid_cfg.hid.out.wMaxPacketSize = packet_size;
    composite_cfg.hid.in.wMaxPacketSize = packet_size;
    composite_cfg.hid.out.wMaxPacketSize = packet_size;
    return true;
}

static void weave_usb_init(usbd_device* dev, FuriHalUsbInterface* intf, void* ctx);
static void weave_usb_deinit(usbd_device* dev);
static void weave_usb_wakeup(usbd_device* dev);
static void weave_usb_suspend(usbd_device* dev);
static usbd_respond weave_usb_config(usbd_device* dev, uint8_t cfg);
static usbd_respond weave_usb_control(
    usbd_device* dev, usbd_ctlreq* req, usbd_rqc_callback* callback);

#define WEAVE_INTERFACE(device, product, config) { \
    .init = weave_usb_init, .deinit = weave_usb_deinit, \
    .wakeup = weave_usb_wakeup, .suspend = weave_usb_suspend, \
    .dev_descr = (struct usb_device_descriptor*)&(device), \
    .str_manuf_descr = (void*)&manuf_desc, \
    .str_prod_descr = (void*)&(product), .str_serial_descr = NULL, \
    .cfg_descr = (void*)&(config), \
}
FuriHalUsbInterface usb_weave_hid = WEAVE_INTERFACE(hid_dev_desc, hid_prod_desc, hid_cfg);
FuriHalUsbInterface usb_weave_ccid = WEAVE_INTERFACE(ccid_dev_desc, ccid_prod_desc, ccid_cfg);
FuriHalUsbInterface usb_weave_composite =
    WEAVE_INTERFACE(composite_dev_desc, composite_prod_desc, composite_cfg);

static void weave_usb_init(usbd_device* dev, FuriHalUsbInterface* intf, void* ctx) {
    UNUSED(ctx);
    furi_check(usb_buffers);
    usb_device = dev;
    if(!hid_report_size) (void)furi_hal_weave_usb_configure_hid(NULL, 0, 0);
    hid_enabled = intf != &usb_weave_ccid;
    ccid_enabled = intf != &usb_weave_hid;
    ccid_if_num = hid_enabled ? 1 : 0;
    memset(&hid_pipe, 0, sizeof(hid_pipe));
    memset(&ccid_pipe, 0, sizeof(ccid_pipe));
    hid_rx_head = 0;
    hid_rx_tail = 0;
    hid_rx_count = 0;
    usb_connected = false;
    ccid_notification_busy = false;
    usbd_reg_config(dev, weave_usb_config);
    usbd_reg_control(dev, weave_usb_control);
    usbd_connect(dev, true);
}

static void weave_usb_deinit(usbd_device* dev) {
    usb_connected = false;
    /* Remove endpoint callbacks before the broker can release the buffers. */
    (void)weave_usb_config(dev, 0);
    usbd_reg_config(dev, NULL);
    usbd_reg_control(dev, NULL);
    usb_device = NULL;
    hid_enabled = false;
    ccid_enabled = false;
}
static void weave_usb_wakeup(usbd_device* dev) {
    UNUSED(dev);
    usb_connected = true;
}
static void weave_usb_suspend(usbd_device* dev) {
    if(!usb_buffers) return;
    UNUSED(dev);
    usb_connected = false;
    hid_pipe.tx_busy = false;
    ccid_pipe.tx_busy = false;
    hid_pipe.rx_ready = false;
    ccid_pipe.rx_ready = false;
    ccid_pipe.rx_accumulated = 0;
    ccid_notification_busy = false;
    hid_rx_head = hid_rx_tail = hid_rx_count = 0;
}

static void weave_usb_hid_rx(usbd_device* dev, uint8_t event, uint8_t ep) {
    if(!usb_buffers) return;
    UNUSED(event);
    UNUSED(ep);
    uint8_t packet[USB_PACKET_SIZE];
    int32_t size = usbd_ep_read(dev, HID_OUT, packet, sizeof(packet));
    if(size > 0 && size <= hid_packet_size && hid_rx_count < HID_RX_QUEUE) {
        memcpy(hid_rx_frames[hid_rx_tail], packet, size);
        hid_rx_sizes[hid_rx_tail] = size;
        hid_rx_tail = (hid_rx_tail + 1U) % HID_RX_QUEUE;
        hid_rx_count++;
    }
}
static void weave_usb_ccid_rx(usbd_device* dev, uint8_t event, uint8_t ep) {
    if(!usb_buffers) return;
    UNUSED(event);
    UNUSED(ep);
    uint8_t packet[USB_PACKET_SIZE];
    int32_t size = usbd_ep_read(dev, CCID_OUT, packet, sizeof(packet));
    if(size <= 0 || ccid_pipe.rx_ready) return;
    if(ccid_pipe.rx_accumulated + (size_t)size > sizeof(ccid_pipe.rx)) {
        ccid_pipe.rx_accumulated = 0;
        return;
    }
    memcpy(ccid_pipe.rx + ccid_pipe.rx_accumulated, packet, size);
    ccid_pipe.rx_accumulated += size;
    if(ccid_pipe.rx_accumulated >= 10) {
        uint32_t payload = (uint32_t)ccid_pipe.rx[1] |
            ((uint32_t)ccid_pipe.rx[2] << 8) |
            ((uint32_t)ccid_pipe.rx[3] << 16) |
            ((uint32_t)ccid_pipe.rx[4] << 24);
        if(payload > sizeof(ccid_pipe.rx) - 10) {
            ccid_pipe.rx_accumulated = 0;
        } else if(ccid_pipe.rx_accumulated == payload + 10U) {
            ccid_pipe.rx_length = ccid_pipe.rx_accumulated;
            ccid_pipe.rx_accumulated = 0;
            ccid_pipe.rx_ready = true;
        } else if(ccid_pipe.rx_accumulated > payload + 10U) {
            ccid_pipe.rx_accumulated = 0;
        }
    }
}

static void weave_usb_tx_next(usbd_device* dev, WeaveUsbPipe* pipe, uint8_t endpoint) {
    if(!pipe->tx_busy) return;
    if(pipe->tx_offset < pipe->tx_length) {
        uint16_t remain = pipe->tx_length - pipe->tx_offset;
        uint16_t size = remain > USB_PACKET_SIZE ? USB_PACKET_SIZE : remain;
        uint16_t offset = pipe->tx_offset;
        pipe->tx_offset += size; /* advance before enabling the endpoint IRQ */
        usbd_ep_write(dev, endpoint, pipe->tx + offset, size);
    } else {
        pipe->tx_busy = false;
    }
}
static void weave_usb_hid_tx(usbd_device* dev, uint8_t event, uint8_t ep) {
    if(!usb_buffers) return;
    UNUSED(event);
    UNUSED(ep);
    weave_usb_tx_next(dev, &hid_pipe, HID_IN);
}
static void weave_usb_ccid_tx(usbd_device* dev, uint8_t event, uint8_t ep) {
    if(!usb_buffers) return;
    UNUSED(event);
    UNUSED(ep);
    weave_usb_tx_next(dev, &ccid_pipe, CCID_IN);
}

/* The STM32 USB stack indexes endpoint callbacks by endpoint number, not
 * direction. IN and OUT on EP1/EP2 MUST share one combined handler. */
static void weave_usb_hid_txrx(usbd_device* dev, uint8_t event, uint8_t ep) {
    if(event == usbd_evt_eptx) weave_usb_hid_tx(dev, event, ep);
    else weave_usb_hid_rx(dev, event, ep);
}
static void weave_usb_ccid_txrx(usbd_device* dev, uint8_t event, uint8_t ep) {
    if(event == usbd_evt_eptx) weave_usb_ccid_tx(dev, event, ep);
    else weave_usb_ccid_rx(dev, event, ep);
}

static void weave_usb_ccid_notify_tx(usbd_device* dev, uint8_t event, uint8_t ep) {
    UNUSED(dev);
    UNUSED(event);
    UNUSED(ep);
    ccid_notification_busy = false;
}

static usbd_respond weave_usb_config(usbd_device* dev, uint8_t cfg) {
    if(cfg == 0) {
        if(hid_enabled) {
            usbd_ep_deconfig(dev, HID_IN);
            usbd_ep_deconfig(dev, HID_OUT);
            usbd_reg_endpoint(dev, HID_IN, NULL);
            usbd_reg_endpoint(dev, HID_OUT, NULL);
        }
        if(ccid_enabled) {
            usbd_ep_deconfig(dev, CCID_IN);
            usbd_ep_deconfig(dev, CCID_OUT);
            usbd_ep_deconfig(dev, CCID_NOTIFY);
            usbd_reg_endpoint(dev, CCID_IN, NULL);
            usbd_reg_endpoint(dev, CCID_OUT, NULL);
            usbd_reg_endpoint(dev, CCID_NOTIFY, NULL);
            ccid_notification_busy = false;
        }
        usb_connected = false;
        return usbd_ack;
    }
    if(cfg != 1) return usbd_fail;
    if(hid_enabled) {
        usbd_ep_config(dev, HID_IN, USB_EPTYPE_INTERRUPT, hid_packet_size);
        usbd_ep_config(dev, HID_OUT, USB_EPTYPE_INTERRUPT, hid_packet_size);
        usbd_reg_endpoint(dev, HID_IN, weave_usb_hid_txrx);
        usbd_reg_endpoint(dev, HID_OUT, weave_usb_hid_txrx);
    }
    if(ccid_enabled) {
        usbd_ep_config(dev, CCID_IN, USB_EPTYPE_BULK, USB_PACKET_SIZE);
        usbd_ep_config(dev, CCID_OUT, USB_EPTYPE_BULK, USB_PACKET_SIZE);
        usbd_ep_config(dev, CCID_NOTIFY, USB_EPTYPE_INTERRUPT, 8);
        usbd_reg_endpoint(dev, CCID_IN, weave_usb_ccid_txrx);
        usbd_reg_endpoint(dev, CCID_OUT, weave_usb_ccid_txrx);
        usbd_reg_endpoint(dev, CCID_NOTIFY, weave_usb_ccid_notify_tx);
    }
    usb_connected = true;
    return usbd_ack;
}

static usbd_respond weave_usb_control(
    usbd_device* dev, usbd_ctlreq* req, usbd_rqc_callback* callback) {
    UNUSED(callback);
    if(!usb_buffers) return usbd_fail;
    uint8_t req_type = req->bmRequestType & (USB_REQ_RECIPIENT | USB_REQ_TYPE);
    if(hid_enabled && req->wIndex == 0) {
        if(req_type == (USB_REQ_INTERFACE | USB_REQ_CLASS) &&
           req->bRequest == 0x0a) { /* SET_IDLE */
            return usbd_ack;
        }
        if(req_type == (USB_REQ_INTERFACE | USB_REQ_STANDARD) &&
           req->bRequest == USB_STD_GET_DESCRIPTOR) {
            switch(req->wValue >> 8) {
            case USB_DTYPE_HID:
                dev->status.data_ptr = (uint8_t*)(
                    ccid_enabled ? &composite_cfg.hid.hid : &hid_cfg.hid.hid);
                dev->status.data_count = sizeof(struct usb_hid_descriptor);
                return usbd_ack;
            case USB_DTYPE_HID_REPORT:
                dev->status.data_ptr = (uint8_t*)hid_report;
                dev->status.data_count = hid_report_size;
                return usbd_ack;
            default:
                break;
            }
        }
    }
    if(ccid_enabled && req->wIndex == ccid_if_num &&
       req_type == (USB_REQ_INTERFACE | USB_REQ_CLASS) &&
       req->bRequest == 0x01) { /* CCID ABORT control stage */
        ccid_pipe.rx_accumulated = 0;
        /* The FAP receives the corresponding bulk CCID ABORT command. */
        return usbd_ack;
    }
    /* Other unsupported CCID class control requests must not be ACKed. */
    return usbd_fail;
}

bool furi_hal_weave_usb_notify_card(bool present) {
    if(!ccid_enabled || !usb_connected || !usb_device) return false;
    bool ok = false;
    FURI_CRITICAL_ENTER();
    if(!ccid_notification_busy) {
        ccid_notification[0] = 0x50; /* RDR_to_PC_NotifySlotChange */
        ccid_notification[1] = present ? 0x03 : 0x02; /* slot 0 changed */
        ccid_notification_busy = true;
        ok = true;
    }
    FURI_CRITICAL_EXIT();
    if(ok) usbd_ep_write(usb_device, CCID_NOTIFY, ccid_notification, sizeof(ccid_notification));
    return ok;
}

bool furi_hal_weave_usb_connected(void) {
    return usb_connected;
}
bool furi_hal_weave_usb_receive(
    FuriHalWeaveUsbChannel channel, uint8_t* data, size_t capacity, size_t* size) {
    if(!usb_buffers || !data || !size) return false;
    if((channel == FuriHalWeaveUsbHid && !hid_enabled) ||
       (channel == FuriHalWeaveUsbCcid && !ccid_enabled)) return false;
    WeaveUsbPipe* pipe = channel == FuriHalWeaveUsbHid ? &hid_pipe : &ccid_pipe;
    bool ok = false;
    FURI_CRITICAL_ENTER();
    if(usb_connected && channel == FuriHalWeaveUsbHid &&
       hid_rx_count && capacity >= hid_rx_sizes[hid_rx_head]) {
        memcpy(data, hid_rx_frames[hid_rx_head], hid_rx_sizes[hid_rx_head]);
        *size = hid_rx_sizes[hid_rx_head];
        hid_rx_head = (hid_rx_head + 1U) % HID_RX_QUEUE;
        hid_rx_count--;
        ok = true;
    } else if(usb_connected && channel == FuriHalWeaveUsbCcid &&
              pipe->rx_ready && capacity >= pipe->rx_length) {
        memcpy(data, pipe->rx, pipe->rx_length);
        *size = pipe->rx_length;
        pipe->rx_ready = false;
        ok = true;
    }
    FURI_CRITICAL_EXIT();
    return ok;
}
bool furi_hal_weave_usb_transmit(
    FuriHalWeaveUsbChannel channel, const uint8_t* data, size_t size) {
    if(!usb_buffers || !data || !size) return false;
    if((channel == FuriHalWeaveUsbHid && (!hid_enabled || size > hid_packet_size)) ||
       (channel == FuriHalWeaveUsbCcid && (!ccid_enabled || size < 10 || size > CCID_RX_BUFFER)))
        return false;
    if(channel == FuriHalWeaveUsbCcid) {
        const uint32_t payload = (uint32_t)data[1] | ((uint32_t)data[2] << 8) |
            ((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 24);
        if(payload + 10U != size) return false;
    }
    WeaveUsbPipe* pipe = channel == FuriHalWeaveUsbHid ? &hid_pipe : &ccid_pipe;
    uint8_t endpoint = channel == FuriHalWeaveUsbHid ? HID_IN : CCID_IN;
    bool ok = false;
    FURI_CRITICAL_ENTER();
    if(usb_device && usb_connected && !pipe->tx_busy) {
        memcpy(pipe->tx, data, size);
        pipe->tx_length = size;
        pipe->tx_offset = 0;
        pipe->tx_busy = true;
        ok = true;
    }
    FURI_CRITICAL_EXIT();
    if(ok) weave_usb_tx_next(usb_device, pipe, endpoint);
    return ok;
}
