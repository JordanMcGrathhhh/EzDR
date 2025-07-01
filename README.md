# EzDR
Experimental EDR Platform Utilizing Event Tracing for Windows (ETW)

## Why ETW? 
Obviously, ETW is not a full-fledged EDR solution. A kernel-bound driver will always be closer to the "source of truth." ETW, however, is an efficient, built-in, and interesting protocol for an EDR proof-of-concept (PoC) like EzDR. While EzDR will never replace a kernel-bound EDR driver, I do hope to eventually achieve detection and response capabilities from ETW data. 

## Why not just use the Sysmon ETW trace session? 
That would make things significantly easier, I probably should have. Something about making this project as difficult as possible seemed appealing to me. While it's caused many stress-induced headaches, it has also taught me a lot!

## Usage
EzDR will start and stop trace sessions automatically. Simply compile and run EzDR to begin logging the currently supported _EVENT_TRACE_FLAGS_*_. 

With some basic modifications you could also configure EzDR to read logman-controlled ETW sessions. (Just comment out the _startEzTrace()_ calls!) 
