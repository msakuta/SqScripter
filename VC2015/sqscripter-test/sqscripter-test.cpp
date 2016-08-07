/** \file 
 * \brief A demonstration program for SqScripter GUI window
 *
 * It does not demonstrate anything like scripting.
 * It just shows how to setup and run the program to enable SqScripter window.
 */


#define NOMINMAX

#include "sqscripter.h"
#include "squirrel.h"
#include "sqstdblob.h"
#include "sqstdio.h"
#include "sqstdmath.h"
#include "sqstdstring.h"
#include "sqstdsystem.h"

extern "C"{
#include "clib/rseq.h"
#include "clib/c.h"
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
	int cost; ///< Used for A* pathfinding algorithm
};

struct RoomPosition{
	int x;
	int y;

	RoomPosition(int x, int y) : x(x), y(y){}
	bool operator==(const RoomPosition& o)const{return x == o.x && y == o.y;}
	bool operator!=(const RoomPosition& o)const{return !(*this == o);}
};

struct PathNode{
	RoomPosition pos;
	int dx;
	int dy;

	PathNode(const RoomPosition& pos, int dx = 0, int dy = 0) : pos(pos), dx(dx), dy(dy){}
};

/// Path has nodes in reverse order of progress, because it's better to truncate from the last
/// element so that former elements are not moved in the memory.
typedef std::vector<PathNode> Path;

struct RoomObject{
	RoomPosition pos;

	RoomObject(int x, int y) : pos(x, y){}
};

struct Creep : public RoomObject{
	Path path;
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
static Creep *selected;
static int global_time = 0;

std::vector<PathNode> findPath(const RoomPosition& from, const RoomPosition& to){
	auto internal = [to](){
		int changes = 0;
		for(int y = 0; y < ROOMSIZE; y++){
			for(int x = 0; x < ROOMSIZE; x++){
				Tile &tile = room[y][x];
				if(tile.cost == INT_MAX)
					continue;
				for(int y1 = std::max(y - 1, 0); y1 <= std::min(y + 1, ROOMSIZE-1); y1++){
					for(int x1 = std::max(x - 1, 0); x1 <= std::min(x + 1, ROOMSIZE-1); x1++){
						Tile &tile1 = room[y1][x1];
						if(tile1.type == 0 && tile.cost + 1 < tile1.cost){
							tile1.cost = tile.cost + 1;
							changes++;
							if(to.x == x1 && to.y == y1)
								return 0;
						}
					}
				}
			}
		}
		return changes;
	};

	for(int y = 0; y < ROOMSIZE; y++){
		for(int x = 0; x < ROOMSIZE; x++){
			Tile &tile = room[y][x];
			tile.cost = (x == from.x && y == from.y ? 0 : INT_MAX);
		}
	}

	std::vector<PathNode> ret;

	while(internal());

	Tile &destTile = room[to.y][to.x];
	if(destTile.cost == INT_MAX)
		return ret;

	// Prefer vertical or horizontal movenent over diagonal even though the costs are the same,
	// by searching preferred positions first.
	static const int deltaPreferences[][2] = {{-1,0},{0,-1},{1,0},{0,1},{-1,-1},{-1,1},{1,-1},{1,1}};

	RoomPosition cur = to;
	while(cur != from){
		Tile &tile = room[cur.y][cur.x];
		int bestx = -1, besty = -1;
		int besti;
		int bestcost = tile.cost;
		for(int i = 0; i < numof(deltaPreferences); i++){
			int y1 = cur.y + deltaPreferences[i][1];
			int x1 = cur.x + deltaPreferences[i][0];
			if(x1 < 0 || ROOMSIZE <= x1 || y1 < 0 || ROOMSIZE <= y1)
				continue;
			Tile &tile1 = room[y1][x1];
			if(tile1.type == 0 && tile1.cost < bestcost){
				bestcost = tile1.cost;
				bestx = x1;
				besty = y1;
				besti = i;
			}
		}
		cur.x = bestx;
		cur.y = besty;
		if(cur != from)
			ret.push_back(PathNode(cur, deltaPreferences[besti][0], deltaPreferences[besti][1]));
	}

	return ret;
}

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
	GLdouble lastTransform[16];
	void Render();
public:
	wxGLCanvasSubClass(wxFrame* parent);
	void Paintit(wxPaintEvent& event);
	void timer(wxTimerEvent& event);
	void OnClick(wxMouseEvent&);
protected:
	DECLARE_EVENT_TABLE()
};

enum{
	ID_SELECT_NAME = 1,
	ID_SELECT_POS = 2,
	ID_TIME
};

BEGIN_EVENT_TABLE(wxGLCanvasSubClass, wxGLCanvas)
EVT_PAINT    (wxGLCanvasSubClass::Paintit)
EVT_TIMER    (0, wxGLCanvasSubClass::timer)
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



void wxGLCanvasSubClass::Paintit(wxPaintEvent& WXUNUSED(event)){
	Render();
}

bool isBlocked(const RoomPosition &pos){
	if(room[pos.y][pos.x].type || pos.x < 0 || ROOMSIZE <= pos.x || pos.y < 0 || ROOMSIZE <= pos.y)
		return true;
	for(auto& it : creeps){
		if(it.pos == pos)
			return true;
	}
	for(auto& it : spawns){
		if(it.pos == pos)
			return true;
	}
	return false;
}

void wxGLCanvasSubClass::timer(wxTimerEvent&){
	for(auto& it : creeps){
		if(it.path.size() == 0){
			Spawn *spawn = [](int owner){
				for(auto& it : spawns)
					if(it.owner == owner)
						return &it;
				return (Spawn*)NULL;
			}(it.owner);
			if(spawn)
				it.path = findPath(it.pos, spawn->pos);
		}
		if(it.path.size()){
			RoomPosition newPos = it.path.back().pos;
			if(isBlocked(newPos))
				continue;
			it.path.pop_back();
			it.pos = newPos;
		}
		else{
			RoomPosition newPos(it.pos.x + rand() % 3 - 1, it.pos.y + rand() % 3 - 1);
			if(isBlocked(newPos))
				continue;
			it.pos = newPos;
		}
	}

	wxStaticText *stc = static_cast<wxStaticText*>(this->GetParent()->GetWindowChild(ID_TIME));
	if(stc){
		wxString str = "Time: " + wxString::Format("%d", global_time);
		stc->SetLabelText(str);
	}

	if(selected){
		stc = static_cast<wxStaticText*>(this->GetParent()->GetWindowChild(ID_SELECT_NAME));
		if(stc){
			wxString str = "Selected: " + wxString::Format("Creep %p", selected);
			stc->SetLabelText(str);
		}

		stc = static_cast<wxStaticText*>(this->GetParent()->GetWindowChild(ID_SELECT_POS));
		if(stc){
			stc->SetLabelText(wxString::Format("Pos: %d, %d", selected->pos.x, selected->pos.y));
		}
	}

	this->Refresh();

	global_time++;
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

	glGetDoublev(GL_MODELVIEW_MATRIX, lastTransform);

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

	for(auto& it : creeps){
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

		if(selected == &it){
			glColor4f(1,1,0,1);
			glLineWidth(3);
			glBegin(GL_LINE_LOOP);
			for(int j = 0; j < 32; j++){
				double angle = j * 2. * M_PI / 32;
				glVertex2d(0.7 * cos(angle) + it.pos.x, .7 * sin(angle) + it.pos.y);
			}
			glEnd();
		}
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

void wxGLCanvasSubClass::OnClick(wxMouseEvent& evt){
	double vx = (double)evt.GetX() / GetSize().x;
	double vy = (double)evt.GetY() / GetSize().y;
	int x = int((vx) * ROOMSIZE);
	int y = int((vy) * ROOMSIZE);
	for(auto& it : creeps){
		if(it.pos.x == x && it.pos.y == y){
			selected = &it;
			break;
		}
	}
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

	const int rightPanelWidth = 300;

	wxFrame *frame = new wxFrame((wxFrame *)NULL, -1,  wxT("Hello GL World"), wxPoint(50,50), wxSize(640+rightPanelWidth,640));

	MyApp &app = wxGetApp();
	app.MyGLCanvas = new wxGLCanvasSubClass(frame);

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
	new wxStaticText(gamePanel, ID_TIME, "", wxPoint(20, 60));

	wxPanel *selectionPanel = new wxPanel(rightPanel, -1);
	selectionPanel->SetForegroundColour(wxColour(255,255,255));
	wxStaticText *selectName = new wxStaticText(selectionPanel, ID_SELECT_NAME, "No Selection", wxPoint(20, 30));
	selectName->SetFont(titleFont);
	selectName->SetBackgroundColour(wxColour(63,63,63));
	new wxStaticText(selectionPanel, ID_SELECT_POS, "", wxPoint(20, 60));

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

