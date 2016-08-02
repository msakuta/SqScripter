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

extern "C"{
#include "clib/rseq.h"
}

#include <windows.h>

#include <string>
#include <sstream>
#include <list>


#include <wx/wx.h>
#include <wx/glcanvas.h>

#ifdef __WXMAC__
#include <GLUT/glut.h>
#else
//#include <GL/glut.h>
#endif

#ifndef WIN32
#include <unistd.h> // FIXME: This work/necessary in Windows?
 //Not necessary, but if it was, it needs to be replaced by process.h AND io.h
#endif


struct Tile{
	int type;
};

struct RoomPosition{
	int x;
	int y;

	RoomPosition(int x, int y) : x(x), y(y){}
};

struct RoomObject{
	RoomPosition pos;

	RoomObject(int x, int y) : pos(x, y){}
};

struct Creep : public RoomObject{
	int owner;

	Creep(int x, int y, int owner) : RoomObject(x, y), owner(owner){}
};

struct Spawn : public RoomObject{
	int owner;

	Spawn(int x, int y, int owner) : RoomObject(x, y), owner(owner){}
};

const int ROOMSIZE = 50;

static Tile room[ROOMSIZE][ROOMSIZE] = {0};
static std::list<Creep> creeps;
static std::list<Spawn> spawns;

static double noise_pixel(int x, int y, int bit){
	struct random_sequence rs;
	initfull_rseq(&rs, x + (bit << 16), y);
	return drseq(&rs);
}

double perlin_noise_pixel(int x, int y, int bit){
	int ret = 0, i;
	double sum = 0., maxv = 0., f = 1.;
	double persistence = 0.5;
	for(i = 3; 0 <= i; i--){
		int cell = 1 << i;
		double a00, a01, a10, a11, fx, fy;
		a00 = noise_pixel(x / cell, y / cell, bit);
		a01 = noise_pixel(x / cell, y / cell + 1, bit);
		a10 = noise_pixel(x / cell + 1, y / cell, bit);
		a11 = noise_pixel(x / cell + 1, y / cell + 1, bit);
		fx = (double)(x % cell) / cell;
		fy = (double)(y % cell) / cell;
		sum += ((a00 * (1. - fx) + a10 * fx) * (1. - fy)
			+ (a01 * (1. - fx) + a11 * fx) * fy) * f;
		maxv += f;
		f *= persistence;
	}
	return sum / maxv;
}




class wxGLCanvasSubClass: public wxGLCanvas {
	void Render();
public:
	wxGLCanvasSubClass(wxFrame* parent);
	void Paintit(wxPaintEvent& event);
	void timer(wxTimerEvent& event);
protected:
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxGLCanvasSubClass, wxGLCanvas)
EVT_PAINT    (wxGLCanvasSubClass::Paintit)
EVT_TIMER    (0, wxGLCanvasSubClass::timer)
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

void wxGLCanvasSubClass::timer(wxTimerEvent&){
	for(auto& it : creeps){
		RoomPosition newPos(it.pos.x + rand() % 3 - 1, it.pos.y + rand() % 3 - 1);
		if(room[newPos.y][newPos.x].type || newPos.x < 0 || ROOMSIZE <= newPos.x || newPos.y < 0 || ROOMSIZE <= newPos.y)
			continue;
		it.pos = newPos;
	}
	this->Refresh();
}

void wxGLCanvasSubClass::Render()
{
	wxGLContext ctx(this);
	SetCurrent(ctx);
	wxPaintDC(this);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, (GLint)GetSize().x, (GLint)GetSize().y);

	static const GLfloat plainColor[4] = {0.5, 0.5, 0.5, 1};
	static const GLfloat wallColor[4] = {0, 0, 0, 1};
	static const GLfloat creepColors[2][4] = {{0,1,1,1}, {1,0,0,1}};

	glPushMatrix();
	glScaled(2. / ROOMSIZE, 2. / ROOMSIZE, 1);
	glTranslated(-ROOMSIZE / 2 + .5, -ROOMSIZE / 2 + .5, 0);
	glBegin(GL_QUADS);
	for(int i = 0; i < ROOMSIZE; i++){
		for(int j = 0; j < ROOMSIZE; j++){
			glColor4fv(room[i][j].type ? wallColor : plainColor);
			glVertex2f(-0.5 + j, -0.5 + i);
			glVertex2f(-0.5 + j, 0.5 + i);
			glVertex2f(0.5 + j, 0.5 + i);
			glVertex2f(0.5 + j, -0.5 + i);
		}
	}
	glEnd();

	for(auto it : creeps){
		glBegin(GL_POLYGON);
		glColor3f(0, 0, 0);
		for(int j = 0; j < 32; j++){
			double angle = j * 2. * M_PI / 32;
			glVertex2d(.5 * cos(angle) + it.pos.x, .5 * sin(angle) + it.pos.y);
		}
		glEnd();

		glBegin(GL_POLYGON);
		glColor4fv(creepColors[!!it.owner]);
		for(int j = 0; j < 32; j++){
			double angle = j * 2. * M_PI / 32;
			glVertex2d(.3 * cos(angle) + it.pos.x, .3 * sin(angle) + it.pos.y);
		}
		glEnd();
	}

	for(auto it : spawns){
		glColor3f(0.8, 0.8, 0.75);
		glBegin(GL_POLYGON);
		for(int j = 0; j < 32; j++){
			double angle = j * 2. * M_PI / 32;
			glVertex2d(.7 * cos(angle) + it.pos.x, .7 * sin(angle) + it.pos.y);
		}
		glEnd();

		glColor3f(0, 0, 0);
		glBegin(GL_POLYGON);
		for(int j = 0; j < 32; j++){
			double angle = j * 2. * M_PI / 32;
			glVertex2d(.6 * cos(angle) + it.pos.x, .6 * sin(angle) + it.pos.y);
		}
		glEnd();

		glColor4fv(creepColors[!!it.owner]);
		glBegin(GL_POLYGON);
		for(int j = 0; j < 32; j++){
			double angle = j * 2. * M_PI / 32;
			glVertex2d(.3 * cos(angle) + it.pos.x, .3 * sin(angle) + it.pos.y);
		}
		glEnd();
	}
	glPopMatrix();

	glFlush();
	SwapBuffers();
}

class MyApp: public wxApp
{
	virtual bool OnInit();
	wxGLCanvas * MyGLCanvas;
	wxTimer animTimer;
};


IMPLEMENT_APP(MyApp)




bool MyApp::OnInit()
{
	for(int i = 0; i < ROOMSIZE; i++){
		int di = i - ROOMSIZE / 2;
		for(size_t j = 0; j < ROOMSIZE; j++){
			int dj = j - ROOMSIZE / 2;
			room[i][j].type = perlin_noise_pixel(j, i, 3) < 1. - 1. / (1. + sqrt(di * di + dj * dj) / (ROOMSIZE / 2));
		}
	}

	for(int i = 0; i < 20; i++){
		int x, y;
		do{
			x = rand() % ROOMSIZE;
			y = rand() % ROOMSIZE;
		}while(room[y][x].type != 0);
		creeps.push_back(Creep(x, y, i % 2));
	}

	for(int i = 0; i < 2; i++){
		int x, y;
		do{
			x = rand() % ROOMSIZE;
			y = rand() % ROOMSIZE;
		}while(room[y][x].type != 0);
		spawns.push_back(Spawn(x, y, i));
	}

	wxFrame *frame = new wxFrame((wxFrame *)NULL, -1,  wxT("Hello GL World"), wxPoint(50,50), wxSize(640,640));

	MyApp &app = wxGetApp();
	app.MyGLCanvas = new wxGLCanvasSubClass(frame);

	frame->Show(TRUE);
	app.animTimer.SetOwner(wxGetApp().MyGLCanvas, 0);
	app.animTimer.Start(100);
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

