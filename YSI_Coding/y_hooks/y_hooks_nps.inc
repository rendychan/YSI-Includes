#define HOOK_NATIVE__%0(%1)

#define HOOK_PUBLIC__%0(%1) FUNC_PARSER(YH_NPS,ARR_MUL_CST:STR_CST_DEF:NUM_CST_DEF:REF_DEF:EXT_TAG:)(%0(%1))()()(1,0,(@H_y,@K_y))
#define HOOK_NATIVE__ HOOK_PUBLIC__

#define @H_y%0\32; @H_y // hook public
#define @K_y%0\32; @K_y // public extras

// Arrays.
//
// %0  = `const`
// %1  = Tag (unused)
// %2  = Name
// %4+ = Dims
#define YH_NPS_ARR(%0,%1,%2,%4)%8$ YH_NPS_BYREF(%0%2[%4],%2)%8$
#define YH_NPS_ARR_ARR(%0,%1,%2,%4,%5)%8$ YH_NPS_BYREF(%0%2[%4][%5],%2)%8$
#define YH_NPS_ARR_ARR_ARR(%0,%1,%2,%4,%5,%6)%8$ YH_NPS_BYREF(%0%2[%4][%5][%6],%2)%8$

// Strings.
//
// %0  = `const`
// %1  = Tag (unused)
// %2  = Name
// %4  = Size
// %3  = Default (unused)
#define YH_NPS_STR(%0,%1,%2,%4)%8$ YH_NPS_BYREF(%0%2[%4],%2)%8$
#define YH_NPS_STR_DEF(%0,%1,%2,%4,%3)%8$ YH_NPS_BYREF(%0%2[%4],%2)%8$

// Varargs.
//
// %0  = `const` (unused)
// %1  = Tag
// %2  = Name (unused)
// %5  = Prototype parameters
// %6  = Call parameters
#define YH_NPS_EXT(%0,%1,%2)%8$(%5)(%6)(%9,%4,%7) %8$(%5,%1...)(%6)(%9,%4,%7)

// References.
//
// %0  = `const` (unused)
// %1  = Tag (unused)
// %2  = Name
// %3  = Default (unused)
#define YH_NPS_REF(%0,%1,%2)%8$ YH_NPS_BYREF(&%2,%2)%8$
#define YH_NPS_REF_DEF(%0,%1,%2,%3)%8$ YH_NPS_BYREF(&%2,%2)%8$

// Variables.
//
// %0  = `const`
// %1  = Tag (unused)
// %2  = Name
// %3  = Default (unused)
#define YH_NPS_NUM(%0,%1,%2)%8$ YH_NPS_BYVAL(%0%2,%2)%8$
#define YH_NPS_NUM_DEF(%0,%1,%2,%3)%8$ YH_NPS_BYVAL(%0%2,%2)%8$

// Generate the parameter descriptions.
//
// %0  = Name in prototype
// %1  = Name in call
// %5  = Prototype parameters
// %6  = Call parameters
// %9  = Shift
// %4  = Existing values
// %7  = Prefix
#define YH_NPS_BYREF(%0,%1)%8$(%5)(%6)(%9,%4,%7) %8$(%5,%0)(%6,%1)(%9+1,%4,%7)
#define YH_NPS_BYVAL(%0,%1)%8$(%5)(%6)(%9,%4,%7) %8$(%5,%0)(%6,%1)(%9+1,1<<%9|%4,%7)

// End, generate the code.
//
// %0  = Name
// %3  = Descriptor
// %5  = Prototype parameters
// %6  = Call parameters
// %9  = Shift
// %4  = Existing values
// %7  = Prefix0
// %1  = Prefix1
#define YH_NPS_END(%0)%8$(,%5)(,%6)(%9,%4,(%7,%1)) %8$_@Ky<%7%0>(%5);_@Ky<%1%0>(%5);_@Ky<%1%0>(%5)_@Ky(%4),%0(%6),_@yK();_@Ky<%7%0>(%5)
#define YH_NPS_NUL(%0)%8$()() YH_NPS_END(%0)%8$(,)(,)

// Helper macro to generate the unique function name without bloating the
// generated code multiple times.
#define _@Ky<%0> UNIQUE_FUNCTION<%0@...>

/*

Trampoline code, to add the indirection pattern to the start of the parameter
list.  I.e. change this:

	SetPlayerHealth(playerid, 5.5);

To:

	SetPlayerHealth("if", playerid, 5.5);

This function is called, and just needs to insert a new function.

// No `PROC`, so no adjusted frame yet.
#emit POP.alt                 // Store return address.
#emit POP.pri                 // Store parameter count.
#emit PUSH.C   "param-string" // Pre-get the address.
#emit ADD.C    4              // Increase the parameter count.
#emit PUSH.pri
#emit PUSH.alt
#emit JUMP     TrueFunction   // Jump to the start of the implementation.

*/

// TODO: Read this data directly in `Hooks_Continue_`.
#define continue(%0) Hooks_Continue_(%0)

stock _@yK(&a = 0, &b = 0, &c = 0, &d = 0, &e = 0, &f = 0)
{
	#pragma unused a, b, c, d, e, f
	// This function does nothing.  It merely exists to reserve space for
	// codegen.  According to my experiments, just 4 parameters are enough to
	// reserve enough space (exactly) for the code generated below, even for a
	// function with no parameters.  But better to be safe than sorry.
}

new
	YSI_g_sNPSTrampoline,
	YSI_g_sNPSReplace,
	YSI_g_sNPSStack;

stock _@Ky(compressedFormat)
{
	#pragma unused compressedFormat
	// Get the next function call address, being the original function.  This
	// can also tell us if it is a native, public, or stock (which is a nice
	// side-effect I wasn't planning).
	new
		dctx[DisasmContext],
		ctx[AsmContext],
		addr,
		type = 0;
	DisasmInit(dctx, GetCurrentFrameReturn());
	while (DisasmNext(dctx))
	{
		switch (DisasmGetOpcode(dctx))
		{
		case OP_CALL:
		{
			addr = DisasmGetOperandReloc(dctx);
			type = 1;
			break;
		}
		case OP_SYSREQ_C:
		{
			addr = DisasmGetOperand(dctx);
			type = 2;
			break;
		}
		case OP_SYSREQ_D:
		{
			type = 3;
			break;
		}
		}
	}
	if (type == 2)
	{
		// Native function call.
		addr = -addr;
	}

	AsmInitPtr(ctx, YSI_g_sNPSTrampoline + AMX_HEADER_COD, YSI_g_sNPSReplace - YSI_g_sNPSTrampoline);
	// Forward the function, storing the chain data in the heap.   Note that no
	// `PROC` is added to this new trambopoline:
	//
	//   https://www.youtube.com/watch?v=geHqnV4Mk_4
	//

	// Update the stack pointer.
	@emit HEAP         24 // Old heap in alt.
	@emit LOAD.pri     ref(YSI_g_sNPSStack)
	@emit STOR.I
	@emit STOR.alt     ref(YSI_g_sNPSStack)

	// Overlap the stack and heap.
	@emit LCTRL        4 // Old stack in pri.
	@emit SUB
	@emit ADD.C        -4
	@emit LOAD.S.alt   8
	@emit ADD

	// Move the stack to touch the heap.
	@emit HEAP         0
	@emit XCHG
	@emit SCTRL        4

	// Push the fake header.
	@emit PUSH.C       compressedFormat
	@emit PUSH.C       addr
	@emit PUSH.alt
	@emit PUSH.S       4
	@emit PUSH.S       0

	// The stack is now 4 bytes from the old heap position, and within the new
	// heap.  Put it back where it was, with our data now in the heap.
	@emit LCTRL        5
	@emit LCTRL        4










	@emit POP.pri      // Get the true return address.
	@emit POP.alt      // Get the arg count.
	@emit PUSH.alt
	@emit PUSH.C       compressedFormat
	@emit PUSH.C       addr
	@emit PUSH.pri     // Swap later.

	// Get the stack.
	@emit LCTRL        4
	@emit ADD.C        4
	@emit ADD          4
	@emit HEAP         24
	@emit SUB

	// Put the new arg count and original return on the stack.
	@emit SWAP.pri
	@emit PUSH.pri

	@emit LCTRL       5
	@emit PUSH.pri
	
	@emit PUSH        ref(YSI_g_sNPSStack)
	@emit STOR.alt    ref(YSI_g_sNPSStack)
	@emit LCTRL       4
	@emit MOVS        24
	@emit STACK       24
	@emit CALL.abs     YSI_g_sNPSReplace

//label_below:







	// Build the structure in the stack first.
	@emit POP.pri
	@emit PUSH.C       compressedFormat
	@emit PUSH.C       addr
	@emit PUSH.C       8
	@emit PUSH.pri     // Save the return address again.
	@emit PUSH         ref(YSI_g_sNPSStack)

	// Copy 20 bytes over (includes the return address and fake param count).
	@emit HEAP         20
	@emit LCTRL        4
	@emit MOVS         20
	@emit STOR.alt     ref(YSI_g_sNPSStack)

	// Call, and return to here to clean up the heap.
	@emit STACK        20
	@emit CALL.abs     YSI_g_sNPSReplace

	// Put data (and return value) on the stack.
	@emit STACK        -20
	@emit STACK        0
	@emit PUSH.pri

	// Copy from the heap.
	@emit LOAD.pri     ref(YSI_g_sNPSStack)
	@emit MOVS         20

	// Restore the return address and `continue` stack.
	@emit POP.pri
	@emit POP.alt
	@emit STOR.alt     ref(YSI_g_sNPSStack)
	@emit HEAP         -20

	// Return from this "function", with our fake parameters and return address.
	// We do this instead of using `SCTRL 6`, as is the normal way, to preserve
	// `pri` as the return value.
	@emit PROC
	@emit RETN

	AsmEmitPadding(ctx);

	// Do a double return - exit the calling function as well as this function,
	// so that the next call is never actually made.
	#emit LOAD.S.pri   0
	#emit SCTRL        5
	#emit SCTRL        4
	// Clear faked parameter counts.   The outer public (maybe) expects lots of
	// parameters.  We call it with none.
	#emit ZERO.S       8
	#emit RETN
}

stock Hooks_Continue_(GLOBAL_TAG_TYPES:...)
{
	static
		addr,
		args;

	#emit ADDR.pri     8
	#emit STOR.pri     addr

	#emit LOAD.pri     YSI_g_sNPSStack
	#emit ADD.C        16
	#emit LOAD.I
	#emit STOR.pri     args

	while ((args >>>= 1))
	{
		addr += 4;
		if (args & 1)
		{
			#emit LREF.pri     addr
			#emit LOAD.I
			#emit SREF.pri     addr
		}
	}
	{}

	#emit LOAD.pri     YSI_g_sNPSStack
	#emit ADD.C        12
	#emit LOAD.I
	#emit STOR.pri     addr

	// `CALL` or `SYSREQ`.
	if (addr > 0)
	{
		// Reset the current frame.
		#emit POP.pri
		#emit SCTRL        5

		// Jump to the function.
		#emit LOAD.pri     addr
		#emit LCTRL        8
		#emit SCTRL        6
	}
	else
	{
		// Save the bottom of the stack.
		#emit POP.alt
		#emit LOAD.pri     addr
		#emit STOR.alt     addr
		#emit POP.alt

		#emit NEG
		#emit SYSREQ.pri

		// Returns to here.
		#emit PUSH.alt
		#emit PUSH         addr
		#emit RETN
	}

	return 0;

/*
	Fast target jump code (for v0.2)

	// Replace calls to this function from:

//	#emit PUSH.S       args__
//	#emit PUSH.S       func__
//	#emit PUSH.C       n
//	#emit CALL         FastJump

	// To:

//	#emit CONST.alt    ARGS
//	#emit CONST.pri    FUNC + 4
//	#emit PUSH.C       n - 8
//	#emit CALL         FastJump

	// If `FUNC` is `0`:

//	#emit NOP
//	#emit NOP
//	#emit NOP
//	#emit NOP
//	#emit STACK        n - 8
//	#emit CONST.pri    0

	// Function:

	#emit PROC                 // Auto-generated.

//	#emit JZER         FastJump_retn

	#emit PUSH.pri             // Store the function to call.
	// Get the start of the normal arguments.
	#emit ADDR.pri     8
	#emit PUSH.pri

FastJump_loop:
		#emit POP.pri
		#emit ADD.C        4
		#emit PUSH.pri

		// Check if `alt` is `0` (no parameters) or odd (move this parameter).
		#emit CONST.pri    1
		#emit SHR.C.alt    1
		#emit JGRTR        FastJump_done
		#emit AND
		#emit JZER         FastJump_loop

		// Adjust the given parameter.
		#emit LREF.S.pri   -8
		#emit LOAD.I
		#emit SREF.S.pri   -8
		#emit JUMP         FastJump_loop

FastJump_done:
	#emit POP.pri
	#emit POP.pri
	#emit SCTRL        6

//FastJump_retn:
//	#emit RETN

*/
}
