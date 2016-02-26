/** \file 
 * \brief A demonstration program for SqScripter GUI window
 *
 * It does not demonstrate anything like scripting.
 * It just shows how to setup and run the program to enable SqScripter window.
 */

#include "sqscripter.h"
#include "squirrel.h"

#include <windows.h>

#include <string>
#include <sstream>

/// Buffer to hold the function to print something to the console (the lower half of the window).
/// Set by scripter_init().
static void (*PrintProc)(ScripterWindow *, const char *) = NULL;

/// ScripterWindow object handle.  It represents single SqScripter window.
static ScripterWindow *sw = NULL;

/// The Squirrel virtual machine
static HSQUIRRELVM sqvm;

/// Function to handle commands come from the command line (the single line edit at the bottom of the window).
/// "Commands" mean anything, but it would be good to use for interactive scripting like Python or irb
void CmdProc(const char *cmd){
	SQInteger oldStack = sq_gettop(sqvm);
	scripter_clearerror(sw);
	if(SQ_FAILED(sq_compilebuffer(sqvm, cmd, strlen(cmd), _SC("command"), SQTrue))){
		sq_pop(sqvm, sq_gettop(sqvm) - oldStack);
		return;
	}
	sq_pushroottable(sqvm);
	if(SQ_FAILED(sq_call(sqvm, 1, SQFalse, SQTrue))){
		sq_pop(sqvm, sq_gettop(sqvm) - oldStack);
		return;
	}
	sq_pop(sqvm, sq_gettop(sqvm) - oldStack);
}

/// Function to handle batch exection of a script file.  File name is provided as the first argument if available.
static void RunProc(const char *fileName, const char *content){
	SQInteger oldStack = sq_gettop(sqvm);
	scripter_clearerror(sw);
	if(SQ_FAILED(sq_compilebuffer(sqvm, content, strlen(content), fileName, SQTrue))){
		sq_pop(sqvm, sq_gettop(sqvm) - oldStack);
		return;
	}
	sq_pushroottable(sqvm);
	if(SQ_FAILED(sq_call(sqvm, 1, SQFalse, SQTrue))){
		sq_pop(sqvm, sq_gettop(sqvm) - oldStack);
		return;
	}
	sq_pop(sqvm, sq_gettop(sqvm) - oldStack);
}

static void SqPrintFunc(HSQUIRRELVM v, const SQChar *s, ...){
	va_list vl;
	va_start(vl, s);
	char buf[2048];
	vsprintf_s(buf, s, vl);
	PrintProc(sw, buf);
	va_end(vl);
}

static SQInteger SqError(HSQUIRRELVM v){
	std::stringstream ss;
	const SQChar *desc;
	if(SQ_SUCCEEDED(sq_getstring(v, 2, &desc)))
		ss << "Runtime error: " << desc;
	else
		ss << "Runtime error: Unknown";
	PrintProc(sw, ss.str().c_str());
	return SQInteger(0);
}

static void SqCompilerError(HSQUIRRELVM v, const SQChar *desc, const SQChar *source, SQInteger line, SQInteger column){
	std::stringstream ss;
	ss << "Compile error on " << source << "(" << line << "," << column << "): " << desc;
	PrintProc(sw, ss.str().c_str());
	scripter_adderror(sw, desc, source, line, column);
}

static void OnClose(ScripterWindow *sc){
	scripter_delete(sc);
	PostQuitMessage(0);
}

/// The starting point for this demonstration program.
int main(int argc, char *argv[])
{
	// Fill in the Scripter's config parameters.
	const char *filters = "All (*.*)\0*.*\0Squirrel Scripts (*.nut)\0*.nut";
	ScripterConfig sc;
	sc.commandProc = CmdProc;
	sc.printProc = &PrintProc;
	sc.runProc = RunProc;
	sc.onClose = OnClose;
	sc.sourceFilters = filters;

	sqvm = sq_open(1024);

	// Initialize a Scripter window
	sw = scripter_init(&sc);

	sq_setprintfunc(sqvm, SqPrintFunc, SqPrintFunc);
	sq_newclosure(sqvm, SqError, 0);
	sq_seterrorhandler(sqvm);
	sq_setcompilererrorhandler(sqvm, SqCompilerError);

	// Show the window on screen
	scripter_show(sw);

	// Set the lexer to squirrel
	scripter_lexer_squirrel(sw);

	// Windows message loop
	do{
		MSG msg;
		if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)){
			if(GetMessage(&msg, NULL, 0, 0) <= 0)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}while (true);

	sq_close(sqvm);

	return 0;
}
