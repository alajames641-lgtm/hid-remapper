/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Jacek Fedorynski
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 */

#include <tusb.h>

#include "config.h"
#include "globals.h"
#include "our_descriptor.h"
#include "platform.h"
#include "remapper.h"

// --- STEALTH LOGITECH G102 IDENTITY ---
#define USB_VID 0x046D         // Logitech Vendor ID
#define USB_PID 0xC09D         // G102 Lightsync Product ID

tusb_desc_device_t desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,          // USB 2.0
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = 0x2703,       // Stealth: Real G102 Hardware Revision

    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x00,     // Stealth: 0 hides serial tracking

    .bNumConfigurations = 0x01,
};

// --- STEALTH: MOUSE-ONLY INTERFACE DESCRIPTORS ---
// We use bNumInterfaces = 1 to hide Keyboard/Media keys

const uint8_t configuration_descriptor0[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN, 0, 100),
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_MOUSE, our_descriptors[0].descriptor_length, 0x81, CFG_TUD_HID_EP_BUFSIZE, 1),
};

const uint8_t configuration_descriptor1[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN, 0, 100),
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_MOUSE, our_descriptors[1].descriptor_length, 0x81, CFG_TUD_HID_EP_BUFSIZE, 1),
};

const uint8_t configuration_descriptor2[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN, 0, 100),
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_MOUSE, our_descriptors[2].descriptor_length, 0x81, CFG_TUD_HID_EP_BUFSIZE, 1),
};

const uint8_t configuration_descriptor3[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN, 0, 100),
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_MOUSE, our_descriptors[3].descriptor_length, 0x81, CFG_TUD_HID_EP_BUFSIZE, 1),
};

const uint8_t configuration_descriptor4[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN, 0, 100),
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_MOUSE, our_descriptors[4].descriptor_length, 0x81, CFG_TUD_HID_EP_BUFSIZE, 1),
};

const uint8_t configuration_descriptor5[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN, 0, 100),
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_MOUSE, our_descriptors[5].descriptor_length, 0x81, CFG_TUD_HID_EP_BUFSIZE, 1),
};

const uint8_t* configuration_descriptors[] = {
    configuration_descriptor0,
    configuration_descriptor1,
    configuration_descriptor2,
    configuration_descriptor3,
    configuration_descriptor4,
    configuration_descriptor5,
};

char const* string_desc_arr[] = {
    (const char[]){ 0x09, 0x04 },    // 0: English
    "Logitech",                      // 1: Manufacturer
    "G102 LIGHTSYNC Gaming Mouse",   // 2: Product
};

// --- CALLBACKS ---

uint8_t const* tud_descriptor_device_cb() {
    return (uint8_t const*) &desc_device;
}

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    return configuration_descriptors[our_descriptor->idx];
}

uint8_t const* tud_hid_descriptor_report_cb(uint8_t itf) {
    // Only Interface 0 (Mouse) exists now
    if (itf == 0) {
        return our_descriptor->descriptor;
    }
    return NULL;
}

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    uint8_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
            return NULL;

        const char* str = string_desc_arr[index];
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;

        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
        
        // Stealth: Unique ID appending removed to keep product name clean
    }

    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
    return _desc_str;
}

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    if (itf == 0) {
        return handle_get_report0(report_id, buffer, reqlen);
    }
    return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    if (itf == 0) {
        if ((report_id == 0) && (report_type == 0) && (bufsize > 0)) {
            report_id = buffer[0];
            buffer++;
        }
        handle_set_report0(report_id, buffer, bufsize);
    }
}

void tud_hid_set_protocol_cb(uint8_t instance, uint8_t protocol) {
    boot_protocol_keyboard = (protocol == HID_PROTOCOL_BOOT);
    boot_protocol_updated = true;
}

void tud_mount_cb() {
    reset_resolution_multiplier();
}

void tud_suspend_cb(bool remote_wakeup_en) {}
void tud_resume_cb() {}
