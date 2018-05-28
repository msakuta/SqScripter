#ifndef CREEP_H
#define CREEP_H
#include "RoomObject.h"

#include <cstdint>
#include <vector>

enum Direction{
	TOP = 1,
	TOP_RIGHT = 2,
	RIGHT = 3,
	BOTTOM_RIGHT = 4,
	BOTTOM = 5,
	BOTTOM_LEFT = 6,
	LEFT = 7,
	TOP_LEFT = 8,
};

struct Tile{
	uint32_t type;
	uint32_t object; ///< Object id if there is one on this tile or 0.
	int cost; ///< Used for A* pathfinding algorithm
};

struct PathNode{
	RoomPosition pos;
	int dx;
	int dy;

	PathNode(const RoomPosition& pos, int dx = 0, int dy = 0) : pos(pos), dx(dx), dy(dy){}
};

/// Path has nodes in reverse order of progress, because it's better to truncate from the last
/// element so that former elements are not moved in the memory.
typedef std::vector<PathNode> Path;

struct Creep : public RoomObject{
	typedef RoomObject st;

	Path path;
	int owner;
	int ttl = max_ttl; // Time to Live
	int health = max_health; // Hit points of this Creep
	int resource = 0; // Resource cargo
	int fatigue = 0;
	int moveParts = 1;
	static const int max_ttl = 1000;
	static const int max_health = 100;
	static const int max_resource = 100;
	static const int max_parts = 20;

	static const SQUserPointer typetag;

	Creep(int x, int y, int owner, int moveParts);
	const char *className()const override{ return "Creep"; }
	bool move(int direction);
	bool move(int dx, int dy);
	bool harvest(int direction);
	bool store(int direction);
	bool attack(int direction);
	bool findPath(const RoomPosition& pos);
	bool followPath();
	void update();

	static void sq_define(HSQUIRRELVM sqvm);
	static SQInteger sqf_get(HSQUIRRELVM v);
	static SQInteger sqf_move(HSQUIRRELVM v);
	static SQInteger sqf_harvest(HSQUIRRELVM v);
	static SQInteger sqf_store(HSQUIRRELVM v);

	static WrenForeignMethodFn wren_bind(WrenVM* vm, bool isStatic, const char* signature);
};


#endif
