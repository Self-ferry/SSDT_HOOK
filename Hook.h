#pragma once
#include "ssdt.h"

//被HOOK函数的函数指针

//HOOK NtOpenProcess 函数
typedef NTSTATUS(NTAPI *PNTOPENPROCESS)(
     PHANDLE ProcessHandle,
     ACCESS_MASK DesiredAccess,
     POBJECT_ATTRIBUTES ObjectAttributes,
     PCLIENT_ID ClientId
);


//函数声明

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

//原来的函数地址
ULONG uOldNtOpenProcess;