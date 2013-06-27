/*
 * driver/serial/s5h2x_uart.c
 *
 * Copyright (C) 2006 Samsung Electronics.co
 * Author : tukho.kim@samsung.com
 *
 */

#include <linux/autoconf.h>

#define S5H2X_SERIAL_DEBUG
//#undef S5H2X_SERIAL_DEBUG


#if defined(CONFIG_SERIAL_S5H2X_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define  SUPPORT_SYSRQ
#endif

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_reg.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/mutex.h>

#include <linux/device.h>

#include <asm/io.h>
#include <asm/irq.h>

#include <asm/hardware.h>

#include <asm/arch/platform.h>
#include <asm/plat-sdp/serial_s5h2x.h>

#ifndef S5H2X_UART_NR
#define S5H2X_UART_NR	 	SDP_UART_NR
#define PORT_S5H2X		PORT_SDP
#define S5H2X_GET_UARTCLK(a)	SDP_GET_UARTCLK(a)
#endif


#define S5H2X_SERIAL_NAME	"ttyS"
#define S5H2X_SERIAL_MAJOR	204
#define S5H2X_SERIAL_MINOR	64

#ifdef S5H2X_SERIAL_DEBUG
#define S5H2X_SERIALD_NAME	"ttySD"
#define S5H2X_SERIALD_MAJOR	S5H2X_SERIAL_MAJOR
#define S5H2X_SERIALD_MINOR	(S5H2X_SERIAL_MINOR + S5H2X_UART_NR)
#endif

//port -> unused is unsigned character variable

#define MODE_INTERRUPT		0
#define MODE_POLLING		1
#define MODE_METHOD_MASK	1

#define MODE_CONSOLE		(0 << 4)
#define MODE_DEBUG			(1 << 4)
#define MODE_DEBUG_MASK		MODE_DEBUG

#define MODE_NREQ_IRQ		(0 << 7)
#define MODE_REQ_IRQ		(1 << 7)
#define MODE_REQ_IRQ_MASK	MODE_REQ_IRQ

/**************************************************
 * Macros
 **************************************************/
#define UART_GET_CHAR(p)	__raw_readb((p)->membase + S5H2X_UARTRXH0_OFF)
#define UART_PUT_CHAR(p,c)	__raw_writeb((c), (p)->membase + S5H2X_UARTTXH0_OFF)

#define UART_GET_ULCON(p)	__raw_readl((p)->membase + S5H2X_UARTLCON_OFF)
#define UART_GET_UCON(p)	__raw_readl((p)->membase + S5H2X_UARTCON_OFF)
#define UART_GET_UFCON(p)	__raw_readl((p)->membase + S5H2X_UARTFCON_OFF)
#define UART_GET_UMCON(p)	__raw_readl((p)->membase + S5H2X_UARTMCON_OFF)
#define UART_GET_UBRDIV(p)	__raw_readl((p)->membase + S5H2X_UARTBRDIV_OFF)

#define UART_GET_UTRSTAT(p)	__raw_readl((p)->membase + S5H2X_UARTTRSTAT_OFF)
#define UART_GET_UERSTAT(p)	__raw_readl((p)->membase + S5H2X_UARTERSTAT_OFF)
#define UART_GET_UFSTAT(p)	__raw_readl((p)->membase + S5H2X_UARTFSTAT_OFF)
#define UART_GET_UMSTAT(p)	__raw_readl((p)->membase + S5H2X_UARTMSTAT_OFF)

#define UART_PUT_UTRSTAT(p,c)	__raw_writel(c, (p)->membase + S5H2X_UARTTRSTAT_OFF)
#define UART_PUT_ULCON(p,c)		__raw_writel(c, (p)->membase + S5H2X_UARTLCON_OFF)
#define UART_PUT_UCON(p,c)		__raw_writel(c, (p)->membase + S5H2X_UARTCON_OFF)
#define UART_PUT_UFCON(p,c)		__raw_writel(c, (p)->membase + S5H2X_UARTFCON_OFF)
#define UART_PUT_UMCON(p,c)		__raw_writel(c, (p)->membase + S5H2X_UARTMCON_OFF)
#define UART_PUT_UBRDIV(p,c)	__raw_writel(c, (p)->membase + S5H2X_UARTBRDIV_OFF)

#define UART_RX_DATA(s)         (((s) & S5H2X_UTRSTAT_RXDR) == S5H2X_UTRSTAT_RXDR)
#define UART_TX_READY(s)        (((s) & S5H2X_UTRSTAT_TXFE) == S5H2X_UTRSTAT_TXFE)
#define TX_FIFOCOUNT(port)      (((UART_GET_UFSTAT(port))>>4)&0xf)

#define TX_ENABLED(port)        ((port)->unused[0])
#define UART_MODE(port)        	((port)->unused[1])

// serial debug code
#ifdef S5H2X_SERIAL_DEBUG
#define UART_DEBUG(port)		((port)->unused[2])

static struct state_console_t {
        unsigned int ulcon;
        unsigned int ucon;
        unsigned int ubrdiv;
} stateConsole[S5H2X_UART_NR];
#endif


extern unsigned long
S5H2X_GET_UARTCLK(char mode);

extern void 
uart_parse_options(char *options, int *baud, int *parity, int *bits, int *flow);
extern int 
uart_set_options(struct uart_port *port, struct console *co, int baud, int parity, int bits, int flow);

static void 
#if 0
s5h2x_serial_stop_tx(struct uart_port *port, unsigned int tty_stop);
#else
s5h2x_serial_stop_tx(struct uart_port *port);
#endif

static unsigned int
s5h2x_serial_tx_chars(struct uart_port *port)
{
	struct circ_buf *xmit = &port->info->xmit;
	int count = 0;

        if (port->x_char) {
                UART_PUT_CHAR(port, port->x_char);
                port->icount.tx++;
                port->x_char = 0;
                return 0;
        }

        if (uart_circ_empty(xmit) || uart_tx_stopped(port)){
			s5h2x_serial_stop_tx(port);
		   	return 0;
		}

		if (port->fifosize > 1)
	   	 	count = (port->fifosize - (((UART_GET_UFSTAT(port) >> 4) & 0xF) + 1));	// avoid to overflow , fifo size
		else
			count = 1;

        while (count > 0) {
                UART_PUT_CHAR(port, xmit->buf[xmit->tail]);
                xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
                port->icount.tx++;
				count--;
                if (uart_circ_empty(xmit)){
					s5h2x_serial_stop_tx(port);
				   	break;
				}
        } 

        if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
                uart_write_wakeup(port);

	return 0;
}

static unsigned int
#ifdef SUPPORT_SYSRQ
s5h2x_serial_rx_chars(struct uart_port *port, struct pt_regs *regs)
#else
s5h2x_serial_rx_chars(struct uart_port *port)
#endif
{
	struct tty_struct *tty = port->info->tty;
	unsigned int status, uerstat;
	unsigned int ch, flag, max_count = 256;

	if (port->fifosize > 1)
		status = UART_GET_UFSTAT(port) & 0x10F;
	else 
		status = UART_GET_UTRSTAT(port) & 0x1;

	uerstat = UART_GET_UERSTAT(port) & 0xF;

	while( status && max_count-- )
	{
#if 0
                if (tty->flip.count >= TTY_FLIPBUF_SIZE) {
                        if (tty->low_latency)
                                tty_flip_buffer_push(tty);
                        /*
                         * If this failed then we will throw away the
                         * bytes but must do so to clear interrupts.
                         */
                }
#endif

		ch = UART_GET_CHAR(port);
		flag = TTY_NORMAL;
		uerstat = UART_GET_UERSTAT(port);

		port->icount.rx++;

//		if(uart_handle_sysrq_char(port, ch, regs))
		if(uart_handle_sysrq_char(port, ch))
			goto ignore_char;
#if 0
		tty_insert_flip_char(tty, ch, flag);
#else
/* static inline void
uart_insert_char(struct uart_port *port, unsigned int status,
		unsigned int overrun, unsigned int ch, unsigned int flag)
*/
		uart_insert_char(port, uerstat, S5H2X_UERSTAT_OE, ch, flag); 

#endif
ignore_char:	
		if (port->fifosize > 1)
			status = UART_GET_UFSTAT(port) & 0x10F;
		else 
			status = UART_GET_UTRSTAT(port) & 0x1;
	}

	tty_flip_buffer_push(tty);
	
	return 0;
}

//static irqreturn_t s5h2x_uart_int (int irq, void *dev_id, struct pt_regs *regs)
static irqreturn_t s5h2x_uart_int (int irq, void *dev_id)
{
	int handled = 0;
	struct uart_port *port = dev_id;
	unsigned int utrstat; 
	
	spin_lock(&port->lock);

	utrstat = UART_GET_UTRSTAT(port);

	if (utrstat & 0xF0){
		if (utrstat & S5H2X_UTRSTAT_RXI){
#ifdef SUPPORT_SYSRQ
			s5h2x_serial_rx_chars(port, regs);
#else
			s5h2x_serial_rx_chars(port);
#endif
			UART_PUT_UTRSTAT(port, 0x10);
		}

		if (utrstat & S5H2X_UTRSTAT_TXI){
			UART_PUT_UTRSTAT(port, 0x20);
			s5h2x_serial_tx_chars(port);
		}

		handled = 1;
	}

	spin_unlock(&port->lock);

	return IRQ_RETVAL(handled);
}


// wait for the transmitter to empty
static unsigned int 
s5h2x_serial_tx_empty(struct uart_port *port)
{
        return (UART_GET_UTRSTAT(port) & S5H2X_UTRSTAT_TXFE) ? TIOCSER_TEMT : 0;
}

static void 
s5h2x_serial_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	// don't support yet
}

static unsigned int 
s5h2x_serial_get_mctrl(struct uart_port *port)
{
	// don't support yet
	unsigned int retVal = 0;

	return retVal;
}

static void 
#if 0
s5h2x_serial_stop_tx(struct uart_port *port, unsigned int tty_stop)
#else
s5h2x_serial_stop_tx(struct uart_port *port)
#endif	
{

#ifdef S5H2X_SERIAL_DEBUG
        if(UART_MODE(port) & MODE_DEBUG){
                if(!UART_DEBUG(port)) return;
        }
#endif

	TX_ENABLED(port) = 0;

}

static void 
#if 0
s5h2x_serial_start_tx(struct uart_port *port, unsigned int tty_start)
#else
s5h2x_serial_start_tx(struct uart_port *port)
#endif
{
	unsigned long flags;

#ifdef S5H2X_SERIAL_DEBUG
// force to empty console buffer
    struct circ_buf *xmit = &port->info->xmit;

        if(UART_MODE(port) & MODE_DEBUG){
                if(!UART_DEBUG(port)) {
                        do {
                                xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
                                port->icount.tx++;
                                if (uart_circ_empty(xmit)){
                                        break;
                                }
                        } while (1);
                        return;
                }
        }
#endif

		local_irq_save(flags);
		local_irq_disable();
		s5h2x_serial_tx_chars(port);		// occur uart tx interrut event
		local_irq_restore(flags);
}

static void 
s5h2x_serial_stop_rx(struct uart_port *port)
{
#if 0
	unsigned long flags;
	unsigned int ucon = UART_GET_UCON(port);
	unsigned int ufcon;
#endif

}

#if 0
static void 
s5h2x_serial_pm(struct uart_port *, unsigned int state, unsigned int oldstate)
{

}
#endif

/*
 * Release IO and memory resources used by the port.
 * This includes iounmap if necessary.
 */

static void 
s5h2x_serial_enable_ms(struct uart_port *port)
{

}



// upper layer doesn't apply mutex
static void 
s5h2x_serial_break_ctl(struct uart_port *port, int ctl)
{
//	unsigned long flags;
        unsigned long ucon=UART_GET_UCON(port);

	if(ctl) ucon |= S5H2X_UCON_SBREAK;
	else	ucon &= ~S5H2X_UCON_SBREAK;

//	spin_lock_irqsave(&port->lock,flags);
      	UART_PUT_UCON(port,ucon);
//	spin_unlock_irqrestore(&port->lock,flags);
}

static int 
s5h2x_serial_startup(struct uart_port *port)
{
	int retVal = 0;

#ifdef S5H2X_SERIAL_DEBUG
// free console interrupt and requeset debug interrupt
        struct uart_port* console_port;

        if(UART_DEBUG(port)){
                console_port = port - S5H2X_UART_NR;
                UART_MODE(console_port) &= ~MODE_DEBUG_MASK;
                UART_MODE(console_port) |= MODE_DEBUG;

//              printk("Debug port startup \n");
                stateConsole[console_port->line].ucon =
                        UART_GET_UCON(port);

                UART_PUT_UCON(port, (UART_GET_UCON(port) & 0xFF) | S5H2X_UCON_TXIE); // interrupt disable

                while((UART_GET_UTRSTAT(port) & 0x06) != 0x06);  // wait to empty of tx,rx buffer

                UART_PUT_UTRSTAT(port, 0xF0);                   // clear interrupt flag

                if(UART_MODE(console_port) & MODE_REQ_IRQ){
//                      printk("console free_irq\n");
                        free_irq(port->irq, console_port);
                }
        }

#endif

	retVal = request_irq(port->irq, s5h2x_uart_int, 0, "s5h2x-serial", port);
	if (retVal) 
		return retVal;

#ifdef S5H2X_SERIAL_DEBUG
// only check console port
        if(!UART_DEBUG(port)) {
                UART_MODE(port) &= ~MODE_REQ_IRQ_MASK;
                UART_MODE(port) |= MODE_REQ_IRQ;
        }
#endif

	if(port->fifosize > 1){		// fifo enable 
	        UART_PUT_UCON(port, 0x1085 | S5H2X_UCON_TXIE );  // rx int, rx mode, rx count
       		UART_PUT_UFCON(port, 0x07);   // fifo clear, fifo mode
	}
	else {    			// don't use fifo buffer
	        UART_PUT_UCON(port, 0x1005 | S5H2X_UCON_TXIE);  // rx int, rx mode
       		UART_PUT_UFCON(port, 0x06);   // fifo clear
	}

        UART_PUT_UMCON(port, 0);
	
	if(port->uartclk == 0)
#if defined(CONFIG_ARCH_S4LXXX)
		port->uartclk = S4LX_GET_UARTCLK(UART_CLOCK);
#elif  defined(CONFIG_ARCH_SDPXX)
		port->uartclk = SDPXX_GET_UARTCLK(UART_CLOCK);
#else
		port->uartclk = S5H2X_GET_UARTCLK(UART_CLOCK);
#endif
        UART_PUT_UBRDIV(port, (int)((port->uartclk >> 4)/(CURRENT_BAUD_RATE))-1);

	return retVal;
}

static void 
s5h2x_serial_shutdown(struct uart_port *port)
{
	unsigned int ucon;

	ucon = UART_GET_UCON(port) & 0xFF;
	UART_PUT_UCON(port, ucon);

	free_irq(port->irq, port);

#ifdef S5H2X_SERIAL_DEBUG
{
        struct uart_port* console_port;

        if(UART_DEBUG(port)){
                console_port = port - S5H2X_UART_NR;
                UART_MODE(console_port) &= ~MODE_DEBUG_MASK;
                UART_MODE(console_port) |= MODE_CONSOLE;

                while((UART_GET_UTRSTAT(port) & 0x06) != 0x06);  // wait to empty of tx,rx buffer

                UART_PUT_UTRSTAT(port, 0xF0);                   // clear interrupt flag

                if(UART_MODE(console_port) & MODE_REQ_IRQ){
                        printk("recovery console port ISR \n");
                        request_irq(port->irq, s5h2x_uart_int, 0, "s5h2x-uart", console_port);
                }

                UART_PUT_ULCON(console_port,
                        stateConsole[console_port->line].ulcon);
                UART_PUT_UBRDIV(console_port,
                        stateConsole[console_port->line].ubrdiv);
                UART_PUT_UCON(console_port,
                        stateConsole[console_port->line].ucon);

                printk("Debug port shutdown \n");
        }
        else {
                UART_MODE(port) &= ~MODE_REQ_IRQ_MASK;
                UART_MODE(port) |= MODE_NREQ_IRQ;
        }

}
#endif

}

static void 
//s5h2x_serial_set_termios(struct uart_port *port, struct termios *new, struct termios *old)
s5h2x_serial_set_termios(struct uart_port *port, struct ktermios *new, struct ktermios *old)
{
	unsigned int 	lcon;
//	unsigned long	flags;
	unsigned int	baud, quot;

//	baud = uart_get_baud_rate(port, new, old, 1, port->uartclk >> 4);
	baud = uart_get_baud_rate(port, new, old, 2400, 115200);
	quot = uart_get_divisor(port,baud);

	lcon = UART_GET_ULCON(port);

      	switch (new->c_cflag & CSIZE) {
        	case CS5:
                	lcon |= S5H2X_LCON_CS5;
                	break;
        	case CS6:
               	 	lcon |= S5H2X_LCON_CS6;
                	break;
        	case CS7:
                	lcon |= S5H2X_LCON_CS7;
                	break;
        	default:
                	lcon |= S5H2X_LCON_CS8;
                	break;
        }

        if (new->c_cflag & PARENB) {
                lcon |= (new->c_iflag & PARODD)? S5H2X_LCON_PODD:S5H2X_LCON_PEVEN;
        } else {
                lcon |= S5H2X_LCON_PNONE;
        }

//	spin_lock_irqsave(&port->lock, flags);

        UART_PUT_ULCON(port, lcon);
	if (new->c_iflag & BRKINT){
                unsigned long ucon=UART_GET_UCON(port);
                ucon |= S5H2X_UCON_SBREAK;
                UART_PUT_UCON(port, ucon);
	}

	quot--;

        UART_PUT_UBRDIV(port, quot);

#ifdef S5H2X_SERIAL_DEBUG
        if(!UART_DEBUG(port)){
                stateConsole[port->line].ulcon = UART_GET_ULCON(port);
                stateConsole[port->line].ubrdiv = UART_GET_UBRDIV(port);
        }
#endif

//	spin_unlock_irqrestore(&port->lock, flags);
}

#if 0
static void 
s5h2x_serial_pm(struct uart_port *, unsigned int state, unsigned int oldstate)
{

}
#endif

static const char*
s5h2x_serial_type(struct uart_port *port)
{
	return (port->type == PORT_S5H2X)? "S5H2X" : NULL ;
}
/*
 * Release IO and memory resources used by the port.
 * This includes iounmap if necessary.
 */
static void 
s5h2x_serial_release_port(struct uart_port *port)
{
	printk(KERN_NOTICE "enter serial_release\n" );
}

/*
 * Request IO and memory resources used by the port.
 * This includes iomapping the port if necessary.
 */
static int 
s5h2x_serial_request_port(struct uart_port *port)
{
	printk(KERN_NOTICE "enter serial_request_port\n" );
	return 0;
}
static void 
s5h2x_serial_config_port(struct uart_port *port, int flags)
{
        if (flags & UART_CONFIG_TYPE && s5h2x_serial_request_port(port) == 0)
                port->type = PORT_S5H2X;
}
static int 
s5h2x_serial_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	int retVal = 0;

	return retVal;
}

#ifdef CONFIG_SERIAL_S5H2X_CONSOLE
static struct console s5h2x_serial_console;
#define S5H2X_SERIAL_CONSOLE	&s5h2x_serial_console
#else
#define S5H2X_SERIAL_CONSOLE	NULL
#endif

#ifndef S5H2X_SERIAL_DEBUG
static struct uart_port s5h2x_ports[S5H2X_UART_NR];
#else
static struct uart_port s5h2x_ports[S5H2X_UART_NR << 1];
#endif

static struct uart_ops	s5h2x_serial_ops = {
        .tx_empty       = s5h2x_serial_tx_empty,
        .set_mctrl      = s5h2x_serial_set_mctrl,
        .get_mctrl      = s5h2x_serial_get_mctrl,
        .stop_tx        = s5h2x_serial_stop_tx,
        .start_tx       = s5h2x_serial_start_tx,
	.send_xchar	= NULL,
        .stop_rx        = s5h2x_serial_stop_rx,
        .enable_ms      = s5h2x_serial_enable_ms, 
        .break_ctl      = s5h2x_serial_break_ctl,
        .startup        = s5h2x_serial_startup,
        .shutdown       = s5h2x_serial_shutdown,
        .set_termios    = s5h2x_serial_set_termios,
        .pm             = NULL,
	.set_wake	= NULL,
        .type           = s5h2x_serial_type,
        .release_port   = s5h2x_serial_release_port,
        .request_port   = s5h2x_serial_request_port,
        .config_port    = s5h2x_serial_config_port,
        .verify_port    = s5h2x_serial_verify_port,
	.ioctl		= NULL,
};

static struct uart_driver s5h2x_uart_drv = {
	.owner			= THIS_MODULE,
	.dev_name 		= "s5h2x_serial",
	.dev_name 		= S5H2X_SERIAL_NAME,
	.nr			= S5H2X_UART_NR,
	.cons			= S5H2X_SERIAL_CONSOLE,
	.driver_name		= S5H2X_SERIAL_NAME,
//	.devfs_name		= S5H2X_SERIAL_DEVFS,
	.major			= S5H2X_SERIAL_MAJOR,
	.minor			= S5H2X_SERIAL_MINOR,
};

#ifdef S5H2X_SERIAL_DEBUG
static struct uart_driver s5h2x_uartd_drv = {
        .owner                  = THIS_MODULE,
        .dev_name               = S5H2X_SERIALD_NAME,
        .nr                     = S5H2X_UART_NR,
        .cons                   = NULL,
        .driver_name            = S5H2X_SERIALD_NAME,
//      .devfs_name             = S5H2X_SERIALD_DEVFS,
        .major                  = S5H2X_SERIALD_MAJOR,
        .minor                  = S5H2X_SERIALD_MINOR,
};
#endif





#if 0
static int s5h2x_serial_probe(struct device *_dev)
{
	struct platform_device *dev = to_platform_device(_dev);
#else
static int s5h2x_serial_probe(struct platform_device *dev)
{
	struct device *_dev = &dev->dev;
#endif
	struct resource *res = dev->resource;
	int i;

	for (i = 0; i < dev->num_resources; i++, res++) {
		if (res->flags & IORESOURCE_MEM) {
			break;
		}
	}

	if ( res->flags & IORESOURCE_MEM ) {
		for (i = 0; i < S5H2X_UART_NR; i++) {
			if (s5h2x_ports[i].mapbase != res->start) {
				continue;
			}
			s5h2x_ports[i].dev = _dev;
			uart_add_one_port(&s5h2x_uart_drv, &s5h2x_ports[i]);
			dev_set_drvdata(_dev, &s5h2x_ports[i]);
			break;
		}
#ifdef S5H2X_SERIAL_DEBUG
                for (i = S5H2X_UART_NR; i < (S5H2X_UART_NR << 1); i++) {
                        if (s5h2x_ports[i].mapbase != res->start) {
                                continue;
                        }
                        s5h2x_ports[i].dev = _dev;
                        uart_add_one_port(&s5h2x_uartd_drv, &s5h2x_ports[i]);
                        dev_set_drvdata(_dev, &s5h2x_ports[i]);
                        break;
                }
#endif
	}

	return 0;
}

#if 0
static int s5h2x_serial_remove(struct device *_dev)
{
	struct uart_port *port = dev_get_drvdata(_dev);

	dev_set_drvdata(_dev, NULL);
#else
static int s5h2x_serial_remove(struct platform_device *dev)
{
        struct uart_port *port= platform_get_drvdata(dev);

	platform_set_drvdata(dev, NULL);
#endif
	if(port) {
#ifdef S5H2X_SERIAL_DEBUG
                if(UART_DEBUG(port)) uart_remove_one_port(&s5h2x_uartd_drv, port);
                else
#endif
		uart_remove_one_port(&s5h2x_uart_drv, port);
	}

	return 0;
}

#if 0
static int s5h2x_serial_suspend(struct device *_dev, u32 state, u32 level)
{
        struct uart_port *port= dev_get_drvdata(_dev);

        if (port && (level == SUSPEND_DISABLE))
#else
static int s5h2x_serial_suspend(struct platform_device *dev, pm_message_t state)
{
        struct uart_port *port= platform_get_drvdata(dev);
#endif
#ifdef S5H2X_SERIAL_DEBUG
                if(UART_DEBUG(port)) uart_suspend_port(&s5h2x_uartd_drv, port);
                else
#endif
                uart_suspend_port(&s5h2x_uart_drv, port);

        return 0;
}

#if 0
static int s5h2x_serial_resume(struct device *_dev, u32 level)
{
        struct uart_port *port= dev_get_drvdata(_dev);

        if (port && (level == RESUME_ENABLE))
#else
static int s5h2x_serial_resume(struct platform_device *dev)
{
        struct uart_port *port= platform_get_drvdata(dev);
#endif
#ifdef S5H2X_SERIAL_DEBUG
                if(UART_DEBUG(port)) uart_resume_port(&s5h2x_uartd_drv, port);
                else
#endif
                uart_resume_port(&s5h2x_uart_drv, port);

        return 0;
}


#if 0
static struct device_driver s5h2x_serial_driver = {
	.name		= "s5h2x-uart",
	.bus		= &platform_bus_type,
	.probe		= s5h2x_serial_probe,
	.remove		= s5h2x_serial_remove,
	.suspend	= s5h2x_serial_suspend,
	.resume		= s5h2x_serial_resume,
};
#else
static struct platform_driver  s5h2x_serial_driver = {
	.probe		= s5h2x_serial_probe,
//	.remove		= __devexit_p(s5h2x_serial_remove),
	.remove		= s5h2x_serial_remove,
	.suspend	= s5h2x_serial_suspend,
	.resume		= s5h2x_serial_resume,
	.driver		= {
		.name 	= "s5h2x-serial",
		.owner 	= THIS_MODULE,
	},
};
#endif

static struct platform_device *s5h2x_uart_devs;

static void __init 
s5h2x_init_port(void)
{
	static int first = 1;
	int i;
#ifdef S5H2X_SERIAL_DEBUG
        int j = S5H2X_UART_NR;
#endif

	if(!first) return;

	first = 0;

	for (i = 0; i < S5H2X_UART_NR; i++) {
		s5h2x_ports[i].membase	 = (void *) (VA_UART_BASE + (S5H2X_UART_GAP * i));
		s5h2x_ports[i].mapbase	 = (PA_UART_BASE + (S5H2X_UART_GAP * i));
		s5h2x_ports[i].iotype	 = SERIAL_IO_MEM;
		s5h2x_ports[i].irq	 = IRQ_UART0 + i;
#if defined(CONFIG_ARCH_S4LXXX)
		s5h2x_ports[i].uartclk   = S4LX_GET_UARTCLK(UART_CLOCK);
#elif  defined(CONFIG_ARCH_SDPXX)
		s5h2x_ports[i].uartclk   = SDPXX_GET_UARTCLK(UART_CLOCK);
#else
		s5h2x_ports[i].uartclk	 = S5H2X_GET_UARTCLK(UART_CLOCK);
#endif
		s5h2x_ports[i].fifosize	 = 16;
		s5h2x_ports[i].timeout   = HZ/50;
		s5h2x_ports[i].unused[0] = 0;
		s5h2x_ports[i].unused[1] = 0;
                s5h2x_ports[i].unused[2] = 0;
		s5h2x_ports[i].ops	 = &s5h2x_serial_ops;
		s5h2x_ports[i].flags	 = ASYNC_BOOT_AUTOCONF;
		s5h2x_ports[i].type	 = PORT_S5H2X;
		s5h2x_ports[i].line      = i;
/* add by tukho.kim 20061113, kernel version upgrade -> 2.6.17 */
                s5h2x_ports[i].ignore_status_mask = 0xE;
/* GDB bug fix 20071018 */
                s5h2x_ports[i].timeout = HZ/50;
	}
#ifdef S5H2X_SERIAL_DEBUG
        for (j = S5H2X_UART_NR; j < (S5H2X_UART_NR << 1); j++) {
                i = j - S5H2X_UART_NR;
                s5h2x_ports[j].membase  = (void *) (VA_UART_BASE + (S5H2X_UART_GAP * i));
                s5h2x_ports[j].mapbase  = (PA_UART_BASE + (S5H2X_UART_GAP * i));
                s5h2x_ports[j].iotype   = SERIAL_IO_MEM;
                s5h2x_ports[j].irq      = IRQ_UART0 + i;
#if defined(CONFIG_ARCH_S4LXXX)
		s5h2x_ports[j].uartclk  = S4LX_GET_UARTCLK(UART_CLOCK);
#elif  defined(CONFIG_ARCH_SDPXX)
		s5h2x_ports[i].uartclk   = SDPXX_GET_UARTCLK(UART_CLOCK);
#else
                s5h2x_ports[j].uartclk  = S5H2X_GET_UARTCLK(UART_CLOCK);
#endif
                s5h2x_ports[j].fifosize = 16;
                s5h2x_ports[j].unused[0]        = 0;
                s5h2x_ports[j].unused[1]        = 0;
                s5h2x_ports[j].unused[2]        = 1;
                s5h2x_ports[j].ops      = &s5h2x_serial_ops;
                s5h2x_ports[j].flags    = ASYNC_BOOT_AUTOCONF;
                s5h2x_ports[j].type     = PORT_S5H2X;
                s5h2x_ports[j].line     =  i;
		s5h2x_ports[j].ignore_status_mask = 0xE;
/* GDB bug fix 20071018 */
                s5h2x_ports[j].timeout = HZ/50;
        }
#endif

}

static int __init 
s5h2x_serial_modinit(void)
{
	int i, retVal = 0;

	printk(KERN_INFO "Serial: S5H2X driver $Revision : 1.01 $\n");

	s5h2x_init_port();
	
	retVal = uart_register_driver(&s5h2x_uart_drv);
#ifdef S5H2X_SERIAL_DEBUG
        if (retVal == 0) {
                retVal = uart_register_driver(&s5h2x_uartd_drv);
	        if (retVal)
                        uart_unregister_driver(&s5h2x_uart_drv);
        }
#endif

	if(retVal) 
		goto out;
#if 0
	if (retVal == 0) {
		retVal = driver_register(&s5h2x_serial_driver);
		if (retVal)
			uart_unregister_driver(&s5h2x_uart_drv);
	}
#else
	s5h2x_uart_devs = platform_device_alloc("s5h2x-uart", -1);

        if (!s5h2x_uart_devs) {
                retVal = -ENOMEM;
                goto unreg_uart_drv;
        }

        retVal = platform_device_add(s5h2x_uart_devs);
        if (retVal)
                goto put_dev;

	for(i = 0; i < S5H2X_UART_NR; i++){
		struct uart_port *port = &s5h2x_ports[i];
		
		port->dev = &s5h2x_uart_devs->dev;
		uart_add_one_port(&s5h2x_uart_drv, port);
#ifdef S5H2X_SERIAL_DEBUG
		{
			struct uart_port *pDPort = &s5h2x_ports[i + S5H2X_UART_NR];
		
			pDPort->dev = &s5h2x_uart_devs->dev;
			uart_add_one_port(&s5h2x_uartd_drv, pDPort);
		}
#endif
	}


        retVal = platform_driver_register(&s5h2x_serial_driver);
        if (retVal == 0)
                goto out;

        platform_device_del(s5h2x_uart_devs);
 put_dev:
        platform_device_put(s5h2x_uart_devs);
 unreg_uart_drv:
        uart_unregister_driver(&s5h2x_uart_drv);
#ifdef S5H2X_SERIAL_DEBUG
        uart_unregister_driver(&s5h2x_uartd_drv);
#endif
#endif
 out:
	return retVal;
}

static void __exit 
s5h2x_serial_modexit(void)
{
	struct platform_device *temp_devs = s5h2x_uart_devs;

	s5h2x_uart_devs = NULL;

	platform_driver_unregister(&s5h2x_serial_driver);
	platform_device_unregister(temp_devs);
	uart_unregister_driver(&s5h2x_uart_drv);
#ifdef S5H2X_SERIAL_DEBUG
        uart_unregister_driver(&s5h2x_uartd_drv);
#endif
}

module_init(s5h2x_serial_modinit);
module_exit(s5h2x_serial_modexit);

#ifdef CONFIG_SERIAL_S5H2X_CONSOLE
static void 
s5h2x_console_write(struct console *co, const char *s, unsigned count)
{
        struct uart_port *port = s5h2x_ports + co->index;
        int i, status;
	unsigned int ucon;

#ifdef S5H2X_SERIAL_DEBUG
        if (UART_MODE(port) & MODE_DEBUG) return;
#endif
	ucon = UART_GET_UCON(port);

	UART_PUT_UCON(port, (ucon & ~(S5H2X_UCON_TXIE)));

	
        /*
         *      Now, do each character
         */
        for (i = 0; i < count; i++)
        {
                while( (UART_GET_UTRSTAT(port) & 0x2) != 0x2 )
                        continue/*nop*/;
                UART_PUT_CHAR(port, (s[i]) & 0xff);

                if (s[i] == '\n') {
                        while( (UART_GET_UTRSTAT(port) & 0x2) != 0x2 )
                                continue/*nop*/;
                        UART_PUT_CHAR(port, '\r');
                }
        }
        /* Clear TX Int status */
        status = UART_GET_UTRSTAT(port);
        UART_PUT_UTRSTAT(port, (status & 0x0F) | S5H2X_UTRSTAT_TXI );

	UART_PUT_UCON(port, ucon);
}

#if 0
static int 
s5h2x_console_read(struct console *c, char *s, unsigned count)
{
	int retVal = 0;
	
	return retVal;
}
#endif

static void __init
s5h2x_uart_console_get_options(struct uart_port *port, int *baud, int *parity,
                               int *bits)
{
        *baud = CURRENT_BAUD_RATE;
        *bits = 8;
        *parity = 'n';
}

static int __init 
s5h2x_console_setup(struct console *co, char *options)
{
	struct uart_port *port;
	int baud = CURRENT_BAUD_RATE;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';
	
	if(co->index >= S5H2X_UART_NR) {
		co->index = 0;
	}

	port = s5h2x_ports + co->index;

	s5h2x_init_port();

	if (options) {
		uart_parse_options(options, &baud, &parity, &bits, &flow);
	}
	else {
		s5h2x_uart_console_get_options(port, &baud, &parity, &bits);
	}

	UART_PUT_UCON(port, 0x05);  //Tx, Rx mode setting(Interrupt request or polling mode)

	return uart_set_options(port, co, baud, parity, bits, flow);
}

static struct console s5h2x_serial_console = {
	.name 		= S5H2X_SERIAL_NAME,
	.device		= uart_console_device,
	.flags		= CON_PRINTBUFFER,
	.index		= -1,
	.write		= s5h2x_console_write,
	.read		= NULL,
	.setup		= s5h2x_console_setup,
	.data		= &s5h2x_uart_drv,
};

static int __init 
s5h2x_serial_initconsole(void)
{
#if 0
	if(!(s5h2x_serial_console.flags & CON_ENABLED))
#endif
		register_console(&s5h2x_serial_console);

	return 0;
}

console_initcall(s5h2x_serial_initconsole);

#endif /* CONFIG_SERIAL_S5H2X_CONSOLE */

MODULE_AUTHOR("tukho.kim@samsung.com");
MODULE_DESCRIPTION("Samsung S5H2xxx Serial port driver");
