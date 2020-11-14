#include "padproxy.h"
#include "usb-pad.h"
#include "../qemu-usb/desc.h"

namespace usb_pad {

static const USBDescStrings guncon2_desc_strings = {
    "",
    "Namco Guncon2(tm) Controller V1",
    "",
    "Logitech"
};



static const uint8_t guncon2_dev_descriptor[] = {
    0x12,        // bLength
    0x01,        // bDescriptorType (Device)
    0x00, 0x01,  // bcdUSB 2.00
    0xff,        // bDeviceClass (Use class information in the Interface Descriptors)
    0x00,        // bDeviceSubClass
    0x00,        // bDeviceProtocol
    0x08,        // bMaxPacketSize0 8
    0x9a, 0x0b,  // idVendor 0x0b9a
    0x6a, 0x01,  // idProduct 0x016a
    0x00, 0x01,  // bcdDevice 11.01
    0x00,        // iManufacturer (String Index)
    0x00,        // iProduct (String Index)
    0x00,        // iSerialNumber (String Index)
    0x01,        // bNumConfigurations 1
};

static const uint8_t guncon2_config_descriptor[] = {
    0x09,        // bLength
    0x02,        // bDescriptorType (Configuration)
    0x22, 0x00,  // wTotalLength 34
    0x01,        // bNumInterfaces 1
    0x01,        // bConfigurationValue
    0x00,        // iConfiguration (String Index)
    0x80,        // bmAttributes
    0x19,        // bMaxPower 100mA

    0x09,        // bLength
    0x04,        // bDescriptorType (Interface)
    0x00,        // bInterfaceNumber 0
    0x00,        // bAlternateSetting
    0x01,        // bNumEndpoints 1
    0xff,        // bInterfaceClass
    0x6a,        // bInterfaceSubClass
    0x00,        // bInterfaceProtocol
    0x00,        // iInterface (String Index)

    0x09,        // bLength
    0x21,        // bDescriptorType (HID)
    0x11, 0x01,  // bcdHID 1.11
    0x00,        // bCountryCode
    0x01,        // bNumDescriptors
    0x22,        // bDescriptorType[0] (HID)
    92U, 0x00,  // wDescriptorLength[0] 78

    0x07,        // bLength
    0x05,        // bDescriptorType (Endpoint)
    0x81,        // bEndpointAddress (IN/D2H)
    0x03,        // bmAttributes (Interrupt)
    0x08, 0x00,  // wMaxPacketSize 8
    8,        // bInterval 10 (unit depends on device speed)
};

static const uint8_t guncon2_hid_report_descriptor[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x04,                    // USAGE (Joystick)
    0xa1, 0x01,                    // COLLECTION (Application)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x07,                    //     USAGE_MAXIMUM (Button 7)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x35, 0x00,                    //     PHYSICAL_MINIMUM (0)
    0x45, 0x01,                    //     PHYSICAL_MAXIMUM (1)
    0x95, 0x07,                    //     REPORT_COUNT (7)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0x19, 0x08,                    //     USAGE_MINIMUM (Button 8)
    0x29, 0x0a,                    //     USAGE_MAXIMUM (Button 10)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x04,                    //     USAGE (Joystick)
    0x75, 0x10,                    //     REPORT_SIZE (16)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x09, 0x30,                    //     USAGE (X)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x26, 0xbc, 0x02,              //     LOGICAL_MAXIMUM (700)
    0x35, 0x00,                    //     PHYSICAL_MINIMUM (0)
    0x46, 0xbc, 0x02,              //     PHYSICAL_MAXIMUM (700)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x09, 0x31,                    //     USAGE (Y)
    0x26, 0x54, 0x01,              //     LOGICAL_MAXIMUM (340)
    0x46, 0x54, 0x01,              //     PHYSICAL_MAXIMUM (340)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
    0xc0,                          //     END_COLLECTION
    0xc0                           // END_COLLECTION
};

std::list<std::string> Guncon2Device::ListAPIs()
{
    return PadDevice::ListAPIs();
}

const TCHAR* Guncon2Device::LongAPIName(const std::string& name)
{
    return PadDevice::LongAPIName(name);
}


typedef struct Guncon2State {
    USBDevice		dev;
    USBDesc desc;
    USBDescDevice desc_dev;
    Pad*			pad;
    uint8_t			port;
    struct freeze {
        int nothing;
    } f;
} Guncon2State;

static void pad_handle_data(USBDevice *dev, USBPacket *p)
{
    Guncon2State *s = (Guncon2State *)dev;
    uint8_t data[64];

    int ret = 0;
    uint8_t devep = p->ep->nr;

    switch(p->pid) {
    case USB_TOKEN_IN:
        if (devep == 1 && s->pad) {
            ret = s->pad->TokenIn(data, p->iov.size);
            if (ret > 0)
            {
                usb_packet_copy (p, data, MIN(ret, sizeof(data)));

            }
            else
            {p->status = ret;}
        } else {
            goto fail;
        }
        break;
    case USB_TOKEN_OUT:
        usb_packet_copy (p, data, MIN(p->iov.size, sizeof(data)));
        ret = s->pad->TokenOut(data, p->iov.size);
        break;
    default:
fail:
        p->status = USB_RET_STALL;
        break;
    }
}

static void pad_handle_reset(USBDevice *dev)
{
    /* XXX: do it */
    Guncon2State *s = (Guncon2State*)dev;
    s->pad->Reset();
    return;
}

static void pad_handle_control(USBDevice *dev, USBPacket *p, int request, int value,
                               int index, int length, uint8_t *data)
{
    Guncon2State *s = (Guncon2State *)dev;
    int ret = 0;

    int t = conf.WheelType[s->port];

    switch(request) {
    case DeviceRequest | USB_REQ_GET_DESCRIPTOR:
        OSDebugOut(TEXT("DeviceRequest | USB_REQ_GET_DESCRIPTOR 0x%04X\n"), value);
        ret = usb_desc_handle_control(dev, p, request, value, index, length, data);
        if (ret < 0)
            goto fail;

        break;
    case InterfaceRequest | USB_REQ_GET_DESCRIPTOR: //GT3
        OSDebugOut(TEXT("InterfaceRequest | USB_REQ_GET_DESCRIPTOR 0x%04X\n"), value);
        switch(value >> 8) {
        case USB_DT_REPORT:
            OSDebugOut(TEXT("Sending hid report desc.\n"));
            p->actual_length = ret;
            break;
        default:
            goto fail;
        }
        break;
        /* hid specific requests */
    case SET_REPORT:
        //en guncon2 aquí se pone la pistola en progressive y los offsets

        // no idea, Rock Band 2 keeps spamming this
        OSDebugOut(TEXT("SET_REPORT 0x%04X\n"), value);
        if (length > 0) {
            OSDebugOut(TEXT("SET_REPORT: 0x%02X \n"), data[0]);

            std::cerr << "SET_REPORT: ";
            char cadena[256];
            for(int i_l=0;i_l<length;i_l++)
            {
                sprintf(cadena,"%x",data[i_l]);
                std::cerr << cadena;
            }
            std::cerr << std::endl;


            s->pad->Guncon2SetReport(data);



            /* 0x01: Num Lock LED
             * 0x02: Caps Lock LED
             * 0x04: Scroll Lock LED
             * 0x08: Compose LED
             * 0x10: Kana LED */
            p->actual_length = 0;
            //p->status = USB_RET_SUCCESS;
        }
        break;
    case SET_IDLE:
        OSDebugOut (TEXT("SET_IDLE\n"));
        break;
    default:
        ret = usb_desc_handle_control(dev, p, request, value, index, length, data);
        if (ret >= 0) {
            return;
        }
fail:
        p->status = USB_RET_STALL;
        break;
    }

}

static void pad_handle_destroy(USBDevice *dev)
{
    Guncon2State *s = (Guncon2State *)dev;
    delete s;
}

static int pad_open(USBDevice *dev)
{
    Guncon2State *s = (Guncon2State *) dev;
    if (s)
        return s->pad->Open();
    return 1;
}

static void pad_close(USBDevice *dev)
{
    Guncon2State *s = (Guncon2State *) dev;
    if (s)
        s->pad->Close();
}

USBDevice *Guncon2Device::CreateDevice(int port)
{
    std::string varApi;
    LoadSetting(nullptr, port, TypeName(), N_DEVICE_API, varApi);
    PadProxyBase* proxy = RegisterPad::instance().Proxy(varApi);
    if (!proxy)
    {
        SysMessage(TEXT("Guncon2: Invalid input API.\n"));
        USB_LOG("usb-pad: %s: Invalid input API.\n", TypeName());
        return NULL;
    }

    USB_LOG("usb-pad: creating device '%s' on port %d with %s\n", TypeName(), port, varApi.c_str());
    Pad* pad = proxy->CreateObject(port, TypeName());

    if (!pad)
        return NULL;

    pad->Type(WT_GUNCON2);
    Guncon2State* s = new Guncon2State();

    s->desc.full = &s->desc_dev;
    s->desc.str = guncon2_desc_strings;

    if (usb_desc_parse_dev(guncon2_dev_descriptor, sizeof(guncon2_dev_descriptor), s->desc, s->desc_dev) < 0)
        goto fail;
    if (usb_desc_parse_config(guncon2_config_descriptor, sizeof(guncon2_config_descriptor), s->desc_dev) < 0)
        goto fail;

    //s->f.wheel_type = pad->Type();
    s->pad = pad;
    s->port = port;
    s->dev.speed = USB_SPEED_FULL;
    s->dev.klass.handle_attach = usb_desc_attach;
    s->dev.klass.handle_reset = pad_handle_reset;
    s->dev.klass.handle_control = pad_handle_control;
    s->dev.klass.handle_data = pad_handle_data;
    s->dev.klass.unrealize = pad_handle_destroy;
    s->dev.klass.open = pad_open;
    s->dev.klass.close = pad_close;
    s->dev.klass.usb_desc = &s->desc;
    s->dev.klass.product_desc = s->desc.str[2];

    usb_desc_init(&s->dev);
    usb_ep_init(&s->dev);
    pad_handle_reset((USBDevice*)s);

    return (USBDevice*)s;

fail:
    pad_handle_destroy((USBDevice*)s);
    return nullptr;
}

int Guncon2Device::Configure(int port, const std::string& api, void *data)
{
    auto proxy = RegisterPad::instance().Proxy(api);
    if (proxy)
        return proxy->Configure(port, TypeName(), data);
    return RESULT_CANCELED;
}

int Guncon2Device::Freeze(int mode, USBDevice *dev, void *data)
{
    return PadDevice::Freeze(mode, dev, data);
}

}

