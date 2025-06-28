#include <evntrace.h>
/*
#define PRIVATE_LOGGER_NAME L"EzDRLogger"

typedef struct EventTracePropertyData {
    EVENT_TRACE_PROPERTIES Props;
    WCHAR LoggerName[128];
    WCHAR LogFileName[1024];
} EventTracePropertyData;

GUID DummyGuid = {
    0x00000001, 0x0000, 0x0000,
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }
};

ULONG WINAPI StartEzTrace(PTRACEHANDLE hStartEzTrace)
{
    ULONG startStatus = 0;
    TRACEHANDLE hStartTrace;

    // Typically, this config wouldn't be here but
    // this information is intentionally static in this case

    EventTracePropertyData data = { 0 };

    data.Props.Wnode.BufferSize = sizeof(data);
    data.Props.Wnode.Guid = SystemTraceControlGuid;
    data.Props.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    data.Props.MinimumBuffers = 4;
    data.Props.MaximumBuffers = 8;
    data.Props.MaximumFileSize = 0; // Real-time session
    data.Props.LogFileMode = EVENT_TRACE_SYSTEM_LOGGER_MODE | EVENT_TRACE_REAL_TIME_MODE;
    data.Props.FlushTimer = 5;
    data.Props.EnableFlags = 0x00000001;
    data.Props.LogFileNameOffset = 0; // Real-time session
    data.Props.LoggerNameOffset = offsetof(EventTracePropertyData, LoggerName);

    wcscpy_s(data.LoggerName, L"EzDRLogger");
    wcscpy_s(data.LogFileName, L"");

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
*/