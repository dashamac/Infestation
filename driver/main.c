#include "communication/dispatch.h"
#include "utils/message.h"
#include "memory/memory.h"
#include "memory/process.h"

UNICODE_STRING device_name = RTL_CONSTANT_STRING(L"\\Device\\infestation");
UNICODE_STRING device_link = RTL_CONSTANT_STRING(L"\\DosDevices\\infestation");
PDEVICE_OBJECT device_object;

NTSTATUS driver_unload(PDRIVER_OBJECT driver_object) {
	IoDeleteDevice(device_object);
	IoDeleteSymbolicLink(&device_link);
	message("Goodbye, world!\n");
	return STATUS_SUCCESS;
}

NTSTATUS driver_entry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path) {
	UNREFERENCED_PARAMETER(registry_path);

	message("Hello, world!\n");
	driver_object->DriverUnload = driver_unload;

	message("mouhid.sys %p\n", get_kernel_module("mouhid.sys"));

	IoCreateDevice(driver_object, 0, &device_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &device_object);
	IoCreateSymbolicLink(&device_link, &device_name);

	driver_object->MajorFunction[IRP_MJ_CREATE] = on_create;
	driver_object->MajorFunction[IRP_MJ_CLOSE] = on_close;
	driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = on_message;

	device_object->Flags |= DO_DIRECT_IO;
	device_object->Flags &= ~DO_DEVICE_INITIALIZING;

	int pid = get_process_id("cs2.exe");
	//UNICODE_STRING module_name;
	//RtlInitUnicodeString(&module_name, L"client.dll");
	//uintptr_t client = get_module_base(pid, module_name);
	//int local_health = 0;
	//uintptr_t local_player = 0;
	//read_virtual_memory(pid, (PVOID)(client + 0x16C2B18), &local_player, sizeof(uintptr_t));
	//read_virtual_memory(pid, (PVOID)(local_player + 0x32C), &local_health, sizeof(int));
	//message("local_player %p , health %d", local_player, local_health);

	return STATUS_SUCCESS;
}
