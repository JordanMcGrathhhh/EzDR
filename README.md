# EzDR
Experimental EDR Platform Utilizing Event Tracing for Windows (ETW)

## Why ETW? 
Obviously, ETW is not a full-fledged EDR solution. A kernel-bound driver will always be closer to the "source of truth." ETW, however, is an efficient, built-in, and interesting protocol for an EDR proof-of-concept (PoC) like EzDR. While EzDR will never replace a kernel-bound EDR driver, I do hope to eventually achieve detection and response capabilities from ETW data. 

## Why not just use the Sysmon ETW trace session? 
That would make things significantly easier, I probably should have. Something about making this project as difficult as possible seemed appealing to me. While it's caused many stress-induced headaches, it has also taught me a lot!

## Current Issues
Currently, EzDR reads logs from logman-generated ETW streams only. This will hopefully be fixed soon. The command to generate this stream is detailed in EzDR.cpp
_logman start EzDRLogger "Microsoft-Windows-Kernel-Process" 0x10, 0x40 -ets -rt_
