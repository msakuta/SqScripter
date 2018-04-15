#include "Mine.h"

#include "sqscripter-test.h"

const SQUserPointer Mine::typetag = _SC("Mine");
SQObject Mine::mineClass;



SQInteger Mine::sqf_get(HSQUIRRELVM v)
{
	wxMutexLocker ml(wxGetApp().mutex);
	SQUserPointer up;
	if(SQ_FAILED(sq_getinstanceup(v, 1, &up, typetag)))
		return sq_throwerror(v, _SC("Invalid this pointer for Spawn"));

	const SQChar *key;
	if(SQ_FAILED(sq_getstring(v, 2, &key)))
		return sq_throwerror(v, _SC("Broken key in _get"));

	Mine *mine = *static_cast<WeakPtr<Mine>*>(up);
	if(!scstrcmp(key, _SC("alive"))){
		sq_pushbool(v, mine != nullptr);
		return 1;
	}
	if(!mine)
		return sq_throwerror(v, _SC("Mine object has been deleted"));
	if(!scstrcmp(key, _SC("pos"))){
		sq_pushroottable(v);
		sq_pushstring(v, _SC("RoomPosition"), -1);
		if(SQ_FAILED(sq_get(v, -2)))
			return sq_throwerror(v, _SC("Can't find RoomPosition class definition"));
		sq_createinstance(v, -1);
		sq_getinstanceup(v, -1, &up, RoomPosition::typetag);
		RoomPosition *rp = static_cast<RoomPosition*>(up);
		*rp = mine->pos;
		return 1;
	}
	else if(!scstrcmp(key, _SC("id"))){
		sq_pushroottable(v);
		sq_pushinteger(v, mine->id);
		return 1;
	}
	else if(!scstrcmp(key, _SC("resource"))){
		sq_pushroottable(v);
		sq_pushinteger(v, mine->resource);
		return 1;
	}
	else
		return sq_throwerror(v, _SC("Couldn't find key"));
	return SQInteger();
}

void Mine::update(){
	//	if(resource < max_resource && global_time % 2 == 0)
	//		resource++;
}

static Mine *wrenGetMine(WrenVM* vm){
	WeakPtr<Mine>* pp = (WeakPtr<Mine>*)wrenGetSlotForeign(vm, 0);
	if(!pp)
		return nullptr;
	return *pp;
}

WrenForeignMethodFn Mine::wren_bind(WrenVM * vm, bool isStatic, const char * signature)
{
	if(!isStatic && !strcmp(signature, "alive")){
		return [](WrenVM* vm){
			Mine* mine = wrenGetMine(vm);
			wrenSetSlotBool(vm, 0, mine != nullptr);
		};
	}
	else if(!isStatic && !strcmp(signature, "pos")){
		return [](WrenVM* vm){
			Mine* mine = wrenGetMine(vm);
			if(!mine)
				return;
			wrenEnsureSlots(vm, 2);
			wrenSetSlotNewList(vm, 0);
			wrenSetSlotDouble(vm, 1, mine->pos.x);
			wrenInsertInList(vm, 0, -1, 1);
			wrenSetSlotDouble(vm, 1, mine->pos.y);
			wrenInsertInList(vm, 0, -1, 1);
		};
	}
	else if(!isStatic && !strcmp(signature, "id")){
		return [](WrenVM* vm){
			Mine* mine = wrenGetMine(vm);
			if(!mine)
				return;
			wrenEnsureSlots(vm, 1);
			wrenSetSlotDouble(vm, 0, mine->id);
		};
	}
	else if(!isStatic && !strcmp(signature, "resource")){
		return [](WrenVM* vm){
			Mine* mine = wrenGetMine(vm);
			if(!mine)
				return;
			wrenEnsureSlots(vm, 1);
			wrenSetSlotDouble(vm, 0, mine->resource);
		};
	}
	return WrenForeignMethodFn();
}
