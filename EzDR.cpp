#include <Windows.h>
#include <tdh.h>
#include <evntrace.h>
#include <timezoneapi.h>

#include <stdio.h>

//#include "EzDR.h"

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "tdh.lib")

#define PRIVATE_LOGGER_NAME L"EzDRLogger"

// Define this before using SystemTraceControlGuid
const GUID SystemTraceControlGuid =
{ 0x9e814aad, 0x3204, 0x11d2, {0x9a, 0x82, 0x00, 0x60, 0x08, 0xa8, 0x69, 0x39} };


typedef struct EventTracePropertyData {
    EVENT_TRACE_PROPERTIES Props;
    WCHAR LoggerName[128];
    WCHAR LogFileName[1024];
} EventTracePropertyData;

GUID DummyGuid = {
    0x00000000, 0x0000, 0x0000,
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

ULONG WINAPI StartEzTrace(PTRACEHANDLE hStartEzTrace)
{
    ULONG startStatus = 0;
    TRACEHANDLE hStartTrace;

    // Typically, this config wouldn't be here but
    // this information is intentionally static in this case

    EventTracePropertyData data = { 0 };

    data.Props.Wnode.BufferSize = sizeof(data);
    data.Props.Wnode.Guid = DummyGuid;
    data.Props.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    data.Props.MinimumBuffers = 4;
    data.Props.MaximumBuffers = 8;
    data.Props.MaximumFileSize = 0; // Real-time session
    data.Props.LogFileMode = EVENT_TRACE_SYSTEM_LOGGER_MODE | EVENT_TRACE_REAL_TIME_MODE;
    data.Props.FlushTimer = 5;
    //data.Props.EnableFlags = 0;
    data.Props.LogFileNameOffset = 0; // Real-time session
    //data.Props.LoggerNameOffset = offsetof(EventTracePropertyData, LoggerName);

    wcscpy_s(data.LoggerName, PRIVATE_LOGGER_NAME);
    //wcscpy_s(data.LogFileName, L"");

    startStatus = StartTraceW(&hStartTrace, data.LoggerName, &data.Props);

    if (startStatus == ERROR_ALREADY_EXISTS)
    {
        printf("[!] Error %d. Attempting to restart EzDR.\n", GetLastError());

        startStatus = ControlTraceW(0, data.LoggerName, &data.Props, EVENT_TRACE_CONTROL_STOP);
        printf("[*] Stopped EzDR. Restarting...\n");

        StartEzTrace(hStartEzTrace);
    }

    if (startStatus != ERROR_SUCCESS)
    {
        if (startStatus == ERROR_ACCESS_DENIED)
        {
            printf("[!] Error %d. Please run the program as Administrator.\n", GetLastError());
            exit(-1);
        }
        printf("[!] Error: %d. Failed StartTraceW();", GetLastError());
        exit(-1);
    }

    return startStatus;
}

void WINAPI handleEvent(PEVENT_RECORD EventRecord)
{
    FILETIME timestamp;
    SYSTEMTIME systemtime;

    timestamp.dwHighDateTime = EventRecord->EventHeader.TimeStamp.HighPart;
    timestamp.dwLowDateTime = EventRecord->EventHeader.TimeStamp.LowPart;
    
    FileTimeToSystemTime(&timestamp, &systemtime); // Systemtime = UTC

    printf("[INFO] System Timestamp: %02u:%02u:%02u\n", systemtime.wHour, systemtime.wMinute, systemtime.wSecond);
    printf("[INFO] Provided By: {%lx-%x-%x-%x%x-%x%x%x%x%x%x}\n",
        EventRecord->EventHeader.ProviderId.Data1,
        EventRecord->EventHeader.ProviderId.Data2,
        EventRecord->EventHeader.ProviderId.Data3,
        EventRecord->EventHeader.ProviderId.Data4[0], EventRecord->EventHeader.ProviderId.Data4[1],
        EventRecord->EventHeader.ProviderId.Data4[2], EventRecord->EventHeader.ProviderId.Data4[3],
        EventRecord->EventHeader.ProviderId.Data4[4], EventRecord->EventHeader.ProviderId.Data4[5],
        EventRecord->EventHeader.ProviderId.Data4[6], EventRecord->EventHeader.ProviderId.Data4[7]);
    printf("[INFO] Keyword: %llx\n", EventRecord->EventHeader.EventDescriptor.Keyword);

    // Event parsing

    ULONG status = ERROR_SUCCESS;

    ULONG TraceEventInfoBufferSize = 0;
    PTRACE_EVENT_INFO TraceEventInfoBuffer = nullptr; 

    // Per MS docs:
      // "If the buffer size is zero on input, no data is returned in the buffer and this parameter receives the required buffer size."
    // Therefore, we call TdhGetEventInformation with an obviously wrong (0) size to get the required buffer size for EventPropertyInfoArray
    status = TdhGetEventInformation(EventRecord, 0, NULL, NULL, &TraceEventInfoBufferSize);

    // We expect this to be the case ^^
    if(status == ERROR_INSUFFICIENT_BUFFER)
    {
        TraceEventInfoBuffer = (PTRACE_EVENT_INFO) malloc(TraceEventInfoBufferSize);
        status = TdhGetEventInformation(EventRecord, 0, NULL, TraceEventInfoBuffer, &TraceEventInfoBufferSize);
    }

    // If we still don't have 'ERROR_SUCCESS' something is actually wrong!
    // To Do: Investigate memory permission errors here!
    if(status != ERROR_SUCCESS)
    {
        printf("[!] Error: %d. Failed TdhGetEventInformation();\n", GetLastError());
    }
    
    // Before entering the loop, you set the UserData and UserDataLength parameters to the value of the UserData and UserDataLength members of the EVENT_RECORD structure, respectively.
    
    USHORT UserDataLength = EventRecord->UserDataLength;
    PBYTE UserData = reinterpret_cast<PBYTE>(EventRecord->UserData);

    for (int i = 0; i < TraceEventInfoBuffer->TopLevelPropertyCount; i++)
    {

        DWORD nameOffset = TraceEventInfoBuffer->EventPropertyInfoArray[i].NameOffset; //
        PCWSTR propertyName = (PCWSTR)((PBYTE)TraceEventInfoBuffer + nameOffset); //
        wprintf(L"[*] Parsing field: %s = ", propertyName); //

        ULONG formatPropertyStatus = ERROR_SUCCESS;

        ULONG bufferSize = 0;
        PWCHAR buffer = nullptr;

        USHORT UserDataConsumed = 0;

        formatPropertyStatus = TdhFormatProperty( 
            TraceEventInfoBuffer, 
            nullptr, // No map associated
            ((EventRecord->EventHeader.Flags == EVENT_HEADER_FLAG_32_BIT_HEADER) ?
                (ULONG) 4 : (ULONG) 8),
            TraceEventInfoBuffer->EventPropertyInfoArray[i].nonStructType.InType,
            TraceEventInfoBuffer->EventPropertyInfoArray[i].nonStructType.OutType,
            TraceEventInfoBuffer->EventPropertyInfoArray[i].length,
            UserDataLength, 
            UserData, 
            &bufferSize, 
            buffer, 
            &UserDataConsumed);
        
        if(formatPropertyStatus == ERROR_INSUFFICIENT_BUFFER)
        {
            buffer = (PWCHAR) malloc(bufferSize);

            if (buffer == NULL)
            {
                printf("[*] Error %d. Failed buffer malloc();", GetLastError());
            }

            formatPropertyStatus = TdhFormatProperty(
                TraceEventInfoBuffer,
                nullptr, // No map associated
                ((EventRecord->EventHeader.Flags == EVENT_HEADER_FLAG_32_BIT_HEADER) ?
                    (ULONG) 4 : (ULONG)8),
                TraceEventInfoBuffer->EventPropertyInfoArray[i].nonStructType.InType,
                TraceEventInfoBuffer->EventPropertyInfoArray[i].nonStructType.OutType,
                TraceEventInfoBuffer->EventPropertyInfoArray[i].length,
                UserDataLength,
                UserData,
                &bufferSize,
                buffer,
                &UserDataConsumed);
        }

        if (formatPropertyStatus != ERROR_SUCCESS)
        {
            printf("[!] Error: %d. Failed TdhFormatProperty();\n", GetLastError());
        }

        wprintf(L"%s\n", buffer);
        
        UserDataLength -= UserDataConsumed;
        UserData += (BYTE) UserDataConsumed;

        free(TraceEventInfoBuffer);
        free(buffer);
    }

    printf("\n");
    //exit(-1); // Temporary, used to only print one event for testing!
}   




int main(int argc, char* argv[])
{
    // For setting EzDRLogger ETS manually. (mostly deprecated)
    // logman start EzDRLogger -p "Microsoft-Windows-Kernel-Process" 0x10,0x40 -ets -rt
    // -ets: Send commands to Event Trace Sessions directly without saving or scheduling.
    // -rt: Run the Event Trace Session in real-time mode.
    // -p A single Event Trace provider to enable. The terms 'Flags' and 'Keywords' are synonymous in this context.

    TRACEHANDLE hStartTrace;
    ULONG startStatus = StartEzTrace(&hStartTrace);

    // Try enabling first? 

    EVENT_FILTER_DESCRIPTOR FilterDescriptor;
    ENABLE_TRACE_PARAMETERS EnableParameters;

    ZeroMemory(&EnableParameters, sizeof(EnableParameters));
    EnableParameters.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
    EnableParameters.EnableFilterDesc = &FilterDescriptor;
    EnableParameters.FilterDescCount = 1;

    const GUID WMKProcessGuid = { 0x22FB2CD6, 0x0E7B, 0x422B, { 0xA0, 0xC7, 0x2F, 0xAD, 0x1F, 0xD0, 0xE7, 0x16 } };
    
    ULONG enableStatus = EnableTraceEx2(
        hStartTrace,
        &WMKProcessGuid,
        EVENT_CONTROL_CODE_ENABLE_PROVIDER,
        TRACE_LEVEL_VERBOSE,
        0,
        0,
        0,
        &EnableParameters);

    // /Try enabling first?

    TRACEHANDLE hTrace;

    EVENT_TRACE_LOGFILEW LogFile = { 0 }; 
    LogFile.LoggerName = const_cast<LPWSTR>(PRIVATE_LOGGER_NAME);
    LogFile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
    LogFile.EventRecordCallback = &handleEvent;
    LogFile.Context = NULL;

    ULONG status = ERROR_SUCCESS;

    hTrace = OpenTraceW(
        &LogFile
    );

    if(hTrace == INVALID_PROCESSTRACE_HANDLE)
    {
        printf("[!] Error: %d. Failed OpenTraceW();\n", GetLastError());
        exit(-1);
    }

    printf("[*] Opened Trace Session!\n");

    status = ProcessTrace(&hTrace, 1, NULL, NULL);

    if(status != ERROR_SUCCESS)
    {
        printf("[!] Error: %lu. Failed ProcessTrace()\n", status);
    }

    CloseTrace(hTrace);

    printf("[*] Closed Trace Session.");

}