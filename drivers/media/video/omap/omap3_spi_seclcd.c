/*
 * omap2_mcspi_seclcdtest.c
 *
 * Test Secondary LCD using McSPI driver
 *
 * Copyright (C) 2006 Texas Instruments, Inc.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 *
 */
	     

#define __NO_VERSION__  1

#include <linux/module.h> 



#include <linux/init.h>
#include <linux/kernel.h> 
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <asm/arch/dma.h>
#include <asm/system.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <asm/arch/hardware.h>
#include <linux/spi/spi.h>
#include <asm/arch/power_companion.h>
#include <linux/videodev.h>
#include <linux/pci.h>
#include <media/videobuf-dma-sg.h>
#include <linux/dma-mapping.h>
#include <media/v4l2-dev.h>
#include <linux/workqueue.h>
#include <asm/semaphore.h>

#ifdef CONFIG_PM
#include <linux/notifier.h>
#include <linux/pm.h>
#endif

#ifdef CONFIG_DPM
#include <linux/dpm.h>
#endif


/* #define DEBUG */

#undef DEBUG
#ifdef DEBUG
#define DPRINTK(ARGS...)  printk("<%s>: ",__FUNCTION__);printk(ARGS)
#else
#define DPRINTK( x... )
#endif

#include "omap3_spi_seclcd.h"
#include "war.h"
#include "omap24xxlib.h"
#include "omap24xxvoutdef.h"

extern int twl4030_free_gpio(int gpio);
extern int twl4030_request_gpio(int gpio);
extern int twl4030_set_gpio_direction(int gpio, int is_input);
extern int twl4030_set_gpio_dataout(int gpio, int enable);

/* TODO -- verify the max sizes */
/* 
 * Maximum amount of memory to use for rendering buffers.
 * Default is enough to four (RGB24) VGA buffers. 
 */
#define MAX_ALLOWED_VIDBUFFERS            4
static int render_mem = (PAGE_ALIGN(49 * 68 * 3)) * MAX_ALLOWED_VIDBUFFERS;

#define VOUT_NAME	"omap3auxlcd"
struct omap3_aux_disp_device	*aux_disp_dev = NULL;
struct omap3_aux_disp_fh		*aux_disp_fh = NULL;
static struct videobuf_queue_ops dummy_vbq_ops;
static int rotation_support = -1;
static struct semaphore que_sem;

static void omap3_aux_disp_send(struct work_struct * work);

DECLARE_WORK(aux_disp_work, omap3_aux_disp_send);

static struct spi_device *sublcd_spi;
static struct ssd1788_sublcd_cfg {
	unsigned char grayscale_mode; /* only 16 bit so far */
	unsigned char col_addr_start;
	unsigned char col_addr_end;
	unsigned char page_addr_start;
	unsigned char page_addr_end;
	u8 rotation; /* 0, 1, 2, 3 */
	unsigned char scan_dir;
	unsigned short int contrast; /* 0 - 15 */
	unsigned char display_mode;
} sublcd_obj;

struct ssd1788_sublcd_window{
	unsigned int Left;
	unsigned int Top;
	unsigned int Width;
	unsigned int Height;
} active_win;

static int
spi_lcd_cmd_send(unsigned char cmd)
{

	int ret=0;
	unsigned int cmd_word = 0;

	cmd_word = (0x000) | cmd;
	spi_write(sublcd_spi,(unsigned char*)&cmd_word, 2);
	udelay(10);
	return ret;
}

static int
spi_lcd_data_send(unsigned char data_tx)
{

	int ret=0;
	unsigned int data_word = 0;
	
	data_word = (0x100) | data_tx;
	spi_write(sublcd_spi, (unsigned char*) &data_word, 2);
	udelay(10);

	return ret;
}

void
sec_lcd_req_gpios(void)
{
	/* GPIO15 is reset, GPIO7 is back light */
	twl4030_request_gpio(15);
	twl4030_set_gpio_direction(15,0);
		
	twl4030_request_gpio(7);
        twl4030_set_gpio_direction(7,0);
	return;
}


static int
reset_seclcd(void)
{


        twl4030_set_gpio_dataout(15,1);
	udelay(10);
        twl4030_set_gpio_dataout(15,0);
        udelay(10);
        twl4030_set_gpio_dataout(15,1);
        udelay(10);
	return 0;
}

	void
sec_lcd_bklight_enable(void)
{
	twl4030_set_gpio_dataout(7,1);
	udelay(10);	
	return;
}

	void
sec_lcd_bklight_disable(void)
{
	twl4030_set_gpio_dataout(7,0);
	udelay(10);	
	return;
}

int
ssd1788_contrast(int inc)
{

  if(inc)
	spi_lcd_cmd_send(INCREMENT_CONTRAST_SET);
  else
	spi_lcd_cmd_send(DECREMENT_CONTRAST_SET);

	return 0;
}

int
ssd1788_calc_addr(int width, int height)
{
	if((width > SSD1788_MAX_WIDTH) || (height > SSD1788_MAX_HEIGHT))
		return -EINVAL;
	sublcd_obj.col_addr_start = ((SSD1788_MAX_WIDTH - width)>>1);
	sublcd_obj.col_addr_end = sublcd_obj.col_addr_start + width;
	sublcd_obj.page_addr_start = ((SSD1788_MAX_HEIGHT - height)>>1);
	sublcd_obj.page_addr_end = sublcd_obj.page_addr_start + height;
	return 0;
}


void
ssd1788_config_display(void)
{

	spi_lcd_cmd_send(SET_PAGE_ADDR);
	spi_lcd_data_send(sublcd_obj.page_addr_start);
	spi_lcd_data_send(sublcd_obj.page_addr_end);
	
	/*Set the coloumn address*/
	spi_lcd_cmd_send(SET_COLUMN_ADDR);
	spi_lcd_data_send(sublcd_obj.col_addr_start);
	spi_lcd_data_send(sublcd_obj.col_addr_end);

	return;
}

/* rot 0 -> 0 degrees
1 -> 90 degrees
2 -> 180 degrees
3 -> 270 degrees
*/

int
ssd1788_config_rotation(unsigned int rot)
{

	sublcd_obj.rotation = rot;

	switch(rot){
	case 0:
	  printk("setting scan 0\n");
		sublcd_obj.scan_dir = COLUMN_SCAN_CN_PN;
		break;
	case 1:
		sublcd_obj.scan_dir = PAGE_SCAN_CI_PN;
		break;
	case 2:
		sublcd_obj.scan_dir = COLUMN_SCAN_CI_PI;
		break;
	case 3:
		sublcd_obj.scan_dir = PAGE_SCAN_CN_PI;
		break;
	default:
		return -EINVAL;
	}
	spi_lcd_cmd_send(SET_DATA_OUTPUT_SCAN_DIR);
	spi_lcd_data_send(sublcd_obj.scan_dir);
	spi_lcd_data_send(0x00);
	spi_lcd_data_send(sublcd_obj.grayscale_mode);
	return 0;
}

int ssd1788_initialize_sec_lcd(void)
{
	unsigned char frame_frq = (unsigned char) FRAME_FREQ_78MHZ;
	unsigned char booster_level = (unsigned char) BOOST_LEVEL_5X;        
	unsigned char com_scan_dir = (unsigned char) COM_SCAN_DIR2;

	printk("Entered Initialise_sec_lcd\n");

	sublcd_obj.contrast = CONTRAST_LEVEL_DEFAULT;
	sublcd_obj.display_mode = DISPLAY_INVERSE;
	sublcd_obj.grayscale_mode = GRAY_SCALE_16_BIT_MODE;
	sublcd_obj.scan_dir = COLUMN_SCAN_CN_PN;

	/*----------------enable interal oscillator-----------------*/
	spi_lcd_cmd_send(ENABLE_INT_OSCILLATOR);
	/*----------------set power control register----------------*/
	spi_lcd_cmd_send(SET_POWER_CONTROL_REG);
	/* enable the reference voltage genarator */
	/* internal regulator and voltage follower */
	booster_level |= 0x03;
	spi_lcd_data_send(booster_level);//boost level//
	/*-----------set contrast level & interal regulator----------*/
	spi_lcd_cmd_send(SET_CONTRAST_LVL_AND_RESI_RATIO);
       	spi_lcd_data_send(sublcd_obj.contrast);// contrast level
	spi_lcd_data_send(0x07); // resistor ratio for intrnal
	// regulator gain(1+r2/r1)=8.89
	/*----------------------exit sleep mode----------------------*/
	spi_lcd_cmd_send(EXIT_SLEEP_MODE);
	/*---------------------set biasing ratio---------------------*/
	spi_lcd_cmd_send(SET_BIAS_RATIO);
	spi_lcd_data_send(0x03);//bias ratio - 1/7
	/*---------------------select PWM/FRC------------------------*/
	spi_lcd_cmd_send(SELECT_PWD_OR_FRC);
	spi_lcd_data_send(0x28);//constant value
	spi_lcd_data_send(0x2E);/*2PWR + 2FRC*/
	spi_lcd_data_send(0x5); //constant value
	/*----------------set com output scan direction--------------*/
	spi_lcd_cmd_send(SET_COM_OUTPUT_SCAN_DIR);
	spi_lcd_data_send(com_scan_dir);//com_scan_dir
	/*----------------set data out scan direction----------------*/
       	ssd1788_config_rotation(0);
	/*---------------------set display control------------------*/
	spi_lcd_cmd_send(SET_DISPLAY_CONTROL);
	spi_lcd_data_send(0x00);//dummy byte
	spi_lcd_data_send(0x0F);// display control
	spi_lcd_data_send(0x00);//dummy byte
	/*--------set frame frequency and n-line inversion----------*/
	spi_lcd_cmd_send(SET_FRAME_FRQ_NLINE_INVER);
	spi_lcd_data_send(frame_frq);//frame frq; 
	spi_lcd_data_send(0x06);// N-line inversion
	/*--------------------normal/inverse display----------------*/
	spi_lcd_cmd_send(SET_INVERSE_DISPLAY);//
	/*----------------------set display on----------------------*/
	spi_lcd_cmd_send(SET_DISPLAY_ON);
	return (0);
}

int
spi_lcd_fill_colour(void)
{   
	unsigned int i, j;
	unsigned char colour1, colour2, colour3;
	
	printk("entered spi_lcd_fill_colour\n");


	colour1 = 0xf0;
	colour2 = 0x0f;
	colour3 = 0x00;

	/*Complete Sub LCD Screen*/
	sublcd_obj.page_addr_start = 0;
	sublcd_obj.page_addr_end = 67;
	sublcd_obj.col_addr_start = 0 ;
	sublcd_obj.col_addr_end = 48;

	ssd1788_config_display();

	/*data write to LCD*/
	spi_lcd_cmd_send(WRITE_DISPLAY_DATA);
	/*Send the colour data to LCD*/

	for(i = 0; i<= ((sublcd_obj.page_addr_end - sublcd_obj.page_addr_start)); i++)
	{  
		for(j = 0; j<= (sublcd_obj.col_addr_end - sublcd_obj.col_addr_start); j++)
		{
			if(ar[i][j]==1){
				spi_lcd_data_send(0xff);
				spi_lcd_data_send(0xff);
				spi_lcd_data_send(0xff);
			}
			else if(ar[i][j]==0){
				spi_lcd_data_send(colour1);
				spi_lcd_data_send(colour2);
				spi_lcd_data_send(colour3);
			}
			else{
				spi_lcd_data_send(0x00);
				spi_lcd_data_send(0xF0);
				spi_lcd_data_send(0x0F);
			}
		}
	}
	return 0;
}


void spi_lcd_render(unsigned char *buf)
{
	unsigned int	i,j,count;





	sublcd_obj.page_addr_start = 0;
	sublcd_obj.page_addr_end = 67;
	sublcd_obj.col_addr_start = 0;
	sublcd_obj.col_addr_end = 48;

	ssd1788_config_display();

	/* Data write to LCD */
	spi_lcd_cmd_send(WRITE_DISPLAY_DATA);

	count = 0;
	for (i=0; i<=((sublcd_obj.page_addr_end - sublcd_obj.page_addr_start)); i++)
	{
			for (j=0; j<=(sublcd_obj.col_addr_end - sublcd_obj.col_addr_start); j++)
			{
					spi_lcd_data_send(buf[count]);
					count++;
					spi_lcd_data_send(buf[count]);
					count++;
					spi_lcd_data_send(buf[count]);
					count++;
			}
	}
}

static void omap3_aux_disp_send(struct work_struct *work) {
		struct aux_disp_buf		disp_buf;
		struct aux_disp_queue_hdr	*queue_hdr;
		int		processed,slot;

		DPRINTK("Entering <--- \n");
		queue_hdr = &(aux_disp_dev->aux_queue_hdr);
		processed = queue_hdr->processed;

		slot = processed % (queue_hdr->queue_depth);
		disp_buf = queue_hdr->aux_disp_queue[slot];

		DPRINTK("calling to send to SPI: slot=%d : spi_lcd_render: buf_ptr=0x%x \n",slot,disp_buf.data);
		spi_lcd_render((unsigned char*)(disp_buf.data));

		processed++;
		queue_hdr->processed = processed;
		up(&que_sem);
		DPRINTK("Leaving ---> \n");
}

static int
omap3auxvout_do_ioctl (struct inode *inode, struct file *file,
		       unsigned int cmd, void *arg)
{
	struct omap3_aux_disp_fh *fh = (struct omap3_aux_disp_fh *)file->private_data;
	struct omap3_aux_disp_device *vout = fh->vout;



	switch (cmd) {
	case VIDIOC_ENUMOUTPUT:
	{
		struct v4l2_output *output = (struct v4l2_output *) arg;
		int index = output->index;

		if (index > 0)
			return -EINVAL;

			memset (output, 0, sizeof (*output));
			output->index = index;

			strncpy (output->name, "video out", sizeof (output->name));
			output->type = V4L2_OUTPUT_TYPE_MODULATOR;
			return 0;
	}
			
	case VIDIOC_G_OUTPUT:
	{
		unsigned int *output = arg;
		*output = 0;
		return 0;
	}
		
	case VIDIOC_S_OUTPUT:
	{
		unsigned int *output = arg;

		if (*output > 0)
			return -EINVAL;
		return 0;
	}
		
	case VIDIOC_QUERYCAP:
	{
		struct v4l2_capability *cap = (struct v4l2_capability *) arg;
		memset (cap, 0, sizeof (*cap));
		strncpy (cap->driver, VOUT_NAME, sizeof (cap->driver));
		strncpy (cap->card, vout->vfd->name, sizeof (cap->card));
		cap->bus_info[0] = '\0';
		cap->capabilities = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_OUTPUT;
		return 0;
	}
			
	case VIDIOC_G_FMT:
	{
		struct v4l2_format *fmt = (struct v4l2_format *) arg;
		switch (fmt->type){
			case V4L2_BUF_TYPE_VIDEO_OUTPUT:
			{
				struct v4l2_pix_format *pix = &fmt->fmt.pix;
				memset(pix, 0, sizeof(*pix));
				*pix = vout->pix;
				return 0;							
			}
 			default: 
				return -EINVAL;
			}
	}
			
	case VIDIOC_S_FMT:
	{
		struct v4l2_format *fmt = (struct v4l2_format *) arg;
	
		if(fmt->fmt.pix.pixelformat != V4L2_PIX_FMT_RGB444)
			{	
					return -EINVAL;
			}	
			return 0;	
	}	
	
	case VIDIOC_REQBUFS:
	{
		struct v4l2_requestbuffers *req = (struct v4l2_requestbuffers *) arg;
		struct videobuf_queue *q = &fh->vbq;
		unsigned int phy_addr, virt_addr;
		unsigned int count, i;

		if (req->count > VIDEO_MAX_FRAME)
			req->count = VIDEO_MAX_FRAME;

		/* we allow req->count == 0 */
		if ((req->type != V4L2_BUF_TYPE_VIDEO_OUTPUT) || (req->count < 0))
			return -EINVAL;

		if (req->memory != V4L2_MEMORY_MMAP)
			return -EINVAL;

		if (vout->streaming)
			return -EBUSY;

		/* reuse old buffers */
		if (vout->buffer_allocated && vout->buffer_size >= vout->pix.sizeimage){
			if (req->count) {
				req->count = vout->buffer_allocated;
				return 0;
			}
		}

		/* 
		 * We allow re-request as long as old buffers are not mmaped.
		 * We will need to free old buffers first.
		 */

		/* either no buffer allocated or too small */
		if (vout->buffer_allocated){
			if (vout->mmap_count)
				return -EBUSY;
			mutex_lock(&q->lock);
			for (i = 0; i < vout->buffer_allocated; i++){
				struct videobuf_dmabuf *dma = videobuf_to_dma(q->bufs[i]);

				dma_free_coherent(NULL, vout->buffer_size,
					(void *) dma->vmalloc,
					dma->bus_addr);
				q->bufs[i] = NULL;
			}
			vout->buffer_allocated = 0;
			vout->buffer_size = 0;
			mutex_unlock(&q->lock);
			videobuf_mmap_free(q);
		}
		/* user intends to free buffer */
		if (!req->count)
			return 0;

		mutex_lock(&q->lock);
		count = req->count;
		while (PAGE_ALIGN(vout->pix.sizeimage) * count > render_mem)
			count--;

		vout->buffer_size = PAGE_ALIGN(vout->pix.sizeimage); 
		for (i = 0; i < count; i++) {
			struct videobuf_dmabuf *dma;

			virt_addr =
			(unsigned int)dma_alloc_coherent(NULL,
				vout->buffer_size,
				(dma_addr_t *) & phy_addr,
				GFP_KERNEL | GFP_DMA);

			if (!virt_addr)
				break;
			memset((void *) virt_addr, 0, vout->buffer_size);

			DPRINTK("REQBUFS: buffer %d: virt=0x%x, phy=0x%x, size=0x%x\n",
				i, virt_addr, phy_addr, vout->buffer_size);
			q->bufs[i] = videobuf_alloc(q);
			if (q->bufs[i] == NULL){
				dma_free_coherent(NULL, vout->buffer_size,
					(void *) virt_addr,
					(dma_addr_t) phy_addr);
				break;
			}
			q->bufs[i]->i = i;
			q->bufs[i]->input = UNSET;
			q->bufs[i]->memory = req->memory;
			q->bufs[i]->bsize = vout->buffer_size;
			q->bufs[i]->boff = vout->buffer_size * i;
			dma = videobuf_to_dma(q->bufs[i]);
			dma->vmalloc = (void *) (virt_addr);
			dma->bus_addr = phy_addr;
			q->bufs[i]->state = STATE_PREPARED;

			vout->buf_virt_addr[i] = (unsigned long) dma->vmalloc;
			vout->buf_phy_addr[i] = dma->bus_addr;

			vout->buffer_allocated++;
		}
		vout->buf_memory_type = req->memory; 

		/* Initialize the aux buffer queue structure */
		vout->aux_queue_hdr.queue_depth = count;
		vout->aux_queue_hdr.queued = 0;
		vout->aux_queue_hdr.dequeued = count;
		vout->aux_queue_hdr.processed = 0;
		vout->aux_queue_hdr.aux_disp_queue = kmalloc(count*(sizeof(struct aux_disp_buf)),GFP_KERNEL);
		DPRINTK("Initialized the auxiliary display buffer header \n");
		/* return -ENOMEM if we are not able to allocate a single buffer
		 * we have to seperate the case which user requests 0 buffer which
		 * means user wants to free the already allocated buffers
		 */
		sema_init(&que_sem,count);		
		for (i=0; i<count; i++ ) {
				down_interruptible(&que_sem);
		}
		if (vout->buffer_allocated == 0 && req->count != 0){
			mutex_unlock(&q->lock);
			vout->buffer_size = 0;
			return -ENOMEM;
		}
		req->count = vout->buffer_allocated;
		mutex_unlock(&q->lock);
		return 0;
	}
	case VIDIOC_QUERYBUF:
		return videobuf_querybuf (&fh->vbq, arg);	
	case VIDIOC_STREAMON:
	{

		if (vout->streaming)
			return -EBUSY;
		/* 
		 * If rotation mode is selected then allocate a 
		 *  dummy or hidden buffer to map the SMS virtual space 
		 */
		/* set flag here. Next QBUF will start DMA */
		vout->streaming = (struct omap24xxvout_fh *)fh;
		/* Initialize the queue structure here */
		vout->aux_queue_hdr.queued = 0;
		vout->aux_queue_hdr.dequeued = vout->aux_queue_hdr.queue_depth;
		vout->aux_queue_hdr.processed = 0;
		/* TODO -- flush the work queue */
		
		return 0;
	}
	case VIDIOC_STREAMOFF:
	{
		/* 
		 * what is to be done here ?
		 */
		return 0;
	}	
	case VIDIOC_QBUF:
	{
		struct v4l2_buffer		*buffer = (struct v4l2_buffer *) arg;
		struct videobuf_queue	*q = &fh->vbq;
		struct aux_disp_queue_hdr	*queue_hdr;
		struct aux_disp_buf			buf_elem;
		int 	queued,slot;
		struct videobuf_dmabuf *dma = videobuf_to_dma(q->bufs[buffer->index]);
		
		DPRINTK("queuing buf-index:0%d \n",buffer->index);
		queue_hdr = &(aux_disp_dev->aux_queue_hdr);
		queued = queue_hdr->queued;
		slot = queued % (queue_hdr->queue_depth);
		buf_elem.index = buffer->index;
		buf_elem.data = dma->vmalloc;
		queue_hdr->aux_disp_queue[slot] = buf_elem;
		
		queued++;
		queue_hdr->queued = queued;
		/* schedule the work */
		schedule_work(&aux_disp_work);
		
		return 0;
	}
	case VIDIOC_DQBUF:
	{
		struct v4l2_buffer		*buffer = (struct v4l2_buffer *) arg;

		struct aux_disp_queue_hdr	*queue_hdr;
		struct aux_disp_buf			buf_elem;
		int 	dequeued,processed,queue_depth,slot;

		DPRINTK("De queuing Request !!! \n");
		queue_hdr = &(aux_disp_dev->aux_queue_hdr);
		dequeued = queue_hdr->dequeued;
		processed = queue_hdr->processed;
		queue_depth = queue_hdr->queue_depth;
			
		if (down_interruptible(&que_sem))
		{
			printk("DQUEUE: down_interruptible failed !!! \n");
			return -1;
		}
		slot = dequeued % queue_depth;
		buf_elem = queue_hdr->aux_disp_queue[slot];
		buffer->index = buf_elem.index;
		
		dequeued++;
		queue_hdr->dequeued = dequeued;

		return 0;	
	}	
	default:
		return ENOIOCTLCMD;

	}

	return 0;
}




/* define this to allow videobuf_mmap_free to work */
static void dummy_vbq_release(struct videobuf_queue *q, struct videobuf_buffer *vb)
{
}

/* TODO -- define the file operations here */
static int
omap3auxvout_open (struct inode *inode, struct file *file)
{

	struct omap3_aux_disp_device *vout = aux_disp_dev;
	struct omap3_aux_disp_fh *fh;
	struct videobuf_queue *q;
	int i;

	DPRINTK ("entering\n");

	if (vout == NULL)
		return -ENODEV;

	/* for now, we only support single open */
	if (vout->opened)
		return -EBUSY;

	vout->opened += 1;
	

	/* allocate per-filehandle data */
	fh = kmalloc (sizeof (*fh), GFP_KERNEL);
	if (NULL == fh){
		vout->opened -= 1;
		return -ENOMEM;
	}

	file->private_data = fh;
	fh->vout = vout;
	fh->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	/* TODO -- Do we need to store the file handle as a global variable ? */
	q = &fh->vbq;
	dummy_vbq_ops.buf_release = dummy_vbq_release;
	videobuf_queue_pci_init (q, &dummy_vbq_ops, NULL, &vout->vbq_lock,
		fh->type, V4L2_FIELD_NONE, sizeof (struct videobuf_buffer), fh);

	/* restore info for mmap */
	for (i = 0; i < vout->buffer_allocated; i++) {
		struct videobuf_dmabuf *dma;

		q->bufs[i] = videobuf_alloc(q);
		if (q->bufs[i] == NULL){
			dma_free_coherent(NULL, vout->buffer_size,
					(void *) vout->buf_virt_addr[i],
					(dma_addr_t) vout->buf_phy_addr[i]);
			/* we have to give up some old buffers */
			vout->buffer_allocated = i;
			break;
		}
		q->bufs[i]->i = i;
		q->bufs[i]->input = UNSET;
		q->bufs[i]->memory = vout->buf_memory_type;
		q->bufs[i]->bsize = vout->buffer_size;
		q->bufs[i]->boff = vout->buffer_size * i;
		dma = videobuf_to_dma(q->bufs[i]);
		dma->vmalloc = (void *) vout->buf_virt_addr[i];
		dma->bus_addr = vout->buf_phy_addr[i];
		q->bufs[i]->state = STATE_PREPARED;
	}
	/* enable the LCD back light */
	sec_lcd_bklight_enable();
	return 0;
}
static void
omap3auxvout_vm_open (struct vm_area_struct *vma)
{
	struct omap3_aux_disp_device *vout = vma->vm_private_data;
	DPRINTK ("vm_open [vma=%08lx-%08lx]\n", vma->vm_start, vma->vm_end);
	vout->mmap_count++;
}

static void
omap3auxvout_vm_close (struct vm_area_struct *vma)
{
	struct omap3_aux_disp_device *vout = vma->vm_private_data;
	DPRINTK ("vm_close [vma=%08lx-%08lx]\n", vma->vm_start, vma->vm_end);
	vout->mmap_count--;
}

static struct vm_operations_struct omap3_aux_vout_vm_ops = {
	.open = omap3auxvout_vm_open,
	.close = omap3auxvout_vm_close,
};

static int
omap3auxvout_mmap (struct file *file, struct vm_area_struct *vma)
{
	struct omap3_aux_disp_fh *fh = file->private_data;
	struct omap3_aux_disp_device *vout = fh->vout;
	struct videobuf_queue *q = &fh->vbq;
	unsigned long size = (vma->vm_end - vma->vm_start), start = vma->vm_start;
	int i;
	void *pos;
	struct videobuf_dmabuf *dma;

	DPRINTK ("pgoff=0x%x, start=0x%x, end=0x%x\n", vma->vm_pgoff, 
		vma->vm_start,  vma->vm_end);

	mutex_lock(&q->lock);

	/* look for the buffer to map */
	for (i = 0; i < VIDEO_MAX_FRAME; i++){
		if (NULL == q->bufs[i])	continue;
		if (V4L2_MEMORY_MMAP != q->bufs[i]->memory)
			continue;
		if (q->bufs[i]->boff == (vma->vm_pgoff << PAGE_SHIFT))
			break;
	}

	if (VIDEO_MAX_FRAME == i){
		DPRINTK ("offset invalid [offset=0x%lx]\n",
	       (vma->vm_pgoff << PAGE_SHIFT));
		mutex_unlock(&q->lock);
		return -EINVAL;
	}

	vma->vm_flags |= VM_RESERVED;
	vma->vm_page_prot = pgprot_writecombine (vma->vm_page_prot);
	vma->vm_ops = &omap3_aux_vout_vm_ops;
	vma->vm_private_data = (void *) vout;
	dma = videobuf_to_dma(q->bufs[i]);
	pos = dma->vmalloc;

	while (size > 0) {      /* size is page-aligned */
	if (vm_insert_page(vma, start, vmalloc_to_page(pos) )){
		mutex_unlock(&q->lock);
		return -EAGAIN;
	}
	start += PAGE_SIZE;
	pos += PAGE_SIZE;
	size -= PAGE_SIZE;
	}
	vma->vm_flags &= ~VM_IO; /* using shared anonymous pages */




	mutex_unlock(&q->lock);
	vout->mmap_count++;
	return 0;
}


static int
omap3auxvout_ioctl (struct inode *inode, struct file *file, unsigned int cmd,
		    unsigned long arg)
{
	return video_usercopy (inode, file, cmd, arg, omap3auxvout_do_ioctl);
}

static int
omap3auxvout_release (struct inode *inode, struct file *file)
{
	struct omap3_aux_disp_fh *fh = file->private_data;
	struct omap3_aux_disp_device *vout;
	struct videobuf_queue *q;

	DPRINTK ("entering\n");

	if (fh == 0)
		return 0;
	if ((vout = fh->vout) == 0)
		return 0;
	q = &fh->vbq;

	if ((struct omap3_aux_disp_fh *)vout->streaming == fh) vout->streaming = NULL;

	if (vout->mmap_count != 0){
		vout->mmap_count = 0;
		printk("mmap count is not zero!\n");
	}

	vout->opened -= 1;
	file->private_data = NULL;

	if (vout->buffer_allocated)
		videobuf_mmap_free(q);

	kfree (fh);
	
	sec_lcd_bklight_disable();

	return 0;
}


static struct file_operations omap3auxvout_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.ioctl = omap3auxvout_ioctl,
	.mmap = omap3auxvout_mmap,
	.open = omap3auxvout_open,
	.release = omap3auxvout_release,
};

int init_aux_vout_device (void)
{

	struct omap3_aux_disp_device *vout;
	struct video_device *vfd;
	struct v4l2_pix_format *pix;

	vout = kmalloc (sizeof (struct omap3_aux_disp_device), GFP_KERNEL);
	if (!vout){
		printk (KERN_ERR VOUT_NAME ": could not allocate memory \n");
		return -1;
	}

	memset (vout, 0, sizeof (struct omap3_aux_disp_device));
	vout->rotation = rotation_support;

	/* set the default pix */
	pix = &vout->pix;
	pix->width = 49; //QQVGA_WIDTH;
	pix->height = 68; //QQVGA_HEIGHT;

	/* TODO -- verify the below defines */
	pix->pixelformat = V4L2_PIX_FMT_RGB444; //V4L2_PIX_FMT_RGB565;
	pix->field = V4L2_FIELD_NONE;
	pix->bytesperline = pix->width * 3;
	pix->sizeimage = pix->bytesperline * pix->height;
	pix->priv = 0;
	pix->colorspace = V4L2_COLORSPACE_JPEG;

	vout->bpp = RGB565_BPP;
	
	/* initialize the video_device struct */
	vfd = vout->vfd = video_device_alloc ();
	if (!vfd){
		printk (KERN_ERR VOUT_NAME": could not allocate video device struct\n");
		kfree (vout);
		return -1;
	}
	vfd->release = video_device_release;

	strncpy (vfd->name, VOUT_NAME, sizeof (vfd->name));
	vfd->type = VID_TYPE_OVERLAY | VID_TYPE_CHROMAKEY;
	/* need to register for a VID_HARDWARE_* ID in videodev.h */
	//vfd->hardware = 0;
	vfd->fops = &omap3auxvout_fops;
	video_set_drvdata (vfd, vout);
	vfd->minor = -1;
	
	vout->suspended = 0;
	init_waitqueue_head (&vout->suspend_wq);

	/* register for device 3 */
	if (video_register_device (vfd, VFL_TYPE_GRABBER, 3) < 0){
		printk (KERN_ERR VOUT_NAME": could not register Video for Linux device\n");
		vfd->minor = -1;
	}

#if 0
#ifdef CONFIG_DPM
	/* Scaling is enabled only when DPM is enabled */
	if(vid == OMAP2_VIDEO1){
		dpm_register_scale(&omap24xxv1out_pre_scale,SCALE_PRECHANGE);
		dpm_register_scale(&omap24xxv1out_post_scale,SCALE_POSTCHANGE);
	}
	else{
		dpm_register_scale(&omap24xxv2out_pre_scale,SCALE_PRECHANGE);
		dpm_register_scale(&omap24xxv2out_post_scale,SCALE_POSTCHANGE);
	}
#endif
#endif

	/*if rotation support */
	printk (KERN_INFO VOUT_NAME ": registered device video%d [v4l2]\n",
		vfd->minor);
	aux_disp_dev = vout;	
	return 0;

}

static int sublcd_probe(struct spi_device *spi)
{
	printk("in sublcd driver probe");

	/* Is this the right spot to request GPIO resources ? */
	sec_lcd_req_gpios();
	sublcd_spi = spi;
	sublcd_spi->mode = SPI_MODE_0;
        sublcd_spi->bits_per_word = 9;
	spi_setup(sublcd_spi);

	reset_seclcd();
	ssd1788_initialize_sec_lcd();
#if 0
	sec_lcd_bklight_enable();
	spi_lcd_fill_colour();
#endif
	/* Initialize the video device */
	init_aux_vout_device();

	return 0;
}

#ifdef CONFIG_PM
static int sublcd_remove(struct spi_device *pdev)
{
	return 0;
}


static int
sublcd_suspend(struct spi_device *dev, pm_message_t state)
{
	/* do we need to turn off LCD power here ? */
	sec_lcd_bklight_disable();

	return 0;
}

static int 
sublcd_resume(struct spi_device *pdev)
{
	struct omap3_aux_disp_device	*vout = aux_disp_dev;

	if (vout->opened)
		sec_lcd_bklight_enable();	
	
	return 0;
}
#endif

static struct spi_driver sub_lcd_driver = {
	.probe		= sublcd_probe,
#ifdef CONFIG_PM
	.remove		= sublcd_remove,
	.suspend	= sublcd_suspend,
	.resume		= sublcd_resume,
#endif
	.driver		= {
		.name	= "sublcd",
		.bus = &spi_bus_type,
		.owner	= THIS_MODULE,
	},
};

static int __init omap34xx_sublcd_init(void)
{
	printk("Sub LCD init \n");

	printk("setting pin mux for 3430\n");
	/*Muxing for 3430*/
	
	//omap_writew(0x0000,PADCONF_McSPI1_CLK);
	omap_writew(0x0000,PADCONF_McSPI1_CS0);		
	omap_writew(0x0000,PADCONF_McSPI1_CS1);		
	omap_writew(0x0000,PADCONF_McSPI1_CS2);		
	omap_writew(0x0000,PADCONF_McSPI1_CS3);		
	omap_writew(0x0000,PADCONF_McSPI1_SIMO);		
	omap_writew(0x0100,PADCONF_McSPI1_SOMI);		

	twl4030_vaux1_ldo_use();
	spi_register_driver(&sub_lcd_driver); 
	
	return 0;
}
  
static void __exit omap34xx_sublcd_exit(void)
{
	spi_unregister_driver(&sub_lcd_driver);
	twl4030_free_gpio(15);
	twl4030_free_gpio(7);
	return;
}
  

late_initcall(omap34xx_sublcd_init);
module_exit(omap34xx_sublcd_exit);

MODULE_AUTHOR("Texas Instruments");
MODULE_DESCRIPTION("McSPI driver Library");
MODULE_LICENSE("GPL");

