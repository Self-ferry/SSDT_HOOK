#pragma once
#include<ntddk.h>

//SST ϵͳ�����
typedef struct _KSYSTEM_SERVICE_TABLE
{
	PULONG FuncTable;
	PULONG Count;
	ULONG ServiceLimit;
	PUCHAR ArgumentTable;
}KSST, * PKSST;


//SSDT 
typedef struct _SSDT
{
	KSST ServiceTable;
	KSST ServiceTableShadow;
	KSST un1;
	KSST un2;
}SSDT, * PSSDT;



//����SSDT
PSSDT MySSDT;
static BOOLEAN ThreadFlag = TRUE;
HANDLE hThread;
//��ȡSSDT�ĵ�������
extern PSSDT KeServiceDescriptorTable;


//��������
VOID DriverUnload(PDRIVER_OBJECT pDriver);
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pRegPath);
NTSTATUS GetCopySSDT();
ULONG FindProcess(char* ProcessName);
VOID TraversalThreadReplaceE0(ULONG eProcess);
VOID ResetThreadReplaceE0(ULONG eProc);
VOID WorkThreadFunc(_In_ PVOID StartContext);


