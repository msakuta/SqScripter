#ifndef GAME_H
#define GAME_H

#include "squirrel.h"
#include "Creep.h"
#include "Spawn.h"
#include "Mine.h"
#include <list>

const int ROOMSIZE = 50;

struct RoomPosition;
struct Spawn;
struct Mine;

struct Game{
	typedef Game tt;
	Tile room[ROOMSIZE][ROOMSIZE] = {0};
	RoomObject *selected = nullptr;
	std::list<Creep> creeps;
	std::list<Spawn> spawns;
	std::list<Mine> mines;
	int global_time = 0;

	bool isBlocked(const RoomPosition &pos)const;
	void init();
	void update();

	static SQInteger sqf_get(HSQUIRRELVM v);

	static WrenForeignMethodFn wren_bind(WrenVM * vm, bool isStatic, const char * signature);
};

extern Game game;


#endif
