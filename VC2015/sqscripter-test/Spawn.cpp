#include "Spawn.h"
#include "Game.h"
#include "sqscripter-test.h"

extern "C"{
#include "clib/c.h"
}



const SQUserPointer Spawn::typetag = _SC("Spawn");
SQObject Spawn::spawnClass;

bool Spawn::createCreep(int moveParts){
	moveParts = std::max(1, std::min(Creep::max_parts, moveParts));
	int cost = creep_cost + moveParts * move_part_cost;
	if(resource < cost)
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
		game.creeps.push_back(Creep(newPos.x, newPos.y, owner, moveParts));
		game.room[newPos.y][newPos.x].object = game.creeps.back().id;
		resource -= cost;
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
	if(SQ_FAILED(sq_getinstanceup(v, 1, &up, typetag)) || !up)
		return sq_throwerror(v, _SC("Invalid this pointer for a spawn"));
	Spawn *spawn = *static_cast<WeakPtr<Spawn>*>(up);
	if(!spawn)
		return sq_throwerror(v, _SC("Spawn object has been deleted"));
	SQInteger moveParts;
	if(SQ_FAILED(sq_getinteger(v, 2, &moveParts))){
		moveParts = 1;
	}
	SQBool b = spawn->createCreep(int(moveParts));
	sq_pushbool(v, b);
	return 1;
}

SQInteger Spawn::sqf_get(HSQUIRRELVM v){
	wxMutexLocker ml(wxGetApp().mutex);
	SQUserPointer up;
	if(SQ_FAILED(sq_getinstanceup(v, 1, &up, typetag)))
		return sq_throwerror(v, _SC("Invalid this pointer for Spawn"));

	const SQChar *key;
	if(SQ_FAILED(sq_getstring(v, 2, &key)))
		return sq_throwerror(v, _SC("Broken key in _get"));

	Spawn *spawn = *static_cast<WeakPtr<Spawn>*>(up);
	if(!scstrcmp(key, _SC("alive"))){
		sq_pushbool(v, spawn != nullptr);
		return 1;
	}
	if(!spawn)
		return sq_throwerror(v, _SC("Spawn object has been deleted"));
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
	else if(!scstrcmp(key, _SC("owner"))){
		sq_pushroottable(v);
		sq_pushinteger(v, spawn->owner);
		return 1;
	}
	else if(!scstrcmp(key, _SC("resource"))){
		sq_pushroottable(v);
		sq_pushinteger(v, spawn->resource);
		return 1;
	}
	else if(!scstrcmp(key, _SC("creep_cost"))){
		sq_pushroottable(v);
		sq_pushinteger(v, spawn->creep_cost);
		return 1;
	}
	else if(!scstrcmp(key, _SC("move_part_cost"))){
		sq_pushroottable(v);
		sq_pushinteger(v, spawn->move_part_cost);
		return 1;
	}
	else
		return sq_throwerror(v, (std::basic_string<SQChar>(_SC("Couldn't find key: ")) + std::basic_string<SQChar>(key)).c_str());
}

WrenForeignMethodFn Spawn::wren_bind(WrenVM * vm, bool isStatic, const char * signature)
{
	WrenForeignMethodFn ret = st::wren_bind(vm, isStatic, signature);
	if(ret)
		return ret;
	else if(!isStatic && !strcmp(signature, "owner")){
		return [](WrenVM* vm){
			Spawn* spawn = wrenGetWeakPtr<Spawn>(vm);
			if(!spawn)
				return;
			wrenEnsureSlots(vm, 1);
			wrenSetSlotDouble(vm, 0, spawn->owner);
		};
	}
	else if(!isStatic && !strcmp(signature, "resource")){
		return [](WrenVM* vm){
			Spawn* spawn = wrenGetWeakPtr<Spawn>(vm);
			if(!spawn)
				return;
			wrenEnsureSlots(vm, 1);
			wrenSetSlotDouble(vm, 0, spawn->resource);
		};
	}
	else if(!isStatic && !strcmp(signature, "createCreep(_)")){
		return [](WrenVM* vm){
			wxMutexLocker ml(wxGetApp().mutex);
			Spawn* spawn = wrenGetWeakPtr<Spawn>(vm);
			if(!spawn)
				return;
			double moveParts = wrenGetSlotDouble(vm, 1);
			wrenSetSlotBool(vm, 0, spawn->createCreep(int(moveParts)));
		};
	}
	else if(isStatic && !strcmp(signature, "creep_cost")){
		return [](WrenVM* vm){
			wrenSetSlotDouble(vm, 0, Spawn::creep_cost);
		};
	}
	else if(isStatic && !strcmp(signature, "move_part_cost")){
		return [](WrenVM* vm){
			wrenSetSlotDouble(vm, 0, Spawn::move_part_cost);
		};
	}
	return WrenForeignMethodFn();
}

