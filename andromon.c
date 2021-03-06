#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/kref.h>
#include <asm/uaccess.h>

#include "protocol.h"
#include "andromon.h"
#include "util.h"
#include "fops.h"

/* prevent races between open() and disconnect() */
static DEFINE_MUTEX(disconnect_mutex);

static int andromon_open(struct inode *inode, struct file *file)
{
	struct usb_skel *dev = NULL;
	struct usb_interface *interface; 
	int subminor;
	int retval = 0;

	subminor = iminor(inode);

	mutex_lock(&disconnect_mutex);

	interface = usb_find_interface(&andromon_driver, subminor);
	if (!interface){
		Debug_Print("ANDROMON", "can't find device for minor");
		retval = -ENODEV;
		goto exit;
	}

	dev = usb_get_intfdata(interface);
	if (!dev){
		retval = -ENODEV;
		goto exit;
	}

	if (down_interruptible(&dev->sem)){
		retval = -ERESTARTSYS;
		goto unlock_exit;
	}

	file->private_data = dev;

unlock_exit:
	up(&dev->sem);
	
exit:
	mutex_unlock(&disconnect_mutex);
	return retval;
}

static ssize_t andromon_read(struct file *file, char __user *user_buf, size_t
		count, loff_t *ppos)
{
	struct usb_skel *dev = file->private_data;
	char *buffer;
	int i, retval = 0;

	Debug_Print("ANDROMON", "andromon_read entered");

	/*Debug_Print("ANDROMON", "read: down sem");
	if (down_interruptible(&dev->sem)){
		retval = -ERESTARTSYS;
		goto exit;
	}

	if (!dev->udev){
		retval = -ENODEV;
		Debug_Print("ANDROMON", "No device or device unplugged");
		goto unlock_exit;
	}

	if ((int)count >= dev->data.fp) count = dev->data.fp - 1;

	if (!((int)count > 0))
		goto unlock_exit;

	buffer = kmalloc((int)count, GFP_KERNEL);
	for (i=0; i<dev->data.fp; i++) buffer[i] = dev->data.data[i];

	if (copy_to_user(user_buf, buffer, (int)count)) {
		printk("ANDROMON: read error");
        retval = -EFAULT;
        goto unlock_exit;
    }

	dev->data.fp -= (int)count;
	retval = (int)count;

unlock_exit:
	Debug_Print("ANDROMON", "read: up sem");
	up(&dev->sem);

exit: 
	return retval;*/

	buffer = kmalloc((int)count, GFP_KERNEL);


	/* do a blocking bulk read to get data from the device */
	retval = usb_bulk_msg(dev->udev,
			      usb_rcvbulkpipe(dev->udev, dev->bulk_in_endpointAddr),
			      buffer,
			      count,
			      &count, HZ*10);

	/* if the read was successful, copy the data to userspace */
	if (!retval) {
		if (copy_to_user(user_buf, buffer, count))
			retval = -EFAULT;
		else
			retval = count;
	}

	return retval;
}

static void skel_write_bulk_callback(struct urb *urb, struct pt_regs *regs)
{
	/* sync/async unlink faults aren't errors */
	if (urb->status && 
		!(urb->status == -ENOENT || 
		urb->status == -ECONNRESET ||
		urb->status == -ESHUTDOWN)) {

		printk("%s - nonzero write bulk status received: %d",__FUNCTION__, urb->status);
	}

	/* free up our allocated buffer */
	usb_free_coherent(urb->dev, urb->transfer_buffer_length, 
	urb->transfer_buffer, urb->transfer_dma);
}

static ssize_t andromon_write(struct file *file, const char __user *user_buf, size_t
		count, loff_t *ppos)
{
	struct usb_skel *dev;
	char *buffer = NULL;
	int retval = 0;
	struct urb *urb = NULL;

	dev = file->private_data;

	Debug_Print("ANDROMON", "write: down sem");
	if (down_interruptible(&dev->sem)){
		retval = -ERESTARTSYS;
		goto exit;
	}

	// verify that the device wasn't unplugged
	if (!dev->udev){
		retval = -ENODEV;
		Debug_Print("ANDOROMON", "No device or device unplugged");
		goto unlock_exit;
	}

	if ((int)count <= 0)
		goto unlock_exit;

	urb = usb_alloc_urb(0, GFP_KERNEL);

//	buffer = kmalloc(sizeof(char) * (int)count, GFP_KERNEL);

	buffer = usb_alloc_coherent(dev->udev, count, GFP_KERNEL, &urb->transfer_dma);
	if (!buffer){
		retval = -ENOMEM;
		goto unlock_exit;
	}
	//memset(buffer, 0, sizeof(buffer));

	if (copy_from_user(buffer, user_buf, (int)count)){
		Debug_Print("ANDROMON", "Couldn't read from user space");
		retval = -EFAULT;
		goto unlock_exit;
	}

	/*if (copy_from_user(dev->data.data, user_buf, (int)count)){
		Debug_Print("ANDROMON", "Couldn't read from user space");
		retval = -EFAULT;
		goto unlock_exit;
	}*/
printk("pipe  = %d\n", usb_sndbulkpipe(dev->udev, dev->bulk_out_endpointAddr));
	usb_fill_bulk_urb(urb, 
					  dev->udev,
					  usb_sndbulkpipe(dev->udev, dev->bulk_out_endpointAddr),
					  buffer,
					  count,
					  skel_write_bulk_callback,
					  dev);
	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
	
	retval = usb_submit_urb(urb, GFP_KERNEL);
	if (retval){
		printk("urb write failed\n");
		goto unlock_exit;
	}

	printk("Count = %d\n", (int)count);

	dev->data.fp += (int)count;
	retval = (int)count;

unlock_exit:
	Debug_Print("ANDROMON", "write: up sem");
	up(&dev->sem);

exit: 
	return retval;
}

static int andromon_release(struct inode *inode, struct file *file)
{
	struct usb_skel *dev = NULL;
	int retval = 0;

	printk("release\n");

	dev = file->private_data;
	if (!dev){
		Debug_Print("ANDROMON", "dev is NULL");
		retval = -ENODEV;
		goto exit;
	}

//	kfree(dev);

exit:
	return retval;
}

static struct file_operations andromon_fops = {
	.owner =	THIS_MODULE,
	.read =		andromon_read,
	.write =	andromon_write,
	.open =		andromon_open,
	.release =	andromon_release,
};

static struct usb_class_driver andromon_class = {
	.name = "andromon%d",
	.fops = &andromon_fops,
	.minor_base = ANDROMON_MINOR_BASE,
};

int p = 0;
// probe function
// called on device insertion iff no other dricer has beat us to the punch
static int andromon_probe(struct usb_interface *interface, const struct usb_device_id *id){
	struct usb_skel *andromon_usb = NULL;
	struct usb_host_interface *interface_descriptor;
	struct usb_endpoint_descriptor *endpoint;
	unsigned int i/*, sendPipe,*/;
	u8 data[8];
	int datalen = 0;
	int retval = 0;

	printk(KERN_INFO "[*] ANDROMON (%04X:%04X) plugged\n", id->idVendor, id->idProduct);

	// allocate memory for device state and initialize it
	andromon_usb = kmalloc(sizeof(struct usb_skel), GFP_KERNEL);
	if (andromon_usb == NULL){
		printk(KERN_INFO "Out of memory");	
	}
	memset(andromon_usb, 0x00, sizeof(*andromon_usb));
	kref_init(&andromon_usb->kref);

	sema_init(&andromon_usb->sem, 1);

	//andromon_usb->data.data[0] = 'a';
	//andromon_usb->data.data[1] = 'b';
	//andromon_usb->data.data[2] = 'c';
	//andromon_usb->data.fp = 3;

	andromon_usb->udev = usb_get_dev(interface_to_usbdev(interface));
	andromon_usb->interface = interface;

	if(id->idVendor == 0x18D1 && (id->idProduct == 0x2D00 || id->idProduct == 0x2D01)){
		printk("ANDROMON:  [%04X:%04X] Connected in AOA mode\n", id->idVendor, id->idProduct);

		printk("ANDROMON: usb_interface->cur_altsetting->string = %s %s\n", interface->cur_altsetting->string, interface->cur_altsetting->extra);

		interface_descriptor = interface->cur_altsetting;
		for(i=0; i<interface_descriptor->desc.bNumEndpoints; i++){
			endpoint = &interface_descriptor->endpoint[i].desc;

			if (!andromon_usb->bulk_out_endpointAddr &&
				((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT) && 
				(endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK
			   ){
					andromon_usb->bulk_out_endpointAddr = endpoint->bEndpointAddress;
					Debug_Print("ANDROMON", "Bulk out endpoint");
				}

			if (!andromon_usb->bulk_in_endpointAddr &&
				((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN) && 
				(endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK
			   ){
					andromon_usb->bulk_in_endpointAddr = endpoint->bEndpointAddress;
					Debug_Print("ANDROMON", "Bulk in endpoint");
				}
		}

		/* save our data pointer in this interface device */
		usb_set_intfdata(andromon_usb->interface, andromon_usb);
		retval = usb_register_dev(andromon_usb->interface, &andromon_class);
		if (retval) {
			Debug_Print("ANDROMON", "Not able to get a minor for this device.");
			usb_set_intfdata(andromon_usb->interface, NULL);
			//goto error;
		}
		andromon_usb->minor = andromon_usb->interface->minor;

		//SetConfiguration(andromon_usb, (char*)data);
	}
	else{
		datalen = GetProtocol(andromon_usb, (char*)data);

		if (datalen < 0) {
			printk("ANDROMON: usb_control_msg : [%d]", datalen);
		}
		else{
			printk("ANDROMON: AOA version = %d\n", data[0]);
		}

		SendIdentificationInfo(andromon_usb, ID_MANU, (char*)MANU);
		SendIdentificationInfo(andromon_usb, ID_MODEL, (char*)MODEL);
		SendIdentificationInfo(andromon_usb, ID_DESC, (char*)DESCRIPTION);
		SendIdentificationInfo(andromon_usb, ID_VERSION, (char*)VERSION);
		SendIdentificationInfo(andromon_usb, ID_URI, (char*)URI);
		SendIdentificationInfo(andromon_usb, ID_SERIAL, (char*)SERIAL);

		SendAOAActivationRequest(andromon_usb);
	}

	return 0;	// return 0 indicates we'll manage this device
}

// disconnect
static void andromon_disconnect(struct usb_interface *interface){
	struct usb_skel *andromon_usb;
	int minor = interface->minor;

	//lock_kernel();
	
	andromon_usb = usb_get_intfdata(interface);
	usb_set_intfdata(interface, NULL);

	usb_deregister_dev(interface, &andromon_class);

	//unlock_kernel();
	//kref_put(&andromon_usb->kref, skel_delete);
	

	printk(KERN_INFO "[*] ANDROMON removed. Minor: %d\n", minor);
}

//usb device id
static struct usb_device_id andromon_table[] = {
	{ USB_DEVICE(0x058f, 0x6378) },	// information is obtained using "lsusb" at the command line
	{ USB_DEVICE(0x1005, 0xb113) },
	{ USB_DEVICE(0x18d1, 0x4e42) },	// Nexus 7
	{ USB_DEVICE(0x18d1, 0xd002) },	// Nexus 7
	{ USB_DEVICE(0x18d1, 0x2d00) },	// Nexus 7 AOA
	{ USB_DEVICE_AND_INTERFACE_INFO(0x18d1, 0x2d01, 255, 255, 0) },	// Nexus 7 AOA
	{ USB_DEVICE(0x0bb4, 0x0cb0) },	// HTC wildfire
	{ USB_DEVICE(0x276d, 0x1105) },	// USB mouse
	{}	/* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, andromon_table);

//usb_driver
static struct usb_driver andromon_driver = 
{
	.name = "ANDROMON USB DRIVER",
	.id_table = andromon_table,	// usb device id
	.probe = andromon_probe,
	.disconnect = andromon_disconnect,
};

static int __init andromon_init(void){
	int ret = -1;
	
	Debug_Print("ANDROMON", "ANDROMON Registering with kernel");

	ret = usb_register(&andromon_driver);
	if (ret)
        Debug_Print("ANDROMON", "usb_register failed");//. Error number %d", ret);

	return ret;
}

static void __exit andromon_exit(void){
	// deregister
	printk(KERN_INFO "[*] Fahim Destructor of driver");
	usb_deregister(&andromon_driver);
	printk(KERN_INFO "\tunregistration complete");
}

module_init(andromon_init);
module_exit(andromon_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AZIZUL HAKIM");
