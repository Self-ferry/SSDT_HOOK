#pragma once
#include "ssdt.h"

//��HOOK�����ĺ���ָ��

//HOOK NtOpenProcess ����
typedef NTSTATUS(NTAPI *PNTOPENPROCESS)(
     PHANDLE ProcessHandle,
     ACCESS_MASK DesiredAccess,
     POBJECT_ATTRIBUTES ObjectAttributes,
     PCLIENT_ID ClientId
);


//��������

VOID HookNtOpenProcess();
VOID UnHookNtOpenProcess();
VOID PageProtectOn();
VOID PageProtectOff();
NTSTATUS NTAPI ModifyNtOpenProcess(
     PHANDLE ProcessHandle,
     ACCESS_MASK DesiredAccess,
     POBJECT_ATTRIBUTES ObjectAttributes,
     PCLIENT_ID ClientId
);

//ԭ���ĺ�����ַ
ULONG uOldNtOpenProcess;