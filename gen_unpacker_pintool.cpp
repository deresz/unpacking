/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2013 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
/*
 *  This file contains an ISA-portable PIN tool for tracing memory accesses.
 */

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include "pin.H"

typedef std::vector<ADDRINT> addrdeq_t;
addrdeq_t write_address;

FILE * trace;

// Print a memory write record
VOID RecordMemWrite(VOID * ip, VOID * addr)
{
    write_address.push_back((ADDRINT)addr);
}

inline ADDRINT was_written(ADDRINT ea)
{
  return std::find(write_address.begin(), write_address.end(), ea) != write_address.end();
}

VOID check_unpacked_cb(VOID * ip, const CONTEXT *ctxt, THREADID tid)
{ 
  // we clear the current instruction map
  write_address.clear();
  cerr << "Layer unpacked: " << ip << endl; 
  PIN_ApplicationBreakpoint(ctxt, tid, false, "Layer unpacked!");
}

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
    // Instruments memory accesses using a predicated call, i.e.
    // the instrumentation is called iff the instruction will actually be executed.
    //
    // On the IA-32 and Intel(R) 64 architectures conditional moves and REP 
    // prefixed instructions appear as predicated instructions in Pin.
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        // Note that in some architectures a single memory operand can be 
        // both read and written (for instance incl (%eax) on IA-32)
        // In that case we instrument it once for read and once for write.
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp,
                IARG_END);
        }

    }
}

static VOID trace_cb(TRACE trace, VOID *v)
{
  // check if trace is on written region
  for ( BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl) )
  {
	  INS ins = BBL_InsHead(bbl);
	  ADDRINT ea = INS_Address(ins);
	  //cout << "Checking ea: " << hexstr(ea) << endl;
	  if(was_written(ea)) {
		INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)check_unpacked_cb,
            IARG_INST_PTR,
            IARG_CONST_CONTEXT,
            IARG_THREAD_ID,
            IARG_END);
	  }
  }
}

VOID Fini(INT32 code, VOID *v)
{
    fprintf(trace, "#eof\n");
    fclose(trace);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */
   
INT32 Usage()
{
    PIN_ERROR( "This Pintool stops the debugger when code written in runtime is executed\n" 
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{	
	// sometimes PIN wasn't printing to console...
	//std::ofstream outta("out.txt"); 
    //std::cout.rdbuf(outta.rdbuf());
	//std::cerr.rdbuf(outta.rdbuf());
	
	cerr << "Pintool loaded ... " << endl;


	if (PIN_Init(argc, argv)) return Usage();

	TRACE_AddInstrumentFunction(trace_cb, 0);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    cerr << "Starting instrumentation .... " << endl;

	PIN_StartProgram();
    
    return 0;
}
