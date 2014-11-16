/*

check protocol details: http://source.android.com/accessories/aoa.html#establish-communication-with-the-device

*/

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "andromon_vars.h"
#include "util.h"


#define REQ_GET_PROTOCOL	51
#define REQ_SEND_ID			52
#define REQ_AOA_ACTIVATE	53

#define MANU	"IsonProjects"
#define MODEL	"ANDROMON"
#define DESCRIPTION	"ANDROID MONITOR DRIVER"
#define VERSION	"1.0"
#define URI		"www.google.com"
#define SERIAL	"10"

#define ID_MANU		0
#define ID_MODEL	1
#define ID_DESC		2
#define ID_VERSION	3
#define ID_URI		4
#define ID_SERIAL	5

#define VAL_AOA_REQ		0


int GetProtocol(struct usb_skel *andromon_usb, char *buffer){
	Debug_Print("ANDROMON", "Get Protocol: AOA version");
	
	return usb_control_msg(andromon_usb->udev,
			usb_rcvctrlpipe(andromon_usb->udev, 0),
			REQ_GET_PROTOCOL,
			USB_DIR_IN | USB_TYPE_VENDOR,
			VAL_AOA_REQ,
			0,
			buffer,
			sizeof(buffer),
			HZ*5);	
}

int SendIdentificationInfo(struct usb_skel *andromon_usb, int id_index, char *id_info){
	int ret = usb_control_msg(andromon_usb->udev,
				usb_sndctrlpipe(andromon_usb->udev, 0),
				REQ_SEND_ID,
				USB_DIR_OUT | USB_TYPE_VENDOR,
				VAL_AOA_REQ,
				id_index,
				utf8((char*)(id_info)),
				sizeof(id_info) + 1,
				HZ*5);

	return ret;
}

int SendAOAActivationRequest(struct usb_skel *andromon_usb){
	Debug_Print("ANDROMON", "REQ FOR AOA Mode Activation");

	return usb_control_msg(andromon_usb->udev,
							usb_sndctrlpipe(andromon_usb->udev, 0),
							REQ_AOA_ACTIVATE,
							USB_DIR_OUT | USB_TYPE_VENDOR,
							VAL_AOA_REQ,
							0,
							NULL,
							0,
							HZ*5);
}

#endif
