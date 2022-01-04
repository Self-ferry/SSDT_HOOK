#include "Hook.h"

VOID PageProtectOff()
{
	__asm
	{
		mov eax, cr0;
		or eax, 0x10000; // WP位置1
		mov cr0, eax;
		sti; // 恢复中断
	}
}

VOID PageProtectOn()
{
	__asm
	{
		cli; // 关闭中断
		mov eax, cr0;
		and eax, not 0x10000; // WP位置0
		mov cr0, eax;
	}
}

VOID HookNtOpenProcess()
{
	DbgPrint("The start of the hook!\t\n");
	PageProtectOff();
	MySSDT->ServiceTable.FuncTable[0x7A] = (ULONG)ModifyNtOpenProcess;
	PageProtectOn();
}

VOID UnHookNtOpenProcess()
{
	PageProtectOff();
	MySSDT->ServiceTable.FuncTable[0x7A] = uOldNtOpenProcess;
	PageProtectOn();
	DbgPrint("The end of the hook!\t\n");
}


NTSTATUS NTAPI ModifyNtOpenProcess(
	PHANDLE ProcessHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PCLIENT_ID ClientId
)
{
	DbgPrint("打印NtOpenProcess的参数：%X %X %X %X\t\n", ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
	return ((PNTOPENPROCESS)uOldNtOpenProcess)(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
}
