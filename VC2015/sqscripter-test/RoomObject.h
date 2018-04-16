#ifndef ROOMOBJECT_H
#define ROOMOBJECT_H

#include "Observable.h"

#include "squirrel.h"
#include "wren/src/include/wren.hpp"

#include <vector>

struct RoomPosition{
	int x;
	int y;

	static const SQUserPointer typetag;

	RoomPosition(int x, int y) : x(x), y(y){}
	bool operator==(const RoomPosition& o)const{ return x == o.x && y == o.y; }
	bool operator!=(const RoomPosition& o)const{ return !(*this == o); }

	static SQInteger sqf_get(HSQUIRRELVM v);
};


struct RoomObject : public Observable{
	RoomPosition pos;
	int id = id_gen++;
	static int id_gen;

	// Scripting VM's memory handle must reside in C++ object member, since VMs only keep track of wrapper objects.
	HSQOBJECT hMemory;
	WrenHandle *whMemory = nullptr;

	std::vector<Observer*> observers;

	RoomObject(int x, int y);
	virtual ~RoomObject();

	virtual const char *className()const{ return "RoomObject"; }

	static WrenForeignMethodFn wren_bind(WrenVM* vm, bool isStatic, const char* signature);
};

template<typename T>
inline T *wrenGetWeakPtr(WrenVM* vm){
	WeakPtr<T>* pp = (WeakPtr<T>*)wrenGetSlotForeign(vm, 0);
	if(!pp)
		return nullptr;
	return *pp;
}


#endif
