#include "stdafx.h"
#include "AsyncFuzzer.h"

// Static tracker
Fuzzer::Tracker Fuzzer::tracker;

// Trivial constructor
Fuzzer::Fuzzer(FuzzingProvider *p) : fuzzingProvider(p), hDev(INVALID_HANDLE_VALUE)
{
    TPRINT(VERBOSITY_DEBUG, _T("Fuzzer constructor\n"));
    this->state=STATE_FUZZING;
}

// Simple destructor
Fuzzer::~Fuzzer() {
    TPRINT(VERBOSITY_DEBUG, _T("Fuzzer destructor\n"));
    if(hDev!=INVALID_HANDLE_VALUE) {
        CloseHandle(hDev);
    }
    delete fuzzingProvider;
}

//DESCRIPTION:
// Control handler for CTRL-C handling.
//
//INPUT:
// fdwCtrlType - Received code.
//
//
//OUTPUT:
// TRUE - Handled
// FALSE - Forward to next registered handler
//
BOOL __stdcall Fuzzer::CtrlHandler(DWORD fdwCtrlType)
{
    BOOL bResult=FALSE;
    if(fdwCtrlType==CTRL_C_EVENT || fdwCtrlType==CTRL_BREAK_EVENT) {
        bResult = SetEvent(tracker.hEvent); // This triggers the end of async fuzzing stage
    }
    return bResult;
}

BOOL Fuzzer::WaitOnTerminationEvents(ULONG seconds)
{
    BOOL bResult=FALSE;

    if(fuzzingProvider->canGoCold) {
        HANDLE events[2] = {tracker.hEvent, fuzzingProvider->hEvent};
        if(WAIT_FAILED!=WaitForMultipleObjects(2, events, FALSE, seconds*1000)) {
            bResult = TRUE;
        }
    }
    else {
        if(WAIT_FAILED!=WaitForSingleObject(tracker.hEvent, seconds*1000)) {
            bResult = TRUE;
        }
    }
    return bResult;
}

//DESCRIPTION:
// Creates the all-fuzzers-wide bail event and registers CTRL-C handler
//
//INPUT:
// None
//
//OUTPUT:
// BOOL SUCCESS/FAILURE
//
Fuzzer::Tracker::Tracker()
{
    // Create the auto reset bail event (only the main thread ever waits on it)
    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(hEvent) {
        // Register ctrl-c handler
        if(!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE)) {
            TPRINT(VERBOSITY_INFO, _T("Failed to register control handler, ctrl-c will not work as expected\n"));
        }
    }
    else {
        TPRINT(VERBOSITY_ERROR, _T("Failed to create event, error %x\n"), GetLastError());
    }
    return;
}

Fuzzer::Tracker::~Tracker()
{
    if(!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, FALSE)) {
        TPRINT(VERBOSITY_INFO, _T("Failed to unregister control handler\n"));
    }
}

//DESCRIPTION:
// This function prints the RUN STARTED/RUN ENDED date & time string
//
//INPUT:
// ended - boolean controlling whether to print "RUN ENDED" OR "RUN STARTED"
//
//OUTPUT:
// None
//
VOID Fuzzer::printDateTime(BOOL ended)
{
    TCHAR timestr[64];
    TCHAR datestr[64];
    LPTSTR fmt = ended ? _T("Run ended: %s %s\n") : _T("Run started: %s %s\n");

    // Print date & time
    if(GetDateFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, datestr, 32) && GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, NULL, NULL, timestr, 32)) {
        TPRINT(VERBOSITY_DEFAULT, fmt, datestr, timestr);
    }
    else {
        TPRINT(VERBOSITY_DEFAULT, _T("Time unavailable\n"));
    }
    return;
}

//DESCRIPTION:
// This function prints the cummulative tracked statitists
//
//INPUT:
// pStats - Pointer to the stats struct
//
//OUTPUT:
// None
VOID Fuzzer::Tracker::Stats::print()
{
    // Wait for all the volatile writes to go through
    MemoryBarrier();
    // clean print
    fflush(stdout);
    // Print summary
    TPRINT(VERBOSITY_DEFAULT, _T("---------------------------------------\n"));
    TPRINT(VERBOSITY_DEFAULT, _T("Sent Requests : %llu\n"), SentRequests);
    TPRINT(VERBOSITY_DEFAULT, _T("Completed Requests : %llu (%llu sync, %llu async)\n"), CompletedRequests, SynchronousRequests, ASyncRequests);
    TPRINT(VERBOSITY_DEFAULT, _T("SuccessfulRequests : %llu\n"), SuccessfulRequests);
    TPRINT(VERBOSITY_DEFAULT, _T("FailedRequests : %llu\n"), FailedRequests);
    TPRINT(VERBOSITY_DEFAULT, _T("CanceledRequests : %llu\n"), CanceledRequests);
    TPRINT(VERBOSITY_INFO, _T("----\n"));
    TPRINT(VERBOSITY_INFO, _T("Consistent Results: %s\n"), SuccessfulRequests
        +FailedRequests
        +CanceledRequests
        == CompletedRequests ? _T("Yes" : _T("No (it's ok)")));
    // Cleanup completed
    if(!AllocatedRequests && !PendingRequests) {
        TPRINT(VERBOSITY_ALL, _T("Cleanup completed, no request still allocated nor pending\n"));
    }
    else {
        TPRINT(VERBOSITY_ALL, _T("Cleanup incomplete, %llu request%s still allocated, %llu pending\n"), AllocatedRequests, AllocatedRequests>1?_T("s"):_T(""), PendingRequests);
    }
    TPRINT(VERBOSITY_ALL, _T("----\n"));
    printDateTime(TRUE);
    TPRINT(VERBOSITY_DEFAULT, _T("---------------------------------------\n\n"));
    return;
}
