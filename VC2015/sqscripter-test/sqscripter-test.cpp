/** \file 
 * \brief A demonstration program for SqScripter GUI window
 *
 * It does not demonstrate anything like scripting.
 * It just shows how to setup and run the program to enable SqScripter window.
 */

#include "sqscripter.h"
#include "squirrel.h"
#include "sqstdblob.h"
#include "sqstdio.h"
#include "sqstdmath.h"
#include "sqstdstring.h"
#include "sqstdsystem.h"

#include <windows.h>

#include <string>
#include <sstream>


#include <wx/wx.h>
#include <wx/glcanvas.h>

#ifdef __WXMAC__
#include <GLUT/glut.h>
#else
//#include <GL/glut.h>
#endif

#ifndef WIN32
#include <unistd.h> // FIXME: �This work/necessary in Windows?
 //Not necessary, but if it was, it needs to be replaced by process.h AND io.h
#endif



class wxGLCanvasSubClass: public wxGLCanvas {
	void Render();
public:
	wxGLCanvasSubClass(wxFrame* parent);
	void Paintit(wxPaintEvent& event);
protected:
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxGLCanvasSubClass, wxGLCanvas)
EVT_PAINT    (wxGLCanvasSubClass::Paintit)
END_EVENT_TABLE()

wxGLCanvasSubClass::wxGLCanvasSubClass(wxFrame *parent)
	:wxGLCanvas(parent, wxID_ANY, NULL, wxDefaultPosition, wxDefaultSize, 0, wxT("GLCanvas")){
	int argc = 1;
	char* argv[1] = { wxString((wxTheApp->argv)[0]).char_str() };

	/*
	NOTE: this example uses GLUT in order to have a free teapot model
	to display, to show 3D capabilities. GLUT, however, seems to cause problems
	on some systems. If you meet problems, first try commenting out glutInit(),
	then try comeenting out all glut code
	*/
//	glutInit(&argc, argv);
}



void wxGLCanvasSubClass::Paintit(wxPaintEvent& WXUNUSED(event)){
	Render();
}

void wxGLCanvasSubClass::Render()
{
	wxGLContext ctx(this);
	SetCurrent(ctx);
	wxPaintDC(this);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, (GLint)GetSize().x, (GLint)GetSize().y);

	glBegin(GL_POLYGON);
	glColor3f(1.0, 1.0, 1.0);
	glVertex2f(-0.5, -0.5);
	glVertex2f(-0.5, 0.5);
	glVertex2f(0.5, 0.5);
	glVertex2f(0.5, -0.5);
	glColor3f(0.4, 0.5, 0.4);
	glVertex2f(0.0, -0.8);
	glEnd();

	glBegin(GL_POLYGON);
	glColor3f(1.0, 0.0, 0.0);
	glVertex2f(0.1, 0.1);
	glVertex2f(-0.1, 0.1);
	glVertex2f(-0.1, -0.1);
	glVertex2f(0.1, -0.1);
	glEnd();

	// using a little of glut
	glColor4f(0,0,1,1);
//	glutWireTeapot(0.4);

	glLoadIdentity();
	glColor4f(2,0,1,1);
//	glutWireTeapot(0.6);
	// done using glut

	glFlush();
	SwapBuffers();
}

class MyApp: public wxApp
{
	virtual bool OnInit();
	wxGLCanvas * MyGLCanvas;
};


IMPLEMENT_APP(MyApp)


bool MyApp::OnInit()
{
	wxFrame *frame = new wxFrame((wxFrame *)NULL, -1,  wxT("Hello GL World"), wxPoint(50,50), wxSize(200,200));
	new wxGLCanvasSubClass(frame);

	frame->Show(TRUE);
	return TRUE;
}

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
	wchar_t wcmd[256];
	mbstowcs(wcmd, cmd, sizeof wcmd/sizeof*wcmd);
	if(SQ_FAILED(sq_compilebuffer(sqvm, wcmd, wcslen(wcmd), _SC("command"), SQTrue))){
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
	wchar_t wfileName[256];
	mbstowcs(wfileName, fileName, sizeof wfileName/sizeof*wfileName);
	size_t wlen = strlen(content)*4+1;
	std::vector<wchar_t> wcontent(wlen);
	mbstowcs(&wcontent.front(), content, wlen);
	if(SQ_FAILED(sq_compilebuffer(sqvm, &wcontent.front(), wcslen(&wcontent.front()), wfileName, SQTrue))){
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
	char mbs[1024];
	wcstombs(mbs, s, sizeof mbs);
	vsprintf_s(buf, mbs, vl);
	strcat_s(buf, "\n"); // Add line break
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
	char mbdesc[256];
	wcstombs(mbdesc, desc, sizeof mbdesc/sizeof*mbdesc);
	char mbsource[256];
	wcstombs(mbsource, source, sizeof mbsource/sizeof*mbsource);
	scripter_adderror(sw, mbdesc, mbsource, line, column);
}

static void OnClose(ScripterWindow *sc){
	scripter_delete(sc);
	PostQuitMessage(0);
}

/// The starting point for this demonstration program.
int nonmain(int argc, char *argv[])
{
	// Fill in the Scripter's config parameters.
	const char *filters = "All (*.*)|*.*|Squirrel Scripts (*.nut)|*.nut";
	ScripterConfig sc;
	sc.commandProc = CmdProc;
	sc.printProc = &PrintProc;
	sc.runProc = RunProc;
	sc.onClose = OnClose;
	sc.sourceFilters = filters;

	sqvm = sq_open(1024);

	// Initialize a Scripter window
	sw = scripter_init(&sc);
	scripter_set_resource_path(sw, "../..");

	sq_setprintfunc(sqvm, SqPrintFunc, SqPrintFunc);
	sq_newclosure(sqvm, SqError, 0);
	sq_seterrorhandler(sqvm);
	sq_setcompilererrorhandler(sqvm, SqCompilerError);

	// Load standard libraries to the root table
	sq_pushroottable(sqvm);
	sqstd_register_bloblib(sqvm);
	sqstd_register_iolib(sqvm);
	sqstd_register_mathlib(sqvm);
	sqstd_register_stringlib(sqvm);
	sqstd_register_systemlib(sqvm);
	sq_poptop(sqvm);

	// Show the window on screen
	scripter_show(sw);

	// Set the lexer to squirrel
	scripter_lexer_squirrel(sw);

	wxInitializer wxi;

	// Windows message loop
	do{
		MSG msg;
//		if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)){
			if(GetMessage(&msg, NULL, 0, 0) <= 0)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
//		}
	}while (true);

	sq_close(sqvm);

	return 0;
}

