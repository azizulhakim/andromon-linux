#ifndef ANDROMON_VARS_H
#define ANDROMON_VARS_H

#include <linux/mutex.h>

struct driver_data{
	char data[1000];
	int  fp;
};

struct andromon_qset{
	void **data;
	struct andromon_qset *next;
};


/* Structure to hold all of our device specific stuff */
struct usb_skel {
	struct usb_device			*udev;						/* the usb device for this device */
	struct driver_data			data;

	struct andromon_qset		*data1;  					/* Pointer to first quantum set */
    int							quantum;					/* the current quantum size */
    int 						qset;						/* the current array size */
    unsigned long 				size;       				/* amount of data stored here */
    unsigned int 				access_key;					/* used by sculluid and scullpriv */
	struct semaphore			sem;						/* Mutual exclusion semaphore */

	struct usb_interface		*interface;					/* the interface for this device */
	int minor;
	unsigned char				*bulk_in_buffer;			/* the buffer to receive data */
	size_t						bulk_in_size;				/* the size of the receive buffer */
	__u8						bulk_in_endpointAddr;		/* the address of the bulk in endpoint */
	__u8						bulk_out_endpointAddr;		/* the address of the bulk out endpoint */
	struct kref					kref;
};
#endif
