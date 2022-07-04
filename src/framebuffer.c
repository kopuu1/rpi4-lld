#include "framebuffer.h"
#include "mailbox.h"
#include "mem.h"
#include "terminal.h"

u32 pitch;
u8 *fb_addr;

void mailbox_framebuffer_init() {
    mb_fb fbx;
    // Set physical width/height
    fbx.phy_wh.tag.id = RPI_FIRMWARE_FRAMEBUFFER_SET_PHYSICAL_WIDTH_HEIGHT;
    fbx.phy_wh.tag.buffer_size = sizeof(fbx.phy_wh) - sizeof(fbx.phy_wh.tag);
    fbx.phy_wh.tag.value_length = 0;
    fbx.phy_wh.val1 = 1920; // Width
    fbx.phy_wh.val2 = 1080; // Height
    // Set virtual (buffer) width/height
    fbx.virt_wh.tag.id = RPI_FIRMWARE_FRAMEBUFFER_SET_VIRTUAL_WIDTH_HEIGHT;
    fbx.virt_wh.tag.buffer_size = sizeof(fbx.virt_wh) - sizeof(fbx.virt_wh.tag);
    fbx.virt_wh.tag.value_length = 0;
    fbx.virt_wh.val1 = 1920; // Width
    fbx.virt_wh.val2 = 1080; // Height
    // Set virtual offset
    fbx.virt_offs.tag.id = RPI_FIRMWARE_FRAMEBUFFER_SET_VIRTUAL_OFFSET;
    fbx.virt_offs.tag.buffer_size = sizeof(fbx.virt_offs) - sizeof(fbx.virt_offs.tag);
    fbx.virt_offs.tag.value_length = 0;
    fbx.virt_offs.val1 = 0; // x
    fbx.virt_offs.val2 = 0; // y
    // Set depth
    fbx.depth.tag.id = RPI_FIRMWARE_FRAMEBUFFER_SET_DEPTH;
    fbx.depth.tag.buffer_size = sizeof(fbx.depth) - sizeof(fbx.depth.tag);
    fbx.depth.tag.value_length = 0;
    fbx.depth.val = 32; // Bits per pixel
    // Set pixel order
    fbx.pxl_ordr.tag.id = RPI_FIRMWARE_FRAMEBUFFER_SET_PIXEL_ORDER;
    fbx.pxl_ordr.tag.buffer_size = sizeof(fbx.pxl_ordr) - sizeof(fbx.pxl_ordr.tag);
    fbx.pxl_ordr.tag.value_length = 0;
    fbx.pxl_ordr.val = 1; // RGB
    // Allocate frame buffer
    fbx.fb_alloc.tag.id = RPI_FIRMWARE_FRAMEBUFFER_ALLOCATE;
    fbx.fb_alloc.tag.buffer_size = sizeof(fbx.fb_alloc) - sizeof(fbx.fb_alloc.tag);
    fbx.fb_alloc.tag.value_length = 0;
    fbx.fb_alloc.val1 = 0; // Frame buffer base address
    fbx.fb_alloc.val2 = 0; // Frame buffer size in bytes
    // Get pitch
    fbx.pitch.tag.id = RPI_FIRMWARE_FRAMEBUFFER_GET_PITCH;
    fbx.pitch.tag.buffer_size = sizeof(fbx.pitch) - sizeof(fbx.pitch.tag);
    fbx.pitch.tag.value_length = 0;
    fbx.pitch.val = 0; // Bytes per line
    
    mailbox_process((mb_tag *)&fbx, sizeof(fbx));

    fbx.fb_alloc.val1 &= 0x3FFFFFFF; // Map VC address to ARM address
    fb_addr = (u8 *)((u32)fbx.fb_alloc.val1);
    pitch = fbx.pitch.val;
}



void drawPixel(int x, int y, unsigned char attr)
{
    int offs = (y * pitch) + (x * 4);
    *((unsigned int*)(fb_addr + offs)) = vgapal[attr & 0x0f];
}

void drawRect(int x1, int y1, int x2, int y2, unsigned char attr, int fill)
{
    int y=y1;

    while (y <= y2) {
       int x=x1;
       while (x <= x2) {
	  if ((x == x1 || x == x2) || (y == y1 || y == y2)) drawPixel(x, y, attr);
	  else if (fill) drawPixel(x, y, (attr & 0xf0) >> 4);
          x++;
       }
       y++;
    }
}

void drawLine(int x1, int y1, int x2, int y2, unsigned char attr)  
{  
    int dx, dy, p, x, y;

    dx = x2-x1;
    dy = y2-y1;
    x = x1;
    y = y1;
    p = 2*dy-dx;

    while (x<x2) {
       if (p >= 0) {
          drawPixel(x,y,attr);
          y++;
          p = p+2*dy-2*dx;
       } else {
          drawPixel(x,y,attr);
          p = p+2*dy;
       }
       x++;
    }
}

void drawCircle(int x0, int y0, int radius, unsigned char attr, int fill)
{
    int x = radius;
    int y = 0;
    int err = 0;
 
    while (x >= y) {
	if (fill) {
	   drawLine(x0 - y, y0 + x, x0 + y, y0 + x, (attr & 0xf0) >> 4);
	   drawLine(x0 - x, y0 + y, x0 + x, y0 + y, (attr & 0xf0) >> 4);
	   drawLine(x0 - x, y0 - y, x0 + x, y0 - y, (attr & 0xf0) >> 4);
	   drawLine(x0 - y, y0 - x, x0 + y, y0 - x, (attr & 0xf0) >> 4);
	}
	drawPixel(x0 - y, y0 + x, attr);
	drawPixel(x0 + y, y0 + x, attr);
	drawPixel(x0 - x, y0 + y, attr);
        drawPixel(x0 + x, y0 + y, attr);
	drawPixel(x0 - x, y0 - y, attr);
	drawPixel(x0 + x, y0 - y, attr);
	drawPixel(x0 - y, y0 - x, attr);
	drawPixel(x0 + y, y0 - x, attr);

	if (err <= 0) {
	    y += 1;
	    err += 2*y + 1;
	}
 
	if (err > 0) {
	    x -= 1;
	    err -= 2*x + 1;
	}
    }
}

void drawChar(unsigned char ch, int x, int y, unsigned char attr)
{
    unsigned char *glyph = (unsigned char *)&font + (ch < FONT_NUMGLYPHS ? ch : 0) * FONT_BPG;

    for (int i=0;i<FONT_HEIGHT;i++) {
	for (int j=0;j<FONT_WIDTH;j++) {
	    unsigned char mask = 1 << j;
	    unsigned char col = (*glyph & mask) ? attr & 0x0f : (attr & 0xf0) >> 4;

	    drawPixel(x+j, y+i, col);
	}
	glyph += FONT_BPL;
    }
}

void drawString(int x, int y, char *s, unsigned char attr)
{
    while (*s) {
       if (*s == '\r') {
          x = 0;
       } else if(*s == '\n') {
          x = 0; y += FONT_HEIGHT;
       } else {
	  drawChar(*s, x, y, attr);
          x += FONT_WIDTH;
       }
       s++;
    }
}
