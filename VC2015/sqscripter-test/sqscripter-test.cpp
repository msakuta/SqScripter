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

#include "wren/src/include/wren.hpp"

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

	static SQInteger sqf_get(HSQUIRRELVM v);
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

enum Direction{
	TOP = 1,
	TOP_RIGHT = 2,
	RIGHT = 3,
	BOTTOM_RIGHT = 4,
	BOTTOM = 5,
	BOTTOM_LEFT = 6,
	LEFT = 7,
	TOP_LEFT = 8,
};

struct Creep : public RoomObject{
	Path path;
	int owner;
	int id;
	static int id_gen;

	Creep(int x, int y, int owner) : RoomObject(x, y), owner(owner), id(id_gen++){}
	static SQInteger sqf_get(HSQUIRRELVM v);
	static SQInteger sqf_move(HSQUIRRELVM v);

	static WrenForeignMethodFn wren_bind(WrenVM* vm, bool isStatic, const char* signature);
};

struct Spawn : public RoomObject{
	typedef Spawn tt;

	int owner;
	int id;
	static int id_gen;

	static const SQUserPointer typetag;

	Spawn(int x, int y, int owner) : RoomObject(x, y), owner(owner), id(id_gen++){}

	bool createCreep();

	static SQInteger sqf_createCreep(HSQUIRRELVM v);

	static SQInteger sqf_get(HSQUIRRELVM v);

	static HSQOBJECT spawnClass;

	static void sq_define(HSQUIRRELVM v){
		sq_pushstring(v, _SC("Spawn"), -1);
		sq_newclass(v, SQFalse);
		sq_settypetag(v, -1, typetag);
		sq_pushstring(v, _SC("createCreep"), -1);
		sq_newclosure(v, &sqf_createCreep, 0);
		sq_newslot(v, -3, SQFalse);
		sq_pushstring(v, _SC("_get"), -1);
		sq_newclosure(v, &Creep::sqf_get, 0);
		sq_newslot(v, -3, SQFalse);
		sq_getstackobj(v, -1, &spawnClass);
		sq_addref(v, &spawnClass);
		sq_newslot(v, -3, SQFalse);
	}

	static WrenForeignMethodFn wren_bind(WrenVM* vm, bool isStatic, const char* signature);
};

int Spawn::id_gen = 0;
const SQUserPointer Spawn::typetag = _SC("Spawn");
SQObject Spawn::spawnClass;

const int ROOMSIZE = 50;

static int bind_wren(int argc, char *argv[]);

static Tile room[ROOMSIZE][ROOMSIZE] = {0};
static std::list<Creep> creeps;
static std::list<Spawn> spawns;
static Creep *selected;
static int global_time = 0;
/// The Squirrel virtual machine
static HSQUIRRELVM sqvm;
static WrenVM* wren;
WrenHandle* whmain;
WrenHandle* whcall;
WrenHandle* whcallMain;

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

class MainFrame : public wxFrame{
	void OnClose(wxCloseEvent&);
public:
	using wxFrame::wxFrame;
	~MainFrame()override;
	void update();

protected:
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_CLOSE    (MainFrame::OnClose)
END_EVENT_TABLE()



class MyApp: public wxApp
{
	bool OnInit()override;
	int OnExit()override{
		sq_close(sqvm);
		wrenFreeVM(wren);
		return 0;
	}
	wxGLCanvas * MyGLCanvas;
public:
	MainFrame* mainFrame;
	wxTimer animTimer;
	wxMutex mutex;
	void timer(wxTimerEvent&);
protected:
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(MyApp, wxApp)
EVT_TIMER    (0, MyApp::timer)
END_EVENT_TABLE()

IMPLEMENT_APP(MyApp)

SQInteger RoomPosition::sqf_get(HSQUIRRELVM v)
{
	SQUserPointer up;
	if(SQ_FAILED(sq_getinstanceup(v, 1, &up, nullptr)))
		return sq_throwerror(v, _SC("Invalid this pointer"));
	RoomPosition *pos = static_cast<RoomPosition*>(up);
	const SQChar *key;
	if(SQ_FAILED(sq_getstring(v, 2, &key)))
		return sq_throwerror(v, _SC("Broken key in _get"));
	if(!scstrcmp(key, _SC("x"))){
		sq_pushinteger(v, pos->x);
		return 1;
	}	
	else if(!scstrcmp(key, _SC("y"))){
		sq_pushinteger(v, pos->y);
		return 1;
	}	
	else
		return sq_throwerror(v, _SC("Couldn't find key"));
}

int Creep::id_gen = 0;

SQInteger Creep::sqf_get(HSQUIRRELVM v){
	wxMutexLocker ml(wxGetApp().mutex);
	SQUserPointer up;
	if(SQ_FAILED(sq_getinstanceup(v, 1, &up, nullptr)))
		return sq_throwerror(v, _SC("Invalid this pointer"));
	Creep *creep = static_cast<Creep*>(up);
	const SQChar *key;
	if(SQ_FAILED(sq_getstring(v, 2, &key)))
		return sq_throwerror(v, _SC("Broken key in _get"));
	if(!scstrcmp(key, _SC("pos"))){
		sq_pushroottable(v);
		sq_pushstring(v, _SC("RoomPosition"), -1);
		if(SQ_FAILED(sq_get(v, -2)))
			return sq_throwerror(v, _SC("Can't find RoomPosition class definition"));
		sq_createinstance(v, -1);
		sq_getinstanceup(v, -1, &up, nullptr);
		RoomPosition *rp = static_cast<RoomPosition*>(up);
		*rp = creep->pos;
		return 1;
	}
	else if(!scstrcmp(key, _SC("id"))){
		sq_pushroottable(v);
		sq_pushinteger(v, creep->id);
		return 1;
	}
	else if(!scstrcmp(key, _SC("owner"))){
		sq_pushroottable(v);
		sq_pushinteger(v, creep->owner);
		return 1;
	}
	else
		return sq_throwerror(v, _SC("Couldn't find key"));
}

SQInteger Creep::sqf_move(HSQUIRRELVM v){
	wxMutexLocker ml(wxGetApp().mutex);
	SQUserPointer p;
	if(SQ_FAILED(sq_getinstanceup(v, 1, &p, nullptr)))
		return sq_throwerror(v, _SC("Broken Creep instance"));
	SQInteger i;
	if(SQ_FAILED(sq_getinteger(v, 2, &i)) || i < TOP || TOP_LEFT < i)
		return sq_throwerror(v, _SC("Invalid Creep.move argument"));
	static int deltas[8][2] = {
		{0,-1}, // TOP = 1,
		{1,-1}, // TOP_RIGHT = 2,
		{1,0}, // RIGHT = 3,
		{1,1}, // BOTTOM_RIGHT = 4,
		{0,1}, // BOTTOM = 5,
		{-1,1}, // BOTTOM_LEFT = 6,
		{-1,0}, // LEFT = 7,
		{-1,-1}, // TOP_LEFT = 8,
	};
	Creep *creep = static_cast<Creep*>(p);
	creep->pos.x += deltas[i-1][0];
	creep->pos.y += deltas[i-1][1];
	return 0;
}

WrenForeignMethodFn Creep::wren_bind(WrenVM * vm, bool isStatic, const char * signature)
{
	if(!isStatic && !strcmp(signature, "pos")){
		return [](WrenVM* vm){
			Creep** pp = (Creep**)wrenGetSlotForeign(vm, 0);
			if(!pp)
				return;
			Creep *creep = *pp;
			wrenEnsureSlots(vm, 2);
			wrenSetSlotNewList(vm, 0);
			wrenSetSlotDouble(vm, 1, creep->pos.x);
			wrenInsertInList(vm, 0, -1, 1);
			wrenSetSlotDouble(vm, 1, creep->pos.y);
			wrenInsertInList(vm, 0, -1, 1);
		};
	}
	else if(!isStatic && !strcmp(signature, "id")){
		return [](WrenVM* vm){
			Creep** pp = (Creep**)wrenGetSlotForeign(vm, 0);
			if(!pp)
				return;
			Creep *creep = *pp;
			wrenEnsureSlots(vm, 1);
			wrenSetSlotDouble(vm, 0, creep->id);
		};
	}
	else if(!isStatic && !strcmp(signature, "owner")){
		return [](WrenVM* vm){
			Creep** pp = (Creep**)wrenGetSlotForeign(vm, 0);
			if(!pp)
				return;
			Creep *creep = *pp;
			wrenEnsureSlots(vm, 1);
			wrenSetSlotDouble(vm, 0, creep->owner);
		};
	}
	else if(!isStatic && !strcmp(signature, "move(_)")){
		return [](WrenVM* vm){
			wxMutexLocker ml(wxGetApp().mutex);
			Creep** pp = (Creep**)wrenGetSlotForeign(vm, 0);
			if(!pp)
				return;
			Creep *creep = *pp;
			if(WREN_TYPE_NUM != wrenGetSlotType(vm, 1)){
				return;
			}
			int i = (int)wrenGetSlotDouble(vm, 1);
			static int deltas[8][2] = {
				{ 0,-1 }, // TOP = 1,
				{ 1,-1 }, // TOP_RIGHT = 2,
				{ 1,0 }, // RIGHT = 3,
				{ 1,1 }, // BOTTOM_RIGHT = 4,
				{ 0,1 }, // BOTTOM = 5,
				{ -1,1 }, // BOTTOM_LEFT = 6,
				{ -1,0 }, // LEFT = 7,
				{ -1,-1 }, // TOP_LEFT = 8,
			};
			creep->pos.x += deltas[i - 1][0];
			creep->pos.y += deltas[i - 1][1];
		};
	}
	return nullptr;
}

SQInteger Spawn::sqf_get(HSQUIRRELVM v){
	wxMutexLocker ml(wxGetApp().mutex);
	SQUserPointer up;
	if(SQ_FAILED(sq_getinstanceup(v, 1, &up, nullptr)))
		return sq_throwerror(v, _SC("Invalid this pointer for Spawn"));
	Spawn *spawn = static_cast<Spawn*>(up);
	const SQChar *key;
	if(SQ_FAILED(sq_getstring(v, 2, &key)))
		return sq_throwerror(v, _SC("Broken key in _get"));
	if(!scstrcmp(key, _SC("pos"))){
		sq_pushroottable(v);
		sq_pushstring(v, _SC("RoomPosition"), -1);
		if(SQ_FAILED(sq_get(v, -2)))
			return sq_throwerror(v, _SC("Can't find RoomPosition class definition"));
		sq_createinstance(v, -1);
		sq_getinstanceup(v, -1, &up, nullptr);
		RoomPosition *rp = static_cast<RoomPosition*>(up);
		*rp = spawn->pos;
		return 1;
	}
	else
		return sq_throwerror(v, _SC("Couldn't find key"));
}

WrenForeignMethodFn Spawn::wren_bind(WrenVM * vm, bool isStatic, const char * signature)
{
	if(!isStatic && !strcmp(signature, "pos")){
		return [](WrenVM* vm){
			tt** pp = (tt**)wrenGetSlotForeign(vm, 0);
			if(!pp)
				return;
			tt *spawn = *pp;
			wrenEnsureSlots(vm, 2);
			wrenSetSlotNewList(vm, 0);
			wrenSetSlotDouble(vm, 1, spawn->pos.x);
			wrenInsertInList(vm, 0, -1, 1);
			wrenSetSlotDouble(vm, 1, spawn->pos.y);
			wrenInsertInList(vm, 0, -1, 1);
		};
	}
	else if(!isStatic && !strcmp(signature, "id")){
		return [](WrenVM* vm){
			tt** pp = (tt**)wrenGetSlotForeign(vm, 0);
			if(!pp)
				return;
			tt *creep = *pp;
			wrenEnsureSlots(vm, 1);
			wrenSetSlotDouble(vm, 0, creep->id);
		};
	}
	else if(!isStatic && !strcmp(signature, "owner")){
		return [](WrenVM* vm){
			tt** pp = (tt**)wrenGetSlotForeign(vm, 0);
			if(!pp)
				return;
			tt *creep = *pp;
			wrenEnsureSlots(vm, 1);
			wrenSetSlotDouble(vm, 0, creep->owner);
		};
	}
	else if(!isStatic && !strcmp(signature, "createCreep()")){
		return [](WrenVM* vm){
			wxMutexLocker ml(wxGetApp().mutex);
			tt** pp = (tt**)wrenGetSlotForeign(vm, 0);
			if(!pp)
				return;
			tt *spawn = *pp;
			wrenSetSlotBool(vm, 0, spawn->createCreep());
		};
	}
	return WrenForeignMethodFn();
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

	for(auto& it : creeps){
/*		if(it.path.size() == 0){
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
		}*/
	}

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
		wxString str = "Time: " + wxString::Format("%d", global_time);
		stc->SetLabelText(str);
	}

	if(selected){
		stc = static_cast<wxStaticText*>(GetWindowChild(ID_SELECT_NAME));
		if(stc){
			wxString str = "Selected: " + wxString::Format("Creep %p", selected);
			stc->SetLabelText(str);
		}

		stc = static_cast<wxStaticText*>(GetWindowChild(ID_SELECT_POS));
		if(stc){
			stc->SetLabelText(wxString::Format("Pos: %d, %d", selected->pos.x, selected->pos.y));
		}
	}

	Refresh();

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
	double vy = 1. - (double)evt.GetY() / GetSize().y;
	int x = int((vx) * ROOMSIZE);
	int y = int((vy) * ROOMSIZE);
	for(auto& it : creeps){
		if(it.pos.x == x && it.pos.y == y){
			selected = &it;
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
	for(int i = 0; i < ROOMSIZE; i++){
		int di = i - ROOMSIZE / 2;
		for(size_t j = 0; j < ROOMSIZE; j++){
			int dj = j - ROOMSIZE / 2;
			room[i][j].type = perlin_noise_pixel(j, i, 3) < 1. - 1. / (1. + sqrt(di * di + dj * dj) / (ROOMSIZE / 2));
		}
	}

	for(int i = 0; i < 6; i++){
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

	bind_wren(0, NULL);

	frame->Show(TRUE);
	app.animTimer.SetOwner(&app, 0);
	app.animTimer.Start(100);
	return TRUE;
}

/// Buffer to hold the function to print something to the console (the lower half of the window).
/// Set by scripter_init().
static void (*PrintProc)(ScripterWindow *, const char *) = NULL;

/// ScripterWindow object handle.  It represents single SqScripter window.
static ScripterWindow *sw = NULL;


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

struct Game{
	static SQInteger sqf_get(HSQUIRRELVM v){
		wxMutexLocker ml(wxGetApp().mutex);
		const SQChar *key;
		if(SQ_FAILED(sq_getstring(v, -1, &key)))
			return sq_throwerror(v, _SC("get key is missing"));
		if(!scstrcmp(key, _SC("time"))){
			sq_pushinteger(v, global_time);
			return 1;
		}
		else if(!scstrcmp(key, _SC("creeps"))){
			size_t sz = creeps.size();
			SQInteger i = 0;
			sq_newarray(v, sz);
			for(auto& it : creeps){
				sq_pushroottable(v); // root
				sq_pushstring(v, _SC("Creep"), -1); // root "Creep"
				if(SQ_FAILED(sq_get(v, -2))) // root Creep i
					return sq_throwerror(v, _SC("Failed to create creeps array"));
				sq_pushinteger(v, i++); // root Creep i
				sq_createinstance(v, -2); // root Creep i inst
				sq_setinstanceup(v, -1, &it);
				if(SQ_FAILED(sq_set(v, -5))) // root Creep
					return sq_throwerror(v, _SC("Failed to create creeps array"));
				sq_pop(v, 2);
			}
			return 1;
		}
		else if(!scstrcmp(key, _SC("spawns"))){
			size_t sz = spawns.size();
			SQInteger i = 0;
			sq_newarray(v, sz); // array
			for(auto& it : spawns){
				sq_pushobject(v, Spawn::spawnClass); // array Spawn
				sq_pushinteger(v, i++); // array Spawn i
				sq_createinstance(v, -2); // array Spawn i inst
				sq_setinstanceup(v, -1, &it); // array Spawn i inst
				if(SQ_FAILED(sq_set(v, -4))) // array Spawn
					return sq_throwerror(v, _SC("Failed to create creeps array"));
				sq_pop(v, 1); // array
			}
			return 1;
		}
		else
			return sq_throwerror(v, _SC("Index not found"));
	}
};

static Game game;

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
	sq_pushstring(sqvm, _SC("Game"), -1);
		sq_newclass(sqvm, SQFalse);
		sq_settypetag(sqvm, -1, "Game");
		sq_pushstring(sqvm, _SC("_get"), -1);
		sq_newclosure(sqvm, &Game::sqf_get, 0);
		sq_newslot(sqvm, -3, SQTrue);
	// Register the singleton instance instead of the class
	// because _get() method only works for instances.
	sq_createinstance(sqvm, -1);
	sq_remove(sqvm, -2);
	sq_newslot(sqvm, -3, SQFalse);

	sq_pushstring(sqvm, _SC("Creep"), -1);
	sq_newclass(sqvm, SQFalse);
	sq_settypetag(sqvm, -1, _SC("Creep"));
	sq_pushstring(sqvm, _SC("move"), -1);
	sq_newclosure(sqvm, &Creep::sqf_move, 0);
	sq_newslot(sqvm, -3, SQFalse);
	sq_pushstring(sqvm, _SC("_get"), -1);
	sq_newclosure(sqvm, &Creep::sqf_get, 0);
	sq_newslot(sqvm, -3, SQFalse);
	sq_newslot(sqvm, -3, SQFalse);

	Spawn::sq_define(sqvm);

	sq_pushstring(sqvm, _SC("RoomPosition"), -1);
	sq_newclass(sqvm, SQFalse);
	sq_settypetag(sqvm, -1, _SC("RoomPosition"));
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

bool Spawn::createCreep(){
	static const int directions[4][2] = {
		{0,-1},
		{ 1,0 },
		{0,1},
		{-1,0}
	};
	int i = 0;
	do{
		RoomPosition newPos{pos.x + directions[i][0], pos.y + directions[i][1]};
		if(isBlocked(newPos))
			continue;
		creeps.push_back(Creep(newPos.x, newPos.y, owner));
		return true;
	}while(++i < numof(directions));
	return false;
}

SQInteger Spawn::sqf_createCreep(HSQUIRRELVM v)
{
	SQUserPointer up;
	if(SQ_FAILED(sq_getinstanceup(v, 1, &up, static_cast<SQUserPointer>(typetag))))
		return sq_throwerror(v, _SC("Invalid this pointer for a spawn"));
	Spawn *spawn = static_cast<Spawn*>(up);
	if(!up || !spawn)
		return sq_throwerror(v, _SC("Invalid this pointer for a spawn"));
	SQBool b = spawn->createCreep();
	sq_pushbool(v, b);
	return 1;
}

static void wrenPrint(WrenVM* vm)
{
	if(WREN_TYPE_STRING != wrenGetSlotType(vm, 1)){
		PrintProc(sw, "Game.print() needs a string argument");
		return;
	}
	const char *s = wrenGetSlotString(vm, 1);
	PrintProc(sw, s);
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

static void wrenGetTime(WrenVM* vm)
{
	wrenEnsureSlots(vm, 1);
	wrenSetSlotDouble(vm, 0, global_time);
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
			if(isStatic && strcmp(signature, "print(_)") == 0)
			{
				return wrenPrint;
			}
			else if(isStatic && strcmp(signature, "time") == 0)
			{
				return wrenGetTime;
			}
			else if(isStatic && strcmp(signature, "creep(_)") == 0)
			{
				return [](WrenVM* vm){
					if(WREN_TYPE_NUM != wrenGetSlotType(vm, 1)){
						return;
					}
					int idx = (int)wrenGetSlotDouble(vm, 1);
					if(idx < 0 || creeps.size() <= idx){
						return;
					}
					wrenEnsureSlots(vm, 2);
					wrenGetVariable(vm, "main", "Creep", 1);
					void *pp = wrenSetSlotNewForeign(vm, 0, 1, sizeof(Creep*));
					auto cp = creeps.begin();
					for(int i = 0; i < idx && cp != creeps.end(); ++cp, ++i);
					*(Creep**)pp = &*cp;
				};
			}
			else if(isStatic && strcmp(signature, "creeps") == 0)
			{
				return [](WrenVM* vm){
					// Slots: [array] main.Creep [instance]
					wrenEnsureSlots(vm, 3);
					wrenSetSlotNewList(vm, 0);
					wrenGetVariable(vm, "main", "Creep", 1);
					for(auto it = creeps.begin(); it != creeps.end(); ++it){
						void *pp = wrenSetSlotNewForeign(vm, 2, 1, sizeof(Creep*));
						*(Creep**)pp = &*it;
						wrenInsertInList(vm, 0, -1, 2);
					}
				};
			}
			else if(isStatic && strcmp(signature, "spawns") == 0){
				return [](WrenVM* vm){
					// Slots: [array] main.Creep [instance]
					wrenEnsureSlots(vm, 3);
					wrenSetSlotNewList(vm, 0);
					wrenGetVariable(vm, "main", "Spawn", 1);
					for(auto it = spawns.begin(); it != spawns.end(); ++it){
						void *pp = wrenSetSlotNewForeign(vm, 2, 1, sizeof(Spawn*));
						*(Spawn**)pp = &*it;
						wrenInsertInList(vm, 0, -1, 2);
					}
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
	}
	// Other modules...
	return nullptr;
}

WrenForeignClassMethods bindForeignClass(
	WrenVM* vm, const char* module, const char* className)
{
	WrenForeignClassMethods methods;

	if(strcmp(className, "Creep") == 0)
	{
		methods.allocate = [](WrenVM* vm){
			Creep** pp = (Creep**)wrenSetSlotNewForeign(vm, 0, 0, sizeof(FILE*));
		};
		methods.finalize = nullptr;
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
	"class Game{\n"
	"	foreign static print(s)\n"
	"	foreign static time\n"
	"	foreign static creep(i)\n"
	"	foreign static creeps\n"
	"	foreign static spawns\n"
	"	static main { __main }\n"
	"	static main=(value) { __main = value }\n"
	"	static callMain() {\n"
	"		if(!(__main is Null)) __main.call()\n"
	"	}\n"
	"}\n"
	"foreign class Creep{\n"
	"	foreign pos\n"
	"	foreign move(i)\n"
	"	foreign id\n"
	"	foreign owner\n"
	"}\n"
	"foreign class Spawn{\n"
	"	foreign pos\n"
	"	foreign createCreep()\n"
	"	foreign id\n"
	"	foreign owner\n"
	"}\n");

	whmain = wrenMakeCallHandle(wren, "main");
	whcall = wrenMakeCallHandle(wren, "call()");
	whcallMain = wrenMakeCallHandle(wren, "callMain()");

	return 0;
}
