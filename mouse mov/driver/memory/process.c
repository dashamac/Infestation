#include "process.h"

NTSTATUS read_virtual_memory(int pid, PVOID source_addr, PVOID target_addr, SIZE_T size) {
	SIZE_T bytes;
	NTSTATUS status = STATUS_SUCCESS;
	PEPROCESS process;

	if (!NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)pid, &process)))
		return STATUS_INVALID_PARAMETER;

	__try {
		status = MmCopyVirtualMemory(process, source_addr, PsGetCurrentProcess(), target_addr, size, KernelMode, &bytes);
	}
	__finally {
		ObDereferenceObject(process);
	}

	if (!NT_SUCCESS(status))
		return status;

	return status;
}

NTSTATUS write_virtual_memory(int pid, PVOID dest_addr, PVOID source_buffer, SIZE_T size) {
	SIZE_T bytes;
	NTSTATUS status = STATUS_SUCCESS;
	PEPROCESS process;

	if (!NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)pid, &process)))
		return STATUS_INVALID_PARAMETER;

	__try {
		status = MmCopyVirtualMemory(PsGetCurrentProcess(), source_buffer, process, dest_addr, size, KernelMode, &bytes);
	}
	__finally {
		ObDereferenceObject(process);
	}

	if (!NT_SUCCESS(status))
		return status;

	return status;
}

NTSTATUS write_safe_memory(int pid, PVOID dest_addr, PVOID source_buffer, SIZE_T size) {
	NTSTATUS status = STATUS_SUCCESS;
	PEPROCESS process;
	PMDL mdl = NULL;
	PVOID mapped_buffer = NULL;

	if (!NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)pid, &process)))
		return STATUS_INVALID_PARAMETER;

	mdl = IoAllocateMdl(dest_addr, size, FALSE, FALSE, NULL);
	if (!mdl) {
		ObDereferenceObject(process);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	__try {
		MmProbeAndLockPages(mdl, KernelMode, IoReadAccess);
		mapped_buffer = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
		if (!mapped_buffer) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		RtlCopyMemory(mapped_buffer, source_buffer, size);
		MmUnmapLockedPages(mapped_buffer, mdl);
		MmUnlockPages(mdl);
	}
	__finally {
		if (mdl)
			IoFreeMdl(mdl);
		ObDereferenceObject(process);
	}

	return status;
}