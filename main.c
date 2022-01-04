#include"Hook.h"


VOID DriverUnload(PDRIVER_OBJECT pDriver)
{
	ThreadFlag = FALSE;
	//确保线程停止
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

	DbgPrint("我是驱动，我卸载了\t\n");
	return;
}

NTSTATUS GetCopySSDT()
{
	
	//DbgPrint("KeServiceDescriptorTable:%p\t\n", KeServiceDescriptorTable);
	PSSDT SSDTShadow = (PSSDT)(*(PULONG)(&KeServiceDescriptorTable) - 0x40);
	//DbgPrint("SSDTShadow:%p\t\n", SSDTShadow);
	//__asm int 3;
	// 
	// 构造一份SSDT

	// 申请一份SSDT大小的内存
	MySSDT = (PSSDT)ExAllocatePoolWithTag(NonPagedPool, sizeof(SSDT), 'MySD');
	if (MySSDT == NULL)
	{
		DbgPrint("MySD ExAllocatePoolWithTag Failed!\t\n");
		return STATUS_UNSUCCESSFUL;
	}
	memset(MySSDT, 0, sizeof(SSDT));
	DbgPrint("MySSDT:%p\t\n", MySSDT);
	//申请函数表的内存
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

	//拷贝SSDT

	memcpy((PVOID)MySSDT->ServiceTable.FuncTable, (PVOID)KeServiceDescriptorTable->ServiceTable.FuncTable,
		KeServiceDescriptorTable->ServiceTable.ServiceLimit * 4);
	MySSDT->ServiceTable.ServiceLimit = KeServiceDescriptorTable->ServiceTable.ServiceLimit;
	MySSDT->ServiceTable.ArgumentTable = KeServiceDescriptorTable->ServiceTable.ArgumentTable;

	//拷贝Shadow表
	//判断GUI函数表是否已经加载，如果不是GUI线程，则操作系统不会挂载GUI函数表

	//思路判断Shadwo 函数表的物理页属性
	
	//if (影子表中的函数表被挂载了)
	//{
	//	//申请影子表中函数表的内存
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


// 遍历进程 找到进程
ULONG FindProcess(char* ProcessName)
{

	ULONG Pro;
	//由KPCR+0x124的位置得到当前CPU正在处理的线程的结构体的指针，_KTHREAD + 0x44 的位置能够得到此线程的进程结构体的地址
	__asm {
		mov eax, fs: [0x124] ;
		mov ecx, [eax + 0x44];
		mov Pro, ecx;
	}
	//进程结构体EPROCESS +0x88的位置是一个链接所有进程的双向链表
	PLIST_ENTRY pListProcess = (PLIST_ENTRY)(Pro + 0x88);
	BOOLEAN Flag = FALSE;

	//遍历进程
	while (pListProcess->Flink != (PLIST_ENTRY)(Pro + 0x88))
	{
		//EPROCESS 结构体
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
	//由进程结构体遍历线程
	//获取线程的双向链表
	//+0x190 ThreadListHead   : _LIST_ENTRY [ 0x89ff024c - 0x89dbd56c ]
	PLIST_ENTRY ThreadList = (PLIST_ENTRY)(eProc + 0x190);
	//找了半天错误 原来是这里没有获取 焯！  PLIST_ENTRY CurrentThreadList = ThreadList
	PLIST_ENTRY CurrentThreadList = ThreadList->Flink;
	//遍历线程

	do
	{
		//获取当前线程结构体的首部
		ULONG CurrentThread = (ULONG)CurrentThreadList - 0x22c;
		//取出线程中存储的SSDT的值
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
	//遍历线程
	do
	{
		//获取当前线程结构体的首部
		ULONG CurrentThread = (ULONG)CurrentThreadList - 0x22c;
		//取出线程中存储的SSDT的值
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
	DbgPrint("WorkThreadFunc 开始运行!\t\n");

	while (ThreadFlag)
	{
		KeDelayExecutionThread(KernelMode, FALSE, &timer);
		pProcess = FindProcess("Dbgview.exe"); //一直查找此进程，因为此进程可能会被关闭
		TraversalThreadReplaceE0(pProcess);
	}
	ThreadFlag = TRUE;
	ResetThreadReplaceE0(pProcess);
	UnHookNtOpenProcess();
	DbgPrint("WorkThreadFunc 结束运行!\t\n");
	ZwClose(hThread);
}


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pRegPath)
{
	pDriver->DriverUnload = DriverUnload;

	DbgPrint("我是驱动，我运行了!\t\n");
	// 拷贝一份SSDT表

	if (!NT_SUCCESS(GetCopySSDT()))
	{
		DbgPrint("GetCopySSDT Failed!\t\n");
		return STATUS_UNSUCCESSFUL;
	}
	
	//打印一下MySSDT
	DbgPrint("MySSDT:%p\t\n", MySSDT);
	//定义老的NtOpenProcess地址
	uOldNtOpenProcess = MySSDT->ServiceTable.FuncTable[0x7A];
	DbgPrint("uOldNtOpenProcess:%p\t\n", uOldNtOpenProcess);
	//开始Hook
	HookNtOpenProcess();
	// 遍历进程 找到进程
	//遍历线程 替换E0位置
	//设置定时器，每隔10毫秒修改一次
	//结束时恢复E0，结束HOOK，关闭句柄
	extern HANDLE hThread;
	PsCreateSystemThread(&hThread, 0, NULL, NULL, NULL, WorkThreadFunc, NULL); 
	
	return STATUS_SUCCESS;
}