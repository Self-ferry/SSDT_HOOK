#include"Hook.h"


VOID DriverUnload(PDRIVER_OBJECT pDriver)
{
	ThreadFlag = FALSE;
	//ȷ���߳�ֹͣ
	LARGE_INTEGER timer = { 0 };
	timer.QuadPart = -100 * 1000 * 1000;
	KeDelayExecutionThread(KernelMode, TRUE, &timer);

	if (MySSDT != NULL)
	{
		ExFreePoolWithTag(MySSDT->ServiceTable.FuncTable, 'MySF');
		MySSDT->ServiceTable.FuncTable = NULL;
		ExFreePoolWithTag(MySSDT, 'MySD');
		MySSDT = NULL;
	}

	DbgPrint("������������ж����\t\n");
	return;
}

NTSTATUS GetCopySSDT()
{
	
	//DbgPrint("KeServiceDescriptorTable:%p\t\n", KeServiceDescriptorTable);
	PSSDT SSDTShadow = (PSSDT)(*(PULONG)(&KeServiceDescriptorTable) - 0x40);
	//DbgPrint("SSDTShadow:%p\t\n", SSDTShadow);
	//__asm int 3;
	// 
	// ����һ��SSDT

	// ����һ��SSDT��С���ڴ�
	MySSDT = (PSSDT)ExAllocatePoolWithTag(NonPagedPool, sizeof(SSDT), 'MySD');
	if (MySSDT == NULL)
	{
		DbgPrint("MySD ExAllocatePoolWithTag Failed!\t\n");
		return STATUS_UNSUCCESSFUL;
	}
	memset(MySSDT, 0, sizeof(SSDT));
	DbgPrint("MySSDT:%p\t\n", MySSDT);
	//���뺯������ڴ�
	MySSDT->ServiceTable.FuncTable = (PULONG)ExAllocatePoolWithTag(
		NonPagedPool,
		KeServiceDescriptorTable->ServiceTable.ServiceLimit * 4,
		'MySF'
	);
	if (MySSDT->ServiceTable.FuncTable == NULL)
	{
		ExFreePoolWithTag(MySSDT, 'MySD');
		MySSDT = NULL;
		DbgPrint("MySF ExAllocatePoolWithTag Failed!\t\n");
		return STATUS_UNSUCCESSFUL;
	}
	memset((PVOID)MySSDT->ServiceTable.FuncTable, 0, KeServiceDescriptorTable->ServiceTable.ServiceLimit * 4);

	//����SSDT

	memcpy((PVOID)MySSDT->ServiceTable.FuncTable, (PVOID)KeServiceDescriptorTable->ServiceTable.FuncTable,
		KeServiceDescriptorTable->ServiceTable.ServiceLimit * 4);
	MySSDT->ServiceTable.ServiceLimit = KeServiceDescriptorTable->ServiceTable.ServiceLimit;
	MySSDT->ServiceTable.ArgumentTable = KeServiceDescriptorTable->ServiceTable.ArgumentTable;

	//����Shadow��
	//�ж�GUI�������Ƿ��Ѿ����أ��������GUI�̣߳������ϵͳ�������GUI������

	//˼·�ж�Shadwo �����������ҳ����
	
	//if (Ӱ�ӱ��еĺ�����������)
	//{
	//	//����Ӱ�ӱ��к�������ڴ�
	//	MySSDT->ServiceTableShadow.FuncTable = (PULONG)ExAllocatePoolWithTag(
	//		NonPagedPool,
	//		SSDTShadow->ServiceTableShadow.ServiceLimit * 4,
	//		'MySH'
	//	);
	//	if (MySSDT->ServiceTableShadow.FuncTable == NULL)
	//	{
	//		ExFreePoolWithTag(MySSDT->ServiceTable.FuncTable, 'MySF');
	//		ExFreePoolWithTag(MySSDT, 'MySD');
	//		DbgPrint("MySH ExAllocatePoolWithTag Failed!\t\n");
	//		return STATUS_UNSUCCESSFUL;
	//	}
	//	memset((PVOID)MySSDT->ServiceTableShadow.FuncTable, 0, SSDTShadow->ServiceTableShadow.ServiceLimit * 4);
	//	memcpy((PVOID)MySSDT->ServiceTableShadow.FuncTable, (PVOID)SSDTShadow->ServiceTableShadow.FuncTable,
	//		SSDTShadow->ServiceTableShadow.ServiceLimit * 4);

	MySSDT->ServiceTableShadow.FuncTable = SSDTShadow->ServiceTableShadow.FuncTable;

	MySSDT->ServiceTableShadow.ServiceLimit = SSDTShadow->ServiceTableShadow.ServiceLimit;
	MySSDT->ServiceTableShadow.ArgumentTable = SSDTShadow->ServiceTableShadow.ArgumentTable;

return STATUS_SUCCESS;
}


// �������� �ҵ�����
ULONG FindProcess(char* ProcessName)
{

	ULONG Pro;
	//��KPCR+0x124��λ�õõ���ǰCPU���ڴ�����̵߳Ľṹ���ָ�룬_KTHREAD + 0x44 ��λ���ܹ��õ����̵߳Ľ��̽ṹ��ĵ�ַ
	__asm {
		mov eax, fs: [0x124] ;
		mov ecx, [eax + 0x44];
		mov Pro, ecx;
	}
	//���̽ṹ��EPROCESS +0x88��λ����һ���������н��̵�˫������
	PLIST_ENTRY pListProcess = (PLIST_ENTRY)(Pro + 0x88);
	BOOLEAN Flag = FALSE;

	//��������
	while (pListProcess->Flink != (PLIST_ENTRY)(Pro + 0x88))
	{
		//EPROCESS �ṹ��
		ULONG NextProcess = ((ULONG)(pListProcess)) - 0x88;
		
		if (strcmp(ProcessName, (PCHAR)(NextProcess + 0x174)) == 0)
		{
			DbgPrint("FindProcess:%s PEPROCESS:%X\t\n", ((PCHAR)NextProcess + 0x174), NextProcess);
			return NextProcess;
		}
		pListProcess = pListProcess->Flink;
	}
	return 0;
}

VOID TraversalThreadReplaceE0(ULONG eProc)
{
	if (eProc == NULL)
	{
		DbgPrint("eProcess is NULL\t\n");
		return;
	}
	//�ɽ��̽ṹ������߳�
	//��ȡ�̵߳�˫������
	//+0x190 ThreadListHead   : _LIST_ENTRY [ 0x89ff024c - 0x89dbd56c ]
	PLIST_ENTRY ThreadList = (PLIST_ENTRY)(eProc + 0x190);
	//���˰������ ԭ��������û�л�ȡ �̣�  PLIST_ENTRY CurrentThreadList = ThreadList
	PLIST_ENTRY CurrentThreadList = ThreadList->Flink;
	//�����߳�

	do
	{
		//��ȡ��ǰ�߳̽ṹ����ײ�
		ULONG CurrentThread = (ULONG)CurrentThreadList - 0x22c;
		//ȡ���߳��д洢��SSDT��ֵ
		PULONG ServiceTable = (PULONG)(CurrentThread + 0xE0);
		DbgPrint("ServiceTable:%X\t\n", *ServiceTable);
		if (ServiceTable == (PULONG)MySSDT)
		{
			continue;
		}
		*ServiceTable = (PULONG)MySSDT;
		CurrentThreadList = CurrentThreadList->Flink;
	} while (CurrentThreadList->Flink != ThreadList->Flink);
}

VOID ResetThreadReplaceE0(ULONG eProc) {
	if (eProc == NULL)
	{
		DbgPrint("eProcess is NULL\t\n");
		return;
	}
	PLIST_ENTRY ThreadList = (PLIST_ENTRY)(eProc + 0x190);
	PLIST_ENTRY CurrentThreadList = ThreadList->Flink;
	//�����߳�
	do
	{
		//��ȡ��ǰ�߳̽ṹ����ײ�
		ULONG CurrentThread = (ULONG)CurrentThreadList - 0x22c;
		//ȡ���߳��д洢��SSDT��ֵ
		PULONG ServiceTable = (PULONG)(CurrentThread + 0xE0);
		ULONG ShadowTable = *(PULONG)(&KeServiceDescriptorTable);
		if (ServiceTable == (PULONG)(ShadowTable - 0x40))
		{
			continue;
		}
		*ServiceTable = (PULONG)ShadowTable;
		DbgPrint("ServiceTable:%X\t\n", *ServiceTable);
		CurrentThreadList = CurrentThreadList->Flink;
	} while (CurrentThreadList->Flink != ThreadList->Flink);
}

VOID WorkThreadFunc(PVOID StartContext)
{
	
	extern BOOLEAN ThreadFlag;
	ULONG pProcess;
	LARGE_INTEGER timer = { 0 };
	timer.QuadPart = -10 * 1000 * 1000;
	DbgPrint("WorkThreadFunc ��ʼ����!\t\n");

	while (ThreadFlag)
	{
		KeDelayExecutionThread(KernelMode, FALSE, &timer);
		pProcess = FindProcess("Dbgview.exe"); //һֱ���Ҵ˽��̣���Ϊ�˽��̿��ܻᱻ�ر�
		TraversalThreadReplaceE0(pProcess);
	}
	ThreadFlag = TRUE;
	ResetThreadReplaceE0(pProcess);
	UnHookNtOpenProcess();
	DbgPrint("WorkThreadFunc ��������!\t\n");
	ZwClose(hThread);
}


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pRegPath)
{
	pDriver->DriverUnload = DriverUnload;

	DbgPrint("������������������!\t\n");
	// ����һ��SSDT��

	if (!NT_SUCCESS(GetCopySSDT()))
	{
		DbgPrint("GetCopySSDT Failed!\t\n");
		return STATUS_UNSUCCESSFUL;
	}
	
	//��ӡһ��MySSDT
	DbgPrint("MySSDT:%p\t\n", MySSDT);
	//�����ϵ�NtOpenProcess��ַ
	uOldNtOpenProcess = MySSDT->ServiceTable.FuncTable[0x7A];
	DbgPrint("uOldNtOpenProcess:%p\t\n", uOldNtOpenProcess);
	//��ʼHook
	HookNtOpenProcess();
	// �������� �ҵ�����
	//�����߳� �滻E0λ��
	//���ö�ʱ����ÿ��10�����޸�һ��
	//����ʱ�ָ�E0������HOOK���رվ��
	extern HANDLE hThread;
	PsCreateSystemThread(&hThread, 0, NULL, NULL, NULL, WorkThreadFunc, NULL); 
	
	return STATUS_SUCCESS;
}