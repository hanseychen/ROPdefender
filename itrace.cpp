/*BEGIN_LEGAL
Intel Open Source License

Copyright (c) 2002-2014 Intel Corporation. All rights reserved.

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
#include <stdio.h>
#include "pin.H"

/* ===================================================================== */
/* Basic stack operations                                                */
/* ===================================================================== */
class Stack {
public:
	ADDRINT address;
	Stack *next;

    Stack(){};
	Stack(ADDRINT addr): address(addr){};

	void push(ADDRINT address) {
		Stack *s = new Stack(address);
		s->next = next;
		next = s;
	}

	void pop() {
		if(!next) {
			//cout << "stack empty!" << endl;
		} else {
			Stack *s = next;
			next = next->next;
			delete s;
		}
	}

	ADDRINT top() {
		return next->address;
	}
};

FILE * trace;
int indent = 0;
Stack s;

VOID onCall(ADDRINT address)
{
    s.push(address);
    for(int i = 0; i < indent; i++){
        fprintf(trace, "  ");
    }
    indent++;
    fprintf(trace, "call\t%lx\n", address);
}

VOID onRet(ADDRINT address)
{
    for(int i = 0; i < indent; i++){
        fprintf(trace, "  ");
    }
    indent--;
    fprintf(trace, "ret\t%lx", address);
    if(s.top() != address){
        fprintf(trace, "\treturn address doesn't match!\n");
    } else {
        fprintf(trace, "\n");
    }
    s.pop();
}

// Pin calls this function every time a new instruction is encountered
VOID Instruction(INS ins, VOID *v)
{
    ADDRINT nextAddress;
    // Insert a call to printip before every instruction, and pass it the IP
    if(INS_IsCall(ins)){
        nextAddress = INS_NextAddress(ins);
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)onCall, IARG_ADDRINT, nextAddress, IARG_END);
    }
    if(INS_IsRet(ins)){
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)onRet, IARG_BRANCH_TARGET_ADDR, IARG_END);
    }
}

// This function is called when the application exits
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
    PIN_ERROR("This Pintool prints the IPs of every instruction executed\n"
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char * argv[])
{
    trace = fopen("itrace.out", "w");

    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}
