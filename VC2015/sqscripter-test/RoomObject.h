#ifndef ROOMOBJECT_H
#define ROOMOBJECT_H

#include "squirrel.h"

struct RoomPosition{
	int x;
	int y;

	static const SQUserPointer typetag;

	RoomPosition(int x, int y) : x(x), y(y){}
	bool operator==(const RoomPosition& o)const{ return x == o.x && y == o.y; }
	bool operator!=(const RoomPosition& o)const{ return !(*this == o); }

	static SQInteger sqf_get(HSQUIRRELVM v);
};


struct RoomObject{
	RoomPosition pos;
	int id = id_gen++;
	static int id_gen;

	RoomObject(int x, int y);
	virtual ~RoomObject();

	virtual const char *className()const{ return "RoomObject"; }
};


#endif
