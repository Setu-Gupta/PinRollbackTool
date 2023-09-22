/*! @file
 *  This is a test of the CONTEXT API of Pin
 */

#include "pin.H"
#include <iostream>
#include <fstream>
using std::cerr;
using std::cout;
using std::endl;

std::ostream* out = &cout;
ADDRINT first_instr_addr = 0;
BOOL executing_on_wrong_path = false;
UINT wrong_path_resolution_count = 0;
UINT wrong_path_instructions_executed = 0;
CONTEXT rollback_ctxt;

/* ===================================================================== */
// Command Line Switches
/* ===================================================================== */
KNOB<UINT> KnobBranchResolutionTime(KNOB_MODE_WRITEONCE, "pintool", "n", "0", "specify the branch resolution time in terms of number of wrong path instructions");

/* ===================================================================== */
// Utilities
/* ===================================================================== */

/*!
 *  Print out help message.
 */
INT32 Usage()
{
	cerr << "This tool tests the CONTEXT API of Pin" << endl;
	return -1;
}

/* ===================================================================== */
// Analysis routines
/* ===================================================================== */

/*!
 * Increment wrong_path_instructions_executed if executing_on_wrong_path
 */
VOID CheckIfOnWrongPath()
{
	if(executing_on_wrong_path)
		wrong_path_instructions_executed++;
	else
		wrong_path_instructions_executed = 0;
}

/*!
 * Rollback if more than wrong_path_resolution_count instructions have been executed on the wrong path
 */
VOID Rollback()
{
	if(!executing_on_wrong_path)
		return;
	if(wrong_path_instructions_executed <= wrong_path_resolution_count)
		return;

	ADDRINT rollback_addr = PIN_GetContextReg(&rollback_ctxt, REG_INST_PTR);
	*out << "===================================" << endl;
	*out << "Rolling back to " << std::hex << rollback_addr << "(" << rollback_addr - first_instr_addr << ")" << endl;
	*out << "===================================" << endl;
	wrong_path_instructions_executed = 0;
	executing_on_wrong_path = false;
	
	PIN_ExecuteAt(&rollback_ctxt);
}

/*!
 * Record every instruction's address
 * @param[in]   addr	The address of an instruction
 */
VOID RecordInstrAddr(ADDRINT addr)
{
	CheckIfOnWrongPath();
	Rollback();

	if(first_instr_addr == 0)
		first_instr_addr = addr;
	if(executing_on_wrong_path)
		*out << "[ON WRONG PATH, " << wrong_path_instructions_executed << " wrong instructions executed] ";
	*out << "Executing instruction at " << std::hex << addr << "(" << std::hex << addr - first_instr_addr << ")" << endl;
}

/*!
 * Redirect the branch on the wrong path 
 * @param[in]   ctxt    		The current context
 * @param[in]	taken			True if the branch is taken, else false
 * @param[in]	taken_addr		Address of the taken instruction sequence
 * @param[in]	fallthrough_addr	Address of the fallthrough instruction sequence
 */
VOID Redirect(CONTEXT* ctxt, BOOL taken, ADDRINT taken_addr, ADDRINT fallthrough_addr)
{
	// Get the current instruction pointer
	ADDRINT rip = PIN_GetContextReg(ctxt, REG_INST_PTR);
	RecordInstrAddr(rip);
	
	// Compute the wrong path address
	ADDRINT wrong_path_addr = rip;	// Wrong path address
	ADDRINT right_path_addr = rip;	// Right path address
	if(taken != 0)
	{
		*out << "Taken branch" << endl;
		wrong_path_addr = fallthrough_addr;
		right_path_addr = taken_addr;
	}
	else
	{
		*out << "Not taken branch" << endl;
		wrong_path_addr = taken_addr;
		right_path_addr = fallthrough_addr;
	}
	
	// Save the correct path context
	PIN_SetContextReg(ctxt, REG_INST_PTR, right_path_addr);
	PIN_SaveContext(ctxt, &rollback_ctxt);
	wrong_path_instructions_executed = 0;

	// Prepare the wrong path context and start executing on the wrong path
	PIN_SetContextReg(ctxt, REG_INST_PTR, wrong_path_addr);
	executing_on_wrong_path = true;
	*out << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" << endl;
	*out << "Redirecting to wrong path address " << std::hex << wrong_path_addr << "(" << wrong_path_addr - first_instr_addr << ")" << endl;
	*out << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" << endl;
	PIN_ExecuteAt(ctxt);
}

/* ==================================================================== */
// Instrumentation callbacks
/* ===================================================================== */

/*
 * Insert call to the redirect branches
 * @param[in]	ins	Instruction to instrument
 */
VOID Instruction(INS ins, VOID*)
{
	// Insert call to redirect execution
	if(INS_IsControlFlow(ins) && INS_HasFallThrough(ins))
		INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)Redirect,
				IARG_CONTEXT,
				IARG_BRANCH_TAKEN,
				IARG_BRANCH_TARGET_ADDR,
				IARG_FALLTHROUGH_ADDR,
				IARG_END);
	// Insert call to print out the RIP
	else
		INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordInstrAddr, IARG_INST_PTR, IARG_END);
}

/*!
 * Print out analysis results.
 * This function is called when the application exits.
 * @param[in]   code            exit code of the application
 *                              PIN_AddFiniFunction function call
 */
VOID Fini(INT32 code, VOID* v)
{
	*out << "===============================================" << endl;
	*out << "PinRollbackTool Finished" << endl;
	*out << "===============================================" << endl;
}

/*!
 * The main procedure of the tool.
 * This function is called when the application image is loaded but not yet started.
 * @param[in]   argc            total number of elements in the argv array
 * @param[in]   argv            array of command line arguments, 
 *                              including pin -t <toolname> -- ...
 */
int main(int argc, char* argv[])
{
	// Initialize PIN library. Print help message if -h(elp) is specified
	// in the command line or the command line is invalid
	if (PIN_Init(argc, argv))
	{
		return Usage();
	}

	// Get the number of instructions to execute on the wrong path before rolling back the state 
	wrong_path_resolution_count = KnobBranchResolutionTime.Value();
	*out << "Branch resolution time set as " << wrong_path_resolution_count << " instructions" << endl;

	// Register function to be called to instrument instructions
	INS_AddInstrumentFunction(Instruction, 0);

	// Register function to be called when the application exits
	PIN_AddFiniFunction(Fini, 0);

	*out << "===============================================" << endl;
	*out << "PinRollbackTool Started" << endl;
	*out << "===============================================" << endl;
	// Start the program, never returns
	PIN_StartProgram();

	return 0;
}
