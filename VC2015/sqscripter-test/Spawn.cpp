#include "Spawn.h"
#include "Game.h"
#include "sqscripter-test.h"

extern "C"{
#include "clib/c.h"
}



const SQUserPointer Spawn::typetag = _SC("Spawn");
SQObject Spawn::spawnClass;

bool Spawn::createCreep(){
	if(resource < creep_cost)
		return false;
	static const int directions[4][2] = {
		{ 0,-1 },
		{ 1,0 },
		{ 0,1 },
		{ -1,0 }
	};
	int i = 0;
	do{
		RoomPosition newPos{ pos.x + directions[i][0], pos.y + directions[i][1] };
		if(game.isBlocked(newPos))
			continue;
		game.creeps.push_back(Creep(newPos.x, newPos.y, owner));
		game.room[newPos.y][newPos.x].object = game.creeps.back().id;
		resource -= creep_cost;
		return true;
	} while(++i < numof(directions));
	return false;
}

void Spawn::update()
{
	if(game.global_time % 10 == 0 && resource < max_gen_resource)
		resource++;
}

SQInteger Spawn::sqf_createCreep(HSQUIRRELVM v)
{
	SQUserPointer up;
	if(SQ_FAILED(sq_getinstanceup(v, 1, &up, typetag)))
		return sq_throwerror(v, _SC("Invalid this pointer for a spawn"));
	Spawn *spawn = static_cast<Spawn*>(up);
	if(!up || !spawn)
		return sq_throwerror(v, _SC("Invalid this pointer for a spawn"));
	SQBool b = spawn->createCreep();
	sq_pushbool(v, b);
	return 1;
}

SQInteger Spawn::sqf_get(HSQUIRRELVM v){
	wxMutexLocker ml(wxGetApp().mutex);
	SQUserPointer up;
	if(SQ_FAILED(sq_getinstanceup(v, 1, &up, typetag)))
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
		sq_getinstanceup(v, -1, &up, RoomPosition::typetag);
		RoomPosition *rp = static_cast<RoomPosition*>(up);
		*rp = spawn->pos;
		return 1;
	}
	else if(!scstrcmp(key, _SC("id"))){
		sq_pushroottable(v);
		sq_pushinteger(v, spawn->id);
		return 1;
	}
	else if(!scstrcmp(key, _SC("resource"))){
		sq_pushroottable(v);
		sq_pushinteger(v, spawn->resource);
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
	else if(!isStatic && !strcmp(signature, "resource")){
		return [](WrenVM* vm){
			tt** pp = (tt**)wrenGetSlotForeign(vm, 0);
			if(!pp)
				return;
			tt *creep = *pp;
			wrenEnsureSlots(vm, 1);
			wrenSetSlotDouble(vm, 0, creep->resource);
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

