#include "RoomObject.h"
#include "Game.h"

const SQUserPointer RoomPosition::typetag = (SQUserPointer)_SC("RoomPosition");
int RoomObject::id_gen = 1;



SQInteger RoomPosition::sqf_get(HSQUIRRELVM v)
{
	SQUserPointer up;
	if(SQ_FAILED(sq_getinstanceup(v, 1, &up, typetag)))
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

RoomObject::RoomObject(int x, int y) : pos(x, y){
	sq_resetobject(&hMemory);
}

RoomObject::~RoomObject(){
	// Clear the referencer
	if(this == game.selected)
		game.selected = nullptr;
}

WrenForeignMethodFn RoomObject::wren_bind(WrenVM * vm, bool isStatic, const char * signature)
{
	if(!isStatic && !strcmp(signature, "alive")){
		return [](WrenVM* vm){
			RoomObject* obj = wrenGetWeakPtr<RoomObject>(vm);
			wrenSetSlotBool(vm, 0, obj != nullptr);
		};
	}
	else if(!isStatic && !strcmp(signature, "pos")){
		return [](WrenVM* vm){
			RoomObject* obj = wrenGetWeakPtr<RoomObject>(vm);
			if(!obj)
				return;
			wrenEnsureSlots(vm, 2);
			wrenGetVariable(vm, "main", "RoomPosition", 1);
			RoomPosition *ppos = (RoomPosition*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(RoomPosition));
			if(ppos)
				*ppos = obj->pos;
			/*			wrenSetSlotNewList(vm, 0);
			wrenSetSlotDouble(vm, 1, creep->pos.x);
			wrenInsertInList(vm, 0, -1, 1);
			wrenSetSlotDouble(vm, 1, creep->pos.y);
			wrenInsertInList(vm, 0, -1, 1);*/
		};
	}
	else if(!isStatic && !strcmp(signature, "id")){
		return [](WrenVM* vm){
			RoomObject* obj = wrenGetWeakPtr<RoomObject>(vm);
			if(!obj)
				return;
			wrenEnsureSlots(vm, 1);
			wrenSetSlotDouble(vm, 0, obj->id);
		};
	}
	return nullptr;
}
