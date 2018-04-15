#ifndef SPAWN_H
#define SPAWN_H
#include "RoomObject.h"

#include "wren/src/include/wren.hpp"


struct Spawn : public RoomObject{
	typedef Spawn tt;

	int owner;
	int resource = 0;
	static const int max_resource = 1000;
	static const int max_gen_resource = 100;
	static const int creep_cost = 20;

	static const SQUserPointer typetag;

	Spawn(int x, int y, int owner) : RoomObject(x, y), owner(owner){}
	const char *className()const override{ return "Spawn"; }

	bool createCreep();

	void update();

	static SQInteger sqf_createCreep(HSQUIRRELVM v);

	static SQInteger sqf_get(HSQUIRRELVM v);

	static HSQOBJECT spawnClass;

	static void sq_define(HSQUIRRELVM v){
		sq_pushstring(v, _SC("Spawn"), -1);
		sq_newclass(v, SQFalse);
		sq_setclassudsize(v, -1, sizeof(WeakPtr<Spawn>));
		sq_settypetag(v, -1, typetag);
		sq_pushstring(v, _SC("createCreep"), -1);
		sq_newclosure(v, &sqf_createCreep, 0);
		sq_newslot(v, -3, SQFalse);
		sq_pushstring(v, _SC("_get"), -1);
		sq_newclosure(v, &sqf_get, 0);
		sq_newslot(v, -3, SQFalse);
		sq_getstackobj(v, -1, &spawnClass);
		sq_addref(v, &spawnClass);
		sq_newslot(v, -3, SQFalse);
	}

	static WrenForeignMethodFn wren_bind(WrenVM* vm, bool isStatic, const char* signature);
};

#endif
