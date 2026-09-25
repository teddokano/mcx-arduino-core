// ****************************************************************************
// semihost_hardfault.c
//                - Provides hard fault handler to allow semihosting code not
//                  to hang application when debugger not connected.
//
// ****************************************************************************
// Copyright 2017-2025 NXP
// All rights reserved.
//
// SPDX-License-Identifier: BSD-3-Clause
// ****************************************************************************
//
//                       ===== DESCRIPTION =====
//
// One of the issues with applications that make use of semihosting operations
// (such as printf calls) is that the code will not execute correctly when the
// debugger is not connected. Generally this will show up with the application
// appearing to just hang. This may include the application running from reset
// or powering up the board (with the application already in FLASH), and also
// as the application failing to continue to execute after a debug session is
// terminated.
//
// The problem here is that the "bottom layer" of the semihosted variants of
// the C library, semihosting is implemented by a "BKPT 0xAB" instruction.
// When the debug tools are not connected, this instruction triggers a hard
// fault - and the default hard fault handler within an application will
// typically just contains an infinite loop - causing the application to
// appear to have hang when no debugger is connected.
//
// The below code provides an example hard fault handler which instead looks
// to see what the instruction that caused the hard fault was - and if it
// was a "BKPT 0xAB", then it instead returns back to the user application.
//
// In most cases this will allow applications containing semihosting
// operations to execute (to some degree) when the debugger is not connected.
//
// == NOTE ==
//
// Correct execution of the application containing semihosted operations
// which are vectored onto this hard fault handler cannot be guaranteed. This
// is because the handler may not return data or return codes that the higher
// level C library code or application code expects. This hard fault handler
// is meant as a development aid, and it is not recommended to leave
// semihosted code in a production build of your application!
//
// ****************************************************************************
//
// mcx-arduino-core: modified from the SDK's copy where marked, so that a
// fault other than semihosting prints what happened instead of hanging
// silently.
//
// ****************************************************************************

// Allow handler to be removed by setting a define (via command line)
#if !defined (__SEMIHOST_HARDFAULT_DISABLE)

__attribute__((naked))
void HardFault_Handler(void){
    __asm(  ".syntax unified\n"
        // Check which stack is in use
            "MOVS   R0, #4           \n"
            "MOV    R1, LR           \n"
            "TST    R0, R1           \n"
            "BEQ    _MSP             \n"
            "MRS    R0, PSP          \n"
            "B  _process             \n"
            "_MSP:                   \n"
            "MRS    R0, MSP          \n"
        // mcx-arduino-core: a BKPT only gets here as HFSR.DEBUGEVT, so the
        // instruction at the stacked PC is read only then. Any other fault
        // may have a stacked PC that can't be read (a jump to a bad
        // address), and a fault in here would lock the core up.
        "_process:                   \n"
            "LDR    R2,=0xE000ED2C   \n"  // SCB->HFSR
            "LDR    R2,[R2]          \n"
            "CMP    R2,#0            \n"
            "BPL    _hf_report       \n"  // DEBUGEVT (bit 31) clear
        // Load the instruction that triggered hard fault
            "LDR    R1,[R0,#24]      \n"
            "LDRH   R2,[r1]          \n"
        // Semihosting instruction is "BKPT 0xAB" (0xBEAB)
            "LDR    R3,=0xBEAB       \n"
            "CMP    R2,R3            \n"
            "BEQ    _semihost_return \n"
        // mcx-arduino-core: wasn't semihosting, so report the fault on the
        // USB serial port and blink SOS (mcx_fault_report() in mcu.cpp)
        // instead of the original silent "B .". If the stack is within 1KB
        // of its limit (a stack overflow, usually) the report gets a fresh
        // stack from the top; otherwise the stack is kept, so a debugger
        // paused later can still unwind to the fault.
        "_hf_report:                 \n"
            "MRS    R2, MSP          \n"
            "LDR    R3,=_pvHeapLimit \n"
            "ADD    R3, R3, #1024    \n"
            "CMP    R2, R3           \n"
            "BHS    _hf_keep_stack   \n"
            "LDR    R3,=_vStackTop   \n"
            "MSR    MSP, R3          \n"
        "_hf_keep_stack:             \n"
            "MOV    R1, LR           \n"  // EXC_RETURN
            "B      mcx_fault_report \n"
        // Was semihosting instruction, so adjust location to
        // return to by 1 instruction (2 bytes), then exit function
            "_semihost_return:       \n"
            "ADDS   R1,#2            \n"
            "STR    R1,[R0,#24]      \n"
        // Set a return value from semihosting operation.
        // 0 is slightly arbitrary, but appears to allow most
        // C Library IO functions sitting on top of semihosting to
        // continue to operate to some degree
        // Return a positive value (32) for SYS_OPEN only
            "LDR    R1,[ R0,#0 ]     \n"  // R0 is at location 0 on stack
            "CMP    R1, #1           \n"  // 0x01=SYS_OPEN
            "BEQ    _non_zero_ret    \n"
            "MOVS   R1,#0            \n"
            "B      _sys_ret         \n"
            "_non_zero_ret:          \n"
            "MOVS   R1,#32           \n"
            "_sys_ret:               \n"
            "STR    R1,[ R0,#0 ]     \n" // R0 is at location 0 on stack
        // Return from hard fault handler to application
            "BX     LR               \n"
        ".syntax divided\n") ;
}

#endif

