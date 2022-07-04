#include "io.h"
#include "framebuffer.h"
#include "mailbox.h"

void main()
{
    uart_init();
    mailbox_framebuffer_init();

    while (1);
}
