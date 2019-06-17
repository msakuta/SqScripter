#ifndef GAME_H
#define GAME_H

#include "squirrel.h"
#include "Creep.h"
#include "Spawn.h"
#include "Mine.h"
#include <list>
#include <array>
//#include <set>
#include <queue>

const int ROOMSIZE = 50;

struct RoomPosition;
struct Spawn;
struct Mine;

struct Game{
	typedef Game tt;
	struct Race{
		int id = id_gen++;
		int kills = 0;
		int deaths = 0;
		static int id_gen;
		static const SQUserPointer typetag;
	};
	
	using RoomPositionT = std::array<int, 2>;
	struct DistanceEntry : RoomPositionT{
		double dist;

		DistanceEntry(const RoomPositionT& pos, double dist) :
			RoomPositionT(pos), dist(dist) {}

		bool operator>(const DistanceEntry& other) const {
			return this->dist > other.dist;
		}
	};

	Tile room[ROOMSIZE][ROOMSIZE] = {0};
	RoomObject *selected = nullptr;
	std::list<Creep> creeps;
	std::list<Spawn> spawns;
	std::list<Mine> mines;
	Race races[2];
	int global_time = 0;
	std::priority_queue<DistanceEntry, std::vector<DistanceEntry>, std::greater<DistanceEntry>> visitList;
	double distanceMap[ROOMSIZE][ROOMSIZE] = { 0. };

	bool isBlocked(const RoomPosition &pos)const;
	void init();
	void update();

	static void sq_define(HSQUIRRELVM v);
	static SQInteger sqf_get(HSQUIRRELVM v);

	static WrenForeignMethodFn wren_bind(WrenVM * vm, bool isStatic, const char * signature);
};

extern Game game;


#endif
