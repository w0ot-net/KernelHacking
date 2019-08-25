#pragma once
#include "stdafx.h"
#include "common.h"
#include "Fuzzer.h"
#include "FuzzingProvider.h"

// Processors / threads constants
#define WINDOWS_MAX_PROCS 64
#define MAX_THREADS 2*WINDOWS_MAX_PROCS
#define ASYNC_CLEANUP_TIMEOUT 10000 // 10s for threads to do cleanup
// VERBOSITY_DEFAULT Concurency constants
#define MAX_PENDING 64 // max number of concurrent requests pending
#define CANCEL_RATE 15 // percentage of pending I/O to issue a cancel for
// Special requests for async fuzzer
#define SPECIAL_PACKET 1
#define SPECIAL_OVERLAPPED_START (LPOVERLAPPED)0xFFFFFFFF
#define SPECIAL_OVERLAPPED_DONE (LPOVERLAPPED)0xFFFFFFFE

class AsyncFuzzer : public Fuzzer
{
public:
    AsyncFuzzer(ULONG, ULONG, ULONG, FuzzingProvider*);
    ~AsyncFuzzer();
    BOOL init(tstring, ULONG);
    BOOL start();
private:
    // Members
    HANDLE hIocp;
    HANDLE *threads;
    ULONG maxPending;
    ULONG cancelRate;
    UINT startingNbThreads;
    volatile ULONG currentNbThreads;
    // Functions
    static DWORD WINAPI Iocallback(PVOID);
    BOOL InitializeThreadsAndCompletionPort();
    ULONG GetNumberOfProcs();
    BOOL CreateThreads();
    BOOL AllowNewAllocation();
    BOOL DequeueIoPacket(IoRequest**);
};
