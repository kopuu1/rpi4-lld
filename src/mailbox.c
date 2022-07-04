#include "mailbox.h"
#include "mem.h"


static u32 property_data[8192] __attribute__((aligned(16)));

static void mailbox_write(u32 data, u8 channel) {
    while(REGS_MBX->status & MAIL_FULL) ;

    REGS_MBX->write = ((data & 0xFFFFFFF0) | (channel & 0xF));
}

static u32 mailbox_read(u8 channel) {
    while(true) {
        while(REGS_MBX->status & MAIL_EMPTY) ;

        u32 data = REGS_MBX->read;

        u8 read_channel = (u8)(data & 0xF);

        if (read_channel == channel) {
            return data & 0xFFFFFFF0;
        }
    }
}

bool mailbox_process(mb_tag *tag, u32 tag_size) {
    int buffer_size = tag_size + 12;

    memcpy(tag, &property_data[2], tag_size);

    property_buffer *buff = (property_buffer *)property_data;
    buff->size = buffer_size;
    buff->code = RPI_FIRMWARE_STATUS_REQUEST;
    property_data[(tag_size + 12) / 4 - 1] = RPI_FIRMWARE_PROPERTY_END;

    mailbox_write((u32)(void *)property_data, MBX_CH_PTAGS);

    int result = mailbox_read(MBX_CH_PTAGS);

    memcpy(&property_data[2], tag, tag_size);

    return true;
}

bool mailbox_generic_4byte(u32 tag_id, u32 *value) {
    mb_4byte mbx;
    mbx.tag.id = tag_id;
    mbx.tag.buffer_size = sizeof(mbx) - sizeof(mbx.tag);
    mbx.tag.value_length = 0;
    mbx.val = *value;

    mailbox_process((mb_tag *)&mbx, sizeof(mbx));

    *value = mbx.val;

    return true;
}

bool mailbox_generic_8byte(u32 tag_id, u32 id, u32 *value) {
    mb_8byte mbx;
    mbx.tag.id = tag_id;
    mbx.tag.buffer_size = sizeof(mbx) - sizeof(mbx.tag);
    mbx.tag.value_length = 0;
    mbx.val1 = id;
    mbx.val2 = *value;

    mailbox_process((mb_tag *)&mbx, sizeof(mbx));

    *value = mbx.val2;

    return true;
}

u32 mailbox_clock_rate(clock_type ct) {
    mb_8byte c;
    c.tag.id = RPI_FIRMWARE_GET_CLOCK_RATE;
    c.tag.value_length = 0;
    c.tag.buffer_size = sizeof(c) - sizeof(c.tag);
    c.val1 = ct; // id

    mailbox_process((mb_tag *)&c, sizeof(c));

    return c.val2;
}

bool mailbox_power_check(u32 type) {
    mb_8byte p;
    p.tag.id = RPI_FIRMWARE_GET_DOMAIN_STATE;
    p.tag.value_length = 0;
    p.tag.buffer_size = sizeof(p) - sizeof(p.tag);
    p.val1 = type; // id
    p.val2 = ~0;

    mailbox_process((mb_tag *)&p, sizeof(p));

    return p.val2 && p.val2 != ~0;
}
