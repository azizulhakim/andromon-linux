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

static struct file_operations andromon_fops = {
	.owner =	THIS_MODULE,
	.write =	NULL,//andromon_write,
	.open =		NULL,//andromon_open,
	.release =	NULL,//andromon_release,
};

static struct usb_class_driver andromon_class = {
	.name = "andromon%d",
	.fops = &andromon_fops,
	.minor_base = ANDROMON_MINOR_BASE,
};

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

	andromon_usb->udev = usb_get_dev(interface_to_usbdev(interface));
	andromon_usb->interface = interface;

	if(id->idVendor == 0x18D1 && (id->idProduct == 0x2D00 || id->idProduct == 0x2D01)){
		printk("ANDROMON:  [%04X:%04X] Connected in AOA mode\n", id->idVendor, id->idProduct);

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
	{ USB_DEVICE(0x18d1, 0x2d01) },	// Nexus 7 AOA
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
