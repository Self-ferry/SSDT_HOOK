#include "Hook.h"

VOID PageProtectOff()
{
	__asm
	{
		mov eax, cr0;
		or eax, 0x10000; // WPλ��1
		mov cr0, eax;
		sti; // �ָ��ж�
	}
}

VOID PageProtectOn()
{
	__asm
	{
		cli; // �ر��ж�
		mov eax, cr0;
		and eax, not 0x10000; // WPλ��0
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
	DbgPrint("��ӡNtOpenProcess�Ĳ�����%X %X %X %X\t\n", ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
	return ((PNTOPENPROCESS)uOldNtOpenProcess)(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
}
