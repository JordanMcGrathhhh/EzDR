#include <Windows.h>
#include <tdh.h>
#include <evntrace.h>
#include <timezoneapi.h>

#include <stdio.h>

#include "TraceManager.cpp"

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "tdh.lib")

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

        DWORD nameOffset = TraceEventInfoBuffer->EventPropertyInfoArray[i].NameOffset; 
        PCWSTR propertyName = (PCWSTR)((PBYTE)TraceEventInfoBuffer + nameOffset);
        wprintf(L"[*] Parsing field: %s = ", propertyName);

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
        
        free(buffer);
    }

    free(TraceEventInfoBuffer);
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