#ifndef MINE_H
#define MINE_H
#include "RoomObject.h"

#include "wren/src/include/wren.hpp"


struct Mine : public RoomObject{
	typedef Mine tt;

	int resource = max_resource;
	static const int max_resource = 500;

	static const SQUserPointer typetag;

	Mine(int x, int y) : RoomObject(x, y){}
	const char *className()const override{ return "Mine"; }

	static SQInteger sqf_get(HSQUIRRELVM v);

	static HSQOBJECT mineClass;

	void update();

	static void sq_define(HSQUIRRELVM v){
		sq_pushstring(v, _SC("Mine"), -1);
		sq_newclass(v, SQFalse);
		sq_setclassudsize(v, -1, sizeof(WeakPtr<Mine>));
		sq_settypetag(v, -1, typetag);
		sq_pushstring(v, _SC("_get"), -1);
		sq_newclosure(v, &sqf_get, 0);
		sq_newslot(v, -3, SQFalse);
		sq_getstackobj(v, -1, &mineClass);
		sq_addref(v, &mineClass);
		sq_newslot(v, -3, SQFalse);
	}

	static WrenForeignMethodFn wren_bind(WrenVM* vm, bool isStatic, const char* signature);
};

#endif
