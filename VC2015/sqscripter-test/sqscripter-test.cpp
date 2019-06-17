/** \file 
 * \brief A demonstration program for SqScripter GUI window
 *
 * It does not demonstrate anything like scripting.
 * It just shows how to setup and run the program to enable SqScripter window.
 */


#define NOMINMAX

#include "sqscripter-test.h"
#include "sqscripter.h"
#include "squirrel.h"
#include "sqstdblob.h"
#include "sqstdio.h"
#include "sqstdmath.h"
#include "sqstdstring.h"
#include "sqstdsystem.h"

#include "RoomObject.h"
#include "Creep.h"
#include "Game.h"

extern "C"{
#include "clib/rseq.h"
#include "clib/c.h"
}

#include <windows.h>

#include <string>
#include <sstream>
#include <list>


#ifdef __WXMAC__
#include <GLUT/glut.h>
#else
//#include <GL/glut.h>
#endif

#ifndef WIN32
#include <unistd.h> // FIXME: This work/necessary in Windows?
 //Not necessary, but if it was, it needs to be replaced by process.h AND io.h
#endif






static int bind_wren(int argc, char *argv[]);


/// The Squirrel virtual machine
HSQUIRRELVM sqvm;
static WrenVM* wren;
WrenHandle* whmain;
WrenHandle* whcall;
WrenHandle* whcallMain;






class wxGLCanvasSubClass: public wxGLCanvas {
	GLdouble lastTransform[16];
	void Render();
public:
	wxGLCanvasSubClass(wxFrame* parent);
	void Paintit(wxPaintEvent& event);
	void OnClick(wxMouseEvent&);
protected:
	DECLARE_EVENT_TABLE()
};

enum{
	ID_SELECT_NAME = 1,
	ID_SELECT_POS = 2,
	ID_SELECT_RESOURCE = 3,
	ID_TIME
};

BEGIN_EVENT_TABLE(wxGLCanvasSubClass, wxGLCanvas)
EVT_PAINT    (wxGLCanvasSubClass::Paintit)
EVT_LEFT_DOWN(wxGLCanvasSubClass::OnClick)
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

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_CLOSE    (MainFrame::OnClose)
END_EVENT_TABLE()



BEGIN_EVENT_TABLE(MyApp, wxApp)
EVT_TIMER    (0, MyApp::timer)
END_EVENT_TABLE()

IMPLEMENT_APP(MyApp)





void wxGLCanvasSubClass::Paintit(wxPaintEvent& WXUNUSED(event)){
	Render();
}

void MyApp::timer(wxTimerEvent&){
	{
		wxMutexLocker ml(wxGetApp().mutex);
		SQInteger stack = sq_gettop(sqvm);
		sq_pushroottable(sqvm);
		sq_pushstring(sqvm, _SC("main"), -1);
		if(SQ_SUCCEEDED(sq_get(sqvm, -2))){
			sq_pushroottable(sqvm);
			sq_call(sqvm, 1, SQFalse, SQTrue);
		}
		if(stack < sq_gettop(sqvm))
			sq_pop(sqvm, sq_gettop(sqvm) - stack);

		// We need to be in this scope!
		if(whmain && whcallMain){
			wrenEnsureSlots(wren, 1);
			wrenGetVariable(wren, "main", "Game", 0);
			wrenCall(wren, whcallMain);
		}
	}

	game.update();

	// Separating definition of MyApp::timer and MainFrame::update enables us to run the simulation
	// without windows or graphics, hopefully in the future.
	if(mainFrame)
		mainFrame->update();
}

MainFrame::~MainFrame()
{
	// Clear the dangling pointer to prevent cleanup codes from accidentally dereferencing
	wxGetApp().mainFrame = nullptr;
}

void MainFrame::update(){
	wxStaticText *stc = static_cast<wxStaticText*>(GetWindowChild(ID_TIME));
	if(stc){
		double maxDist = game.visitList.empty() ? 0. : game.visitList.top().dist;
		wxString str = "Time: " + wxString::Format("%d", game.global_time) + wxString::Format(", visitList: %lu", (unsigned long)game.visitList.size()) +
			wxString::Format(", maxDist: %g", maxDist) + "\n" +
			wxString::Format("  Race 0: kills: %d, deaths: %d\n", game.races[0].kills, game.races[0].deaths) +
			wxString::Format("  Race 1: kills: %d, deaths: %d\n", game.races[1].kills, game.races[1].deaths);
		stc->SetLabelText(str);
	}

	if(game.selected){
		auto selected = game.selected;
		stc = static_cast<wxStaticText*>(GetWindowChild(ID_SELECT_NAME));
		if(stc){
			wxString str = "Selected: " + wxString::Format("%s %d", selected->className(), selected->id);
			stc->SetLabelText(str);
		}

		stc = static_cast<wxStaticText*>(GetWindowChild(ID_SELECT_POS));
		if(stc){
			stc->SetLabelText(wxString::Format("Pos: %d, %d", selected->pos.x, selected->pos.y));
		}

		if(stc = static_cast<wxStaticText*>(GetWindowChild(ID_SELECT_RESOURCE))){
			if(Creep* creep = dynamic_cast<Creep*>(selected)){
				stc->SetLabelText(wxString::Format("Resource: %d / %d\nTime to Live: %d\nHealth: %d / %d\nMove Parts: %d",
					creep->resource, creep->max_resource, creep->ttl, creep->health, creep->max_health, creep->moveParts));
			}
			else if(Spawn* spawn = dynamic_cast<Spawn*>(selected)){
				stc->SetLabelText(wxString::Format("Resource: %d", spawn->resource));
			}
			else if(Mine* mine = dynamic_cast<Mine*>(selected)){
				stc->SetLabelText(wxString::Format("Resource: %d", mine->resource));
			}
			else
				stc->SetLabelText("");
		}
	}
	else{
		if(stc = static_cast<wxStaticText*>(GetWindowChild(ID_SELECT_NAME)))
			stc->SetLabelText("No Selection");
		if(stc = static_cast<wxStaticText*>(GetWindowChild(ID_SELECT_POS)))
			stc->SetLabelText("");
		if(stc = static_cast<wxStaticText*>(GetWindowChild(ID_SELECT_RESOURCE)))
			stc->SetLabelText("");
	}

	Refresh();
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
	static const GLfloat taggedColor[4] = { 0.75, 0.75, 0.0, 1 };
	static const GLfloat visitColor[4] = { 0.75, 0.75, 1.0, 1 };
	static const GLfloat creepColors[2][4] = {{0,1,1,1}, {1,0,0,1}};

	glPushMatrix();
	glScaled(2. / ROOMSIZE, 2. / ROOMSIZE, 1);
	glTranslated(-ROOMSIZE / 2 + .5, -ROOMSIZE / 2 + .5, 0);

	glGetDoublev(GL_MODELVIEW_MATRIX, lastTransform);

	glBegin(GL_QUADS);
	for(int i = 0; i < ROOMSIZE; i++){
		for(int j = 0; j < ROOMSIZE; j++){
			GLfloat taggedColor[4] = { game.distanceMap[i][j] / 100., game.distanceMap[i][j] / 100., 0., 1. };
			glColor4fv(/*game.visitList.find(std::array<int, 2>{j, i}) != game.visitList.end() ? visitColor :*/
				game.room[i][j].tag ? taggedColor : game.room[i][j].type ? wallColor : plainColor);
			glVertex2f(-0.5 + j, -0.5 + i);
			glVertex2f(-0.5 + j, 0.5 + i);
			glVertex2f(0.5 + j, 0.5 + i);
			glVertex2f(0.5 + j, -0.5 + i);
		}
	}
	glEnd();

	for(auto& it : game.creeps){
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

		glColor3f(1, 1, 0);
		glBegin(GL_POLYGON);
		double radius = 0.2 * ::sqrt((double)it.resource / it.max_resource); // Convert it to be proportional to area
		for(int j = 0; j < 32; j++){
			double angle = j * 2. * M_PI / 32;
			glVertex2d(radius * cos(angle) + it.pos.x, radius * sin(angle) + it.pos.y);
		}
		glEnd();

		// Path line drawing
		glColor3f(1, 1, 0);
		glLineWidth(1);
		glBegin(GL_LINE_STRIP);
		int pathx = it.pos.x, pathy = it.pos.y;
		for(auto& node : it.path){
			glVertex2d(node.pos.x, node.pos.y);
			pathx -= node.dx;
			pathy -= node.dy;
		}
		glEnd();
	}

	for(auto& it : game.spawns){
		glColor3f(0.8, 0.8, 0.75);
		glBegin(GL_POLYGON);
		double radius = 0.8;
		for(int j = 0; j < 32; j++){
			double angle = j * 2. * M_PI / 32;
			glVertex2d(radius * cos(angle) + it.pos.x, radius * sin(angle) + it.pos.y);
		}
		glEnd();

		glColor3f(0, 0, 0);
		glBegin(GL_POLYGON);
		radius = 0.7;
		for(int j = 0; j < 32; j++){
			double angle = j * 2. * M_PI / 32;
			glVertex2d(radius * cos(angle) + it.pos.x, radius * sin(angle) + it.pos.y);
		}
		glEnd();

		glColor4fv(creepColors[!!it.owner]);
		glBegin(GL_POLYGON);
		radius = .5;
		for(int j = 0; j < 32; j++){
			double angle = j * 2. * M_PI / 32;
			glVertex2d(radius * cos(angle) + it.pos.x, radius * sin(angle) + it.pos.y);
		}
		glEnd();

		glColor3f(1, 1, 0);
		glBegin(GL_POLYGON);
		radius = 0.4 * ::sqrt((double)it.resource / it.max_resource); // Convert it to be proportional to area
		for(int j = 0; j < 32; j++){
			double angle = j * 2. * M_PI / 32;
			glVertex2d(radius * cos(angle) + it.pos.x, radius * sin(angle) + it.pos.y);
		}
		glEnd();
	}

	for(auto& it : game.mines){
		glColor3f(0.8, 0.8, 0.75);
		glBegin(GL_POLYGON);
		double radius = 0.7;
		glColor3f(0, 0, 0);
		glBegin(GL_POLYGON);
		for(int j = 0; j < 4; j++){
			double angle = j * 2. * M_PI / 4;
			glVertex2d(radius * cos(angle) + it.pos.x, radius * sin(angle) + it.pos.y);
		}
		glEnd();

		glColor3f(1, 1, 0);
		glBegin(GL_POLYGON);
		radius = 0.5 * ::sqrt((double)it.resource / it.max_resource); // Convert it to be proportional to area
		for(int j = 0; j < 4; j++){
			double angle = j * 2. * M_PI / 4;
			glVertex2d(radius * cos(angle) + it.pos.x, radius * sin(angle) + it.pos.y);
		}
		glEnd();
	}

	if(game.selected){
		glColor4f(1, 1, 0, 1);
		glLineWidth(3);
		glBegin(GL_LINE_LOOP);
		for(int j = 0; j < 32; j++){
			double angle = j * 2. * M_PI / 32;
			glVertex2d(0.7 * cos(angle) + game.selected->pos.x, .7 * sin(angle) + game.selected->pos.y);
		}
		glEnd();
	}

	// Debug rendering to check if Tile::object is correctly synchronized
#if 0
	glBegin(GL_LINES);
	glColor4f(0,1,0,1);
	for(int i = 0; i < ROOMSIZE; i++){
		for(int j = 0; j < ROOMSIZE; j++){
			if(!room[i][j].object)
				continue;
			glVertex2f(-0.5 + j, -0.5 + i);
			glVertex2f(0.5 + j, 0.5 + i);
		}
	}
	glEnd();
#endif

	glPopMatrix();

	glFlush();
	SwapBuffers();
}

void wxGLCanvasSubClass::OnClick(wxMouseEvent& evt){
	double vx = (double)evt.GetX() / GetSize().x;
	double vy = 1. - (double)evt.GetY() / GetSize().y;
	int x = int((vx) * ROOMSIZE);
	int y = int((vy) * ROOMSIZE);
	for(auto& it : game.creeps){
		if(it.pos.x == x && it.pos.y == y){
			game.selected = &it;
			break;
		}
	}
	for(auto& it : game.spawns){
		if(it.pos.x == x && it.pos.y == y){
			game.selected = &it;
			break;
		}
	}
	for(auto& it : game.mines){
		if(it.pos.x == x && it.pos.y == y){
			game.selected = &it;
			break;
		}
	}
}



void MainFrame::OnClose(wxCloseEvent&){
	wxGetApp().animTimer.Stop();
	Destroy();
}



bool MyApp::OnInit()
{
	srand(5326);

	game.init();

	const int rightPanelWidth = 300;

	MainFrame *frame = new MainFrame((wxFrame *)NULL, -1,  wxT("Hello GL World"), wxPoint(50,50), wxSize(640+rightPanelWidth,640));

	MyApp &app = wxGetApp();
	app.MyGLCanvas = new wxGLCanvasSubClass(frame);
	app.mainFrame = frame;

	wxPanel *rightPanel = new wxPanel(frame, -1, wxPoint(150, 100), wxSize(rightPanelWidth,150));
	rightPanel->SetBackgroundColour(wxColour(47, 47, 47));
	rightPanel->SetForegroundColour(wxColour(255,255,255));

	wxPanel *roomPanel = new wxPanel(rightPanel, -1);
	roomPanel->SetForegroundColour(wxColour(255,255,255));
	roomPanel->SetBackgroundColour(wxColour(63,63,63));
	wxStaticText *roomLabel = new wxStaticText(roomPanel, -1, wxT("ROOM ????"));
	wxFont titleFont = roomLabel->GetFont();
	titleFont.SetPointSize(12);
	titleFont.MakeBold();
	roomLabel->SetFont(titleFont);

	wxPanel *gamePanel = new wxPanel(rightPanel, -1);
	gamePanel->SetForegroundColour(wxColour(255,255,255));
	wxStaticText *gameTitle = new wxStaticText(gamePanel, -1, "Game World", wxPoint(20, 30));
	gameTitle->SetFont(titleFont);
	gameTitle->SetBackgroundColour(wxColour(63,63,63));
	new wxStaticText(gamePanel, ID_TIME, "", wxPoint(20, 60), wxSize(rightPanelWidth, 60));

	wxPanel *selectionPanel = new wxPanel(rightPanel, -1);
	selectionPanel->SetForegroundColour(wxColour(255,255,255));
	wxStaticText *selectName = new wxStaticText(selectionPanel, ID_SELECT_NAME, "No Selection", wxPoint(20, 30));
	selectName->SetFont(titleFont);
	selectName->SetBackgroundColour(wxColour(63,63,63));
	new wxStaticText(selectionPanel, ID_SELECT_POS, "", wxPoint(20, 60));
	new wxStaticText(selectionPanel, ID_SELECT_RESOURCE, "", wxPoint(20, 80), wxSize(rightPanelWidth, 300));

	wxBoxSizer *rightSizer = new wxBoxSizer(wxVERTICAL);
	rightSizer->Add(roomPanel, 0, wxEXPAND);
	rightSizer->Add(gamePanel, 0, wxEXPAND);
	rightSizer->Add(selectionPanel, 0, wxEXPAND);
	rightPanel->SetSizer(rightSizer);

	wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(app.MyGLCanvas, 1, wxEXPAND | wxALL);
	sizer->Add(rightPanel, 0, wxEXPAND | wxRIGHT);
	frame->SetSizer(sizer);

	int nonmain(int argc, char *argv[]);
	nonmain(0, NULL);

	bind_wren(0, NULL);

	frame->Show(TRUE);
	app.animTimer.SetOwner(&app, 0);
	app.animTimer.Start(10);
	return TRUE;
}

int MyApp::OnExit(){
	sq_close(sqvm);
	wrenFreeVM(wren);
	return 0;
}


/// Buffer to hold the function to print something to the console (the lower half of the window).
/// Set by scripter_init().
void (*PrintProc)(ScripterWindow *, const char *) = NULL;

/// ScripterWindow object handle.  It represents single SqScripter window.
ScripterWindow *sw = NULL;


/// Function to handle commands come from the command line (the single line edit at the bottom of the window).
/// "Commands" mean anything, but it would be good to use for interactive scripting like Python or irb
void CmdProc(const char *cmd){
	SQInteger oldStack = sq_gettop(sqvm);
	scripter_clearerror(sw);
	wchar_t wcmd[256];
	mbstowcs(wcmd, cmd, sizeof wcmd/sizeof*wcmd);
	wxMutexLocker ml(wxGetApp().mutex);
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
	const char *p = strrchr(fileName, '.');
	if(p && !strcmp(p, ".nut")){
		SQInteger oldStack = sq_gettop(sqvm);
		scripter_clearerror(sw);
		wchar_t wfileName[256];
		mbstowcs(wfileName, fileName, sizeof wfileName/sizeof*wfileName);
		size_t wlen = strlen(content)*4+1;
		std::vector<wchar_t> wcontent(wlen);
		mbstowcs(&wcontent.front(), content, wlen);
		wxMutexLocker ml(wxGetApp().mutex);
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
	else if(wren){
		wxMutexLocker ml(wxGetApp().mutex);
		wrenInterpret(wren, content);
		whcallMain = wrenMakeCallHandle(wren, "callMain()"); // Reallocate method call handle since it may have changed by calling wrenInterpret
	}
}

static void SqPrintFunc(HSQUIRRELVM v, const SQChar *s, ...){
	va_list vl;
	va_start(vl, s);
	wxString str = wxString::FormatV(s, vl);
	str.append(L"\n");
	PrintProc(sw, str.mbc_str());
	va_end(vl);
}

static SQInteger SqError(HSQUIRRELVM v){
	wxString ss;
	const SQChar *desc;
	if(SQ_SUCCEEDED(sq_getstring(v, 2, &desc)))
		ss << "Runtime error: " << desc;
	else
		ss << "Runtime error: Unknown";
	PrintProc(sw, ss.mbc_str());
	return SQInteger(0);
}

static void SqCompilerError(HSQUIRRELVM v, const SQChar *desc, const SQChar *source, SQInteger line, SQInteger column){
	std::stringstream ss;
#ifdef SQUNICODE
	std::vector<char> csource(scstrlen(source)*4+1);
	wcstombs(csource.data(), source, csource.size());
	std::vector<char> cdesc(scstrlen(desc) * 4 + 1);
	wcstombs(cdesc.data(), desc, cdesc.size());
	ss << "Compile error on " << csource.data() << "(" << line << "," << column << "): " << cdesc.data() << std::endl;
#else
	ss << "Compile error on " << source << "(" << line << "," << column << "): " << desc << std::endl;
#endif
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

	Game::sq_define(sqvm);

	Creep::sq_define(sqvm);

	Spawn::sq_define(sqvm);

	Mine::sq_define(sqvm);

	sq_pushstring(sqvm, _SC("RoomPosition"), -1);
	sq_newclass(sqvm, SQFalse);
	sq_settypetag(sqvm, -1, RoomPosition::typetag);
	sq_setclassudsize(sqvm, -1, sizeof(RoomPosition));
	sq_pushstring(sqvm, _SC("_get"), -1);
	sq_newclosure(sqvm, &RoomPosition::sqf_get, 0);
	sq_newslot(sqvm, -3, SQFalse);
	sq_newslot(sqvm, -3, SQFalse);

	sq_poptop(sqvm);

	// Show the window on screen
	scripter_show(sw);

	// Set the lexer to squirrel
	scripter_lexer_squirrel(sw);

	// Windows message loop
/*	do{
		MSG msg;
//		if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)){
			if(GetMessage(&msg, NULL, 0, 0) <= 0)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
//		}
	}while (true);*/

//	sq_close(sqvm);

	return 0;
}


static void wrenError(WrenVM* vm,
	WrenErrorType type,
	const char* module,
	int line,
	const char* message)
{
	wxString ss;
	const SQChar *desc;
	ss << "Runtime error: " << module << "(" << line << "): " << message << "\n";
	PrintProc(sw, ss.mbc_str());
}

static WrenForeignMethodFn bindForeignMethod(
	WrenVM* vm,
	const char* module,
	const char* className,
	bool isStatic,
	const char* signature)
{
	if(strcmp(module, "main") == 0)
	{
		if(strcmp(className, "Game") == 0)
		{
			return Game::wren_bind(vm, isStatic, signature);
		}
		else if(strcmp(className, "Race") == 0){
			if(!isStatic && !strcmp(signature, "id")){
				return [](WrenVM* vm){
					Game::Race* race = static_cast<Game::Race*>(wrenGetSlotForeign(vm, 0));
					if(race)
						wrenSetSlotDouble(vm, 0, race->id);
				};
			}
			else if(!isStatic && !strcmp(signature, "kills")){
				return [](WrenVM* vm){
					Game::Race* race = static_cast<Game::Race*>(wrenGetSlotForeign(vm, 0));
					if(race)
						wrenSetSlotDouble(vm, 0, race->kills);
				};
			}
			else if(!isStatic && !strcmp(signature, "deaths")){
				return [](WrenVM* vm){
					Game::Race* race = static_cast<Game::Race*>(wrenGetSlotForeign(vm, 0));
					if(race)
						wrenSetSlotDouble(vm, 0, race->deaths);
				};
			}
		}
		else if(strcmp(className, "RoomPosition") == 0){
			if(!isStatic && !strcmp(signature, "x")){
				return [](WrenVM* vm){
					RoomPosition* pos = static_cast<RoomPosition*>(wrenGetSlotForeign(vm, 0));
					if(pos)
						wrenSetSlotDouble(vm, 0, pos->x);
				};
			}
			else if(!isStatic && !strcmp(signature, "y")){
				return [](WrenVM* vm){
					RoomPosition* pos = static_cast<RoomPosition*>(wrenGetSlotForeign(vm, 0));
					if(pos)
						wrenSetSlotDouble(vm, 0, pos->y);
				};
			}
		}
		else if(strcmp(className, "Creep") == 0)
		{
			return Creep::wren_bind(vm, isStatic, signature);
		}
		else if(strcmp(className, "Spawn") == 0)
		{
			return Spawn::wren_bind(vm, isStatic, signature);
		}
		else if(strcmp(className, "Mine") == 0)
		{
			return Mine::wren_bind(vm, isStatic, signature);
		}
	}
	// Other modules...
	return nullptr;
}

template<typename T>
static void genericAllocate(WrenVM *vm){
	WeakPtr<T>* pp = (WeakPtr<T>*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WeakPtr<T>));
}

template<typename T>
static void genericFinalize(void *pv){
	WeakPtr<T>* pp = (WeakPtr<T>*)pv;
	pp->~WeakPtr<T>();
}

WrenForeignClassMethods bindForeignClass(
	WrenVM* vm, const char* module, const char* className)
{
	WrenForeignClassMethods methods;

	if(strcmp(className, "Creep") == 0){
		methods.allocate = genericAllocate<Creep>;
		methods.finalize = genericFinalize<Creep>;
	}
	else if(strcmp(className, "Spawn") == 0){
		methods.allocate = genericAllocate<Spawn>;
		methods.finalize = genericFinalize<Spawn>;
	}
	else if(strcmp(className, "Mine") == 0){
		methods.allocate = genericAllocate<Mine>;
		methods.finalize = genericFinalize<Mine>;
	}
	else
	{
		// Unknown class.
		methods.allocate = NULL;
		methods.finalize = NULL;
	}

	return methods;
}

/// The starting point for this demonstration program.
int bind_wren(int argc, char *argv[])
{
	WrenConfiguration config;
	wrenInitConfiguration(&config);
	config.bindForeignMethodFn = bindForeignMethod;
	config.bindForeignClassFn = bindForeignClass;
	config.errorFn = wrenError;

	wren = wrenNewVM(&config);

	wrenInterpret(wren, ""
	"foreign class Race{\n"
	"	foreign id\n"
	"	foreign kills\n"
	"	foreign deaths\n"
	"}\n"
	"class Game{\n"
	"	foreign static print(s)\n"
	"	foreign static time\n"
	"	foreign static creep(i)\n"
	"	foreign static creeps\n"
	"	foreign static spawns\n"
	"	foreign static mines\n"
	"	foreign static races\n"
	"	static main { __main }\n"
	"	static main=(value) { __main = value }\n"
	"	static callMain() {\n"
	"		if(!(__main is Null)) __main.call()\n"
	"	}\n"
	"}\n"
	"foreign class RoomPosition{\n"
	"	foreign x\n"
	"	foreign y\n"
	"}\n"
	"foreign class Creep{\n"
	"	foreign alive\n"
	"	foreign pos\n"
	"	foreign move(i)\n"
	"	foreign harvest(i)\n"
	"	foreign store(i)\n"
	"	foreign attack(i)\n"
	"	foreign findPath(pos)\n"
	"	foreign followPath()\n"
	"	foreign id\n"
	"	foreign owner\n"
	"	foreign ttl\n"
	"	foreign resource\n"
	"	foreign moveParts\n"
	"	foreign memory\n"
	"	foreign memory=(newMemory)\n"
	"}\n"
	"foreign class Spawn{\n"
	"	foreign alive\n"
	"	foreign pos\n"
	"	createCreep(){\n"
	"		return createCreep(1)\n"
	"	}\n"
	"	foreign createCreep(moveParts)\n"
	"	foreign id\n"
	"	foreign owner\n"
	"	foreign resource\n"
	"	foreign static creep_cost\n"
	"	foreign static move_part_cost\n"
	"}\n"
	"foreign class Mine{\n"
	"	foreign alive\n"
	"	foreign pos\n"
	"	foreign id\n"
	"	foreign resource\n"
	"}\n");

	whmain = wrenMakeCallHandle(wren, "main");
	whcall = wrenMakeCallHandle(wren, "call()");
	whcallMain = wrenMakeCallHandle(wren, "callMain()");

	return 0;
}
