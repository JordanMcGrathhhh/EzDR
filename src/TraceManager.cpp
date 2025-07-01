#define PRIVATE_LOGGER_NAME L"EzDRLogger"

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
    data.Props.EnableFlags = EVENT_TRACE_FLAG_PROCESS | EVENT_TRACE_FLAG_IMAGE_LOAD | EVENT_TRACE_FLAG_NETWORK_TCPIP;
    data.Props.LogFileNameOffset = 0; // Real-time session
    data.Props.LoggerNameOffset = offsetof(EventTracePropertyData, LoggerName);

    wcscpy_s(data.LoggerName, PRIVATE_LOGGER_NAME);
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

// Ignore, this was for testing
/*
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
*/