#include "dispatch.h"

static LARGE_INTEGER last_request_time = {0};
static ULONG request_count = 0;
static KSPIN_LOCK rate_limit_lock;

BOOLEAN check_rate_limit() {
	LARGE_INTEGER current_time;
	KIRQL irql;

	KeQuerySystemTime(&current_time);
	LARGE_INTEGER one_second;
	one_second.QuadPart = 10000000;

	KeAcquireSpinLock(&rate_limit_lock, &irql);

	if ((current_time.QuadPart - last_request_time.QuadPart) >= one_second.QuadPart) {
		request_count = 0;
		last_request_time = current_time;
	}

	if (request_count >= 1000) {
		KeReleaseSpinLock(&rate_limit_lock, irql);
		return FALSE;
	}

	request_count++;
	KeReleaseSpinLock(&rate_limit_lock, irql);
	return TRUE;
}

NTSTATUS on_message(PDEVICE_OBJECT device_object, PIRP irp) {
	UNREFERENCED_PARAMETER(device_object);

	PIO_STACK_LOCATION io_stack = IoGetCurrentIrpStackLocation(irp);
	ULONG control_code = io_stack->Parameters.DeviceIoControl.IoControlCode;

	if (!irp->AssociatedIrp.SystemBuffer) {
		irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_INVALID_PARAMETER;
	}

	if (!check_rate_limit()) {
		irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	if (control_code == MOUSE_REQUEST) {
		ULONG input_length = io_stack->Parameters.DeviceIoControl.InputBufferLength;
		if (input_length < sizeof(KMOUSE_REQUEST)) {
			irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
			irp->IoStatus.Information = 0;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return STATUS_BUFFER_TOO_SMALL;
		}

		message("Mouse request\n");
		PKMOUSE_REQUEST mouse_request = (PKMOUSE_REQUEST)irp->AssociatedIrp.SystemBuffer;

		mouse_move(mouse_request->x, mouse_request->y, mouse_request->button_flags);

		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return STATUS_SUCCESS;
	}
	else if (control_code == PROCESSID_REQUEST) {
		ULONG input_length = io_stack->Parameters.DeviceIoControl.InputBufferLength;
		if (input_length < sizeof(KPROCESSID_REQUEST)) {
			irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
			irp->IoStatus.Information = 0;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return STATUS_BUFFER_TOO_SMALL;
		}

		message("Process id request\n");
		PKPROCESSID_REQUEST process_request = (PKPROCESSID_REQUEST)irp->AssociatedIrp.SystemBuffer;
		message("Process name %s\n", process_request->process_name);
		int process_id = get_process_id(process_request->process_name);
		message("Process id %d\n", process_id);
		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = process_id;
		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return STATUS_SUCCESS;
	}
	else if (control_code == MODULEBASE_REQUEST) {
		ULONG input_length = io_stack->Parameters.DeviceIoControl.InputBufferLength;
		if (input_length < sizeof(KERNEL_MODULE_REQUEST)) {
			irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
			irp->IoStatus.Information = 0;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return STATUS_BUFFER_TOO_SMALL;
		}

		message("Module base request\n");
		PKERNEL_MODULE_REQUEST module_request = (PKERNEL_MODULE_REQUEST)irp->AssociatedIrp.SystemBuffer;

		uintptr_t module_base = get_module_base(module_request->pid, module_request->module_name);

		message("Module base %p\n", module_base);

		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = module_base;

		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return STATUS_SUCCESS;
	}
	else if (control_code == READ_REQUEST) {
		ULONG input_length = io_stack->Parameters.DeviceIoControl.InputBufferLength;
		if (input_length < sizeof(KERNEL_READ_REQUEST)) {
			irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
			irp->IoStatus.Information = 0;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return STATUS_BUFFER_TOO_SMALL;
		}

		message("Read request\n");
		PKERNEL_READ_REQUEST read_request = (PKERNEL_READ_REQUEST)irp->AssociatedIrp.SystemBuffer;

		message("Src pid %d\n", read_request->src_pid);
		message("Src address %p\n", read_request->src_address);
		message("Dst buffer %p\n", read_request->p_buffer);
		message("Size %d\n", read_request->size);

		NTSTATUS status = read_virtual_memory(read_request->src_pid, read_request->src_address, read_request->p_buffer, read_request->size);

		if (!NT_SUCCESS(status)) {
			irp->IoStatus.Status = status;
			irp->IoStatus.Information = 0;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return status;
		}

		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = read_request->size;
		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return STATUS_SUCCESS;
	}
	else {
		message("Unknown request\n");
		irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_ABANDONED;
	}
}

NTSTATUS unsupported_opperation(PDEVICE_OBJECT device_object, PIRP irp) {
	UNREFERENCED_PARAMETER(device_object);
	UNREFERENCED_PARAMETER(irp);

	message("TODO\n");
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS on_create(PDEVICE_OBJECT device_object, PIRP irp) {
	UNREFERENCED_PARAMETER(device_object);

	message("Creation called\n");
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS on_close(PDEVICE_OBJECT device_object, PIRP irp) {
	UNREFERENCED_PARAMETER(device_object);

	message("Close called\n");
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}