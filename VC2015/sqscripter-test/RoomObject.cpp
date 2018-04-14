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
}

RoomObject::~RoomObject(){
	// Clear the referencer
	if(this == game.selected)
		game.selected = nullptr;
}
