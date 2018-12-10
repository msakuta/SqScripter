#include "Creep.h"
#include "Game.h"
#include "sqscripter-test.h"

#include <vector>
#include <array>
#include <algorithm>


extern "C"{
#include "clib/c.h"
}


const SQUserPointer Creep::typetag = _SC("Creep");

std::vector<PathNode> findPath(const RoomPosition& from, const RoomPosition& to, const RoomObject& self){
	// List of positions to start calculating cost.
	// Using std::vector to cache the next candidates is surprisingly efficient,
	// because the number of target positions in one iteration is orders of magnitude smaller than the total number of tiles.
	// Also surprisingly, we don't need std::set to exclude duplicates, because the same candidate is already marked solved by assigning
	// non-infinity cost.  If we used std::set, it would take cost in performance to build a hash table (or a binary tree or whatever).
	std::vector<RoomPosition> targetList;

	// Total cost estimation in A* algorithm, i.e. f = g + h.
	// Allocating big chunk of data on the stack doesn't feel pleasant, but we know that this function won't recurse,
	// so it should be safe.  We do this because extending the stack should be faster than allocating from heap.
	std::array<std::array<double, ROOMSIZE>, ROOMSIZE> fScore;

	// Heuristic function used in A* algorithm.  We use simple Euclidian distance.
	static auto heuristic_cost_estimate = [](const RoomPosition& from, const RoomPosition& to){
		return sqrt((from.x - to.x) * (from.x - to.x) + (from.y - to.y) * (from.y - to.y));
	};

	int changes = 0;

	auto internal = [&to, &self, &targetList, &fScore, &changes](){
		if(targetList.empty())
			return 0;
		double min_cost = HUGE_VAL;
		//const RoomPosition* min_i = nullptr;
		int min_i = -1;
		for(int i = 0; i < targetList.size(); i++){
			auto& it = targetList[i];
			auto itcost = fScore[it.y][it.x];
			if(itcost < min_cost){
				min_cost = itcost;
				min_i = i;
			}
		}
/*		for(auto& it : targetList){
			auto itcost = game.room[it.y][it.x].cost;
			if(itcost < min_cost){
				min_cost = itcost;
				min_i = &it;
			}
		}*/
		if(0 <= min_i){
		//if(min_i){
			auto& it = targetList[min_i];
			//const RoomPosition& it = *min_i;
			int x = it.x;
			int y = it.y;
			Tile &tile = game.room[y][x];
			//if(tile.cost == INT_MAX)
				//continue;
			for(int y1 = std::max(y - 1, 0); y1 <= std::min(y + 1, ROOMSIZE - 1); y1++){
				for(int x1 = std::max(x - 1, 0); x1 <= std::min(x + 1, ROOMSIZE - 1); x1++){
					Tile &tile1 = game.room[y1][x1];

					// Ignore collision with destination (assume we're ok with tile next to it)
					if(to.x == x1 && to.y == y1){
						tile1.cost = tile.cost + 1;
						return 0;
					}

					// Check if the path is blocked by another creep.
					if(tile1.object != 0 && tile1.object != self.id)
						continue;

					if(tile1.type == 0 && tile.cost + 1 < tile1.cost){
						tile1.cost = tile.cost + 1;
						targetList.push_back(RoomPosition(x1, y1));
						//targetList.insert(RoomPosition(x1, y1));
						fScore[y1][x1] = tile1.cost + heuristic_cost_estimate(targetList.back(), to);
						changes++;
					}
				}
			}

			// Deleting an element in a random position in a vector is not cheap in general (i.e. O(N)),
			// but we keep the element small so that linked list would be more costly.
			targetList.erase(targetList.begin() + min_i);
		}
		return (int)targetList.size();
	};

	targetList.push_back(from);
	//targetList.insert(from);

	for(int y = 0; y < ROOMSIZE; y++){
		for(int x = 0; x < ROOMSIZE; x++){
			Tile &tile = game.room[y][x];
			tile.cost = (x == from.x && y == from.y ? 0 : INT_MAX);
			fScore[y][x] = HUGE_VAL;
		}
	}

	fScore[from.y][from.x] = heuristic_cost_estimate(from, to);

	std::vector<PathNode> ret;

	while(internal()){
	}

	Tile &destTile = game.room[to.y][to.x];
	if(destTile.cost == INT_MAX)
		return ret;

	// Prefer vertical or horizontal movenent over diagonal even though the costs are the same,
	// by searching preferred positions first.
	static const int deltaPreferences[][2] = { { -1,0 },{ 0,-1 },{ 1,0 },{ 0,1 },{ -1,-1 },{ -1,1 },{ 1,-1 },{ 1,1 } };

	RoomPosition cur = to;
	while(cur != from){
		Tile &tile = game.room[cur.y][cur.x];
		int bestx = -1, besty = -1;
		int besti;
		int bestcost = tile.cost;
		for(int i = 0; i < numof(deltaPreferences); i++){
			int y1 = cur.y + deltaPreferences[i][1];
			int x1 = cur.x + deltaPreferences[i][0];
			if(x1 < 0 || ROOMSIZE <= x1 || y1 < 0 || ROOMSIZE <= y1)
				continue;
			Tile &tile1 = game.room[y1][x1];
			if(tile1.type == 0 && tile1.cost < bestcost){
				bestcost = tile1.cost;
				bestx = x1;
				besty = y1;
				besti = i;
			}
		}
		cur.x = bestx;
		cur.y = besty;
		ret.push_back(PathNode(cur, deltaPreferences[besti][0], deltaPreferences[besti][1]));
	}

	return ret;
}


Creep::Creep(int x, int y, int owner, int moveParts) :
	RoomObject(x, y),
	owner(owner),
	moveParts(moveParts)
{}

void Creep::sq_define(HSQUIRRELVM sqvm){
	sq_pushstring(sqvm, _SC("Creep"), -1);
	sq_newclass(sqvm, SQFalse);
	sq_setclassudsize(sqvm, -1, sizeof(WeakPtr<Creep>));
	sq_settypetag(sqvm, -1, typetag);
	sq_pushstring(sqvm, _SC("move"), -1);
	sq_newclosure(sqvm, &Creep::sqf_move, 0);
	sq_newslot(sqvm, -3, SQFalse);
	sq_pushstring(sqvm, _SC("harvest"), -1);
	sq_newclosure(sqvm, &Creep::sqf_harvest, 0);
	sq_newslot(sqvm, -3, SQFalse);
	sq_pushstring(sqvm, _SC("store"), -1);
	sq_newclosure(sqvm, &Creep::sqf_store, 0);
	sq_newslot(sqvm, -3, SQFalse);
	sq_pushstring(sqvm, _SC("attack"), -1);
	sq_newclosure(sqvm, [](HSQUIRRELVM v){
		wxMutexLocker ml(wxGetApp().mutex);
		SQUserPointer p;
		Creep *creep;
		if(SQ_FAILED(sq_getinstanceup(v, 1, &p, typetag)) || !(creep = *(WeakPtr<Creep>*)p))
			return sq_throwerror(v, _SC("Broken Creep instance"));
		SQInteger i = 1;
		if(SQ_FAILED(sq_getinteger(v, 2, &i)))
			i = 1; // Don't make an omitted parameter an error.
		sq_pushbool(v, creep->attack(i));
		return SQInteger(1);
	}, 0);
	sq_newslot(sqvm, -3, SQFalse);
	sq_pushstring(sqvm, _SC("findPath"), -1);
	sq_newclosure(sqvm, [](HSQUIRRELVM v){
		wxMutexLocker ml(wxGetApp().mutex);
		SQUserPointer p;
		Creep *creep;
		if(SQ_FAILED(sq_getinstanceup(v, 1, &p, typetag)) || !(creep = *(WeakPtr<Creep>*)p))
			return sq_throwerror(v, _SC("Broken Creep instance"));
		RoomPosition *pos;
		if(SQ_FAILED(sq_getinstanceup(v, 2, &p, RoomPosition::typetag)) || !(pos = (RoomPosition*)p))
			return sq_throwerror(v, _SC("Couldn't interpret the second argument as RoomPosition"));
		sq_pushbool(v, creep->findPath(*pos));
		return SQInteger(1);
	}, 0);
	sq_newslot(sqvm, -3, SQFalse);
	sq_pushstring(sqvm, _SC("followPath"), -1);
	sq_newclosure(sqvm, [](HSQUIRRELVM v){
		wxMutexLocker ml(wxGetApp().mutex);
		SQUserPointer p;
		Creep *creep;
		if(SQ_FAILED(sq_getinstanceup(v, 1, &p, typetag)) || !(creep = *(WeakPtr<Creep>*)p))
			return sq_throwerror(v, _SC("Broken Creep instance"));
		sq_pushbool(v, creep->followPath());
		return SQInteger(1);
	}, 0);
	sq_newslot(sqvm, -3, SQFalse);
	sq_pushstring(sqvm, _SC("_get"), -1);
	sq_newclosure(sqvm, &Creep::sqf_get, 0);
	sq_newslot(sqvm, -3, SQFalse);
	sq_pushstring(sqvm, _SC("_set"), -1);
	sq_newclosure(sqvm, [](HSQUIRRELVM v){
		wxMutexLocker ml(wxGetApp().mutex);
		SQUserPointer up;
		if(SQ_FAILED(sq_getinstanceup(v, 1, &up, typetag)))
			return sq_throwerror(v, _SC("Invalid this pointer"));
		Creep *creep = *static_cast<WeakPtr<Creep>*>(up);
		const SQChar *key;
		if(SQ_FAILED(sq_getstring(v, 2, &key)))
			return sq_throwerror(v, _SC("Broken key in _get"));
		if(!scstrcmp(key, _SC("memory"))){
			HSQOBJECT obj;
			if(SQ_FAILED(sq_getstackobj(v, 3, &obj)))
				return sq_throwerror(v, _SC("Couldn't set memory"));
			sq_release(v, &creep->hMemory);
			creep->hMemory = obj;
			sq_addref(v, &creep->hMemory);
			return SQInteger(1);
		}
		return SQInteger(0);
	}, 0);
	sq_newslot(sqvm, -3, SQFalse);
	sq_newslot(sqvm, -3, SQFalse);
}

SQInteger Creep::sqf_get(HSQUIRRELVM v){
	wxMutexLocker ml(wxGetApp().mutex);
	SQUserPointer up;
	if(SQ_FAILED(sq_getinstanceup(v, 1, &up, typetag)))
		return sq_throwerror(v, _SC("Invalid this pointer"));

	const SQChar *key;
	if(SQ_FAILED(sq_getstring(v, 2, &key)))
		return sq_throwerror(v, _SC("Broken key in _get"));

	Creep *creep = *static_cast<WeakPtr<Creep>*>(up);
	if(!scstrcmp(key, _SC("alive"))){
		sq_pushbool(v, creep != nullptr);
		return 1;
	}
	if(!creep)
		return sq_throwerror(v, _SC("Creep object has been deleted"));

	if(!scstrcmp(key, _SC("pos"))){
		sq_pushroottable(v);
		sq_pushstring(v, _SC("RoomPosition"), -1);
		if(SQ_FAILED(sq_get(v, -2)))
			return sq_throwerror(v, _SC("Can't find RoomPosition class definition"));
		sq_createinstance(v, -1);
		sq_getinstanceup(v, -1, &up, RoomPosition::typetag);
		RoomPosition *rp = static_cast<RoomPosition*>(up);
		*rp = creep->pos;
		return 1;
	}
	else if(!scstrcmp(key, _SC("id"))){
		sq_pushroottable(v);
		sq_pushinteger(v, creep->id);
		return 1;
	}
	else if(!scstrcmp(key, _SC("owner"))){
		sq_pushroottable(v);
		sq_pushinteger(v, creep->owner);
		return 1;
	}
	else if(!scstrcmp(key, _SC("memory"))){
		sq_pushroottable(v);
		sq_pushobject(v, creep->hMemory);
		return 1;
	}
	else if(!scstrcmp(key, _SC("ttl"))){
		sq_pushroottable(v);
		sq_pushinteger(v, creep->ttl);
		return 1;
	}
	else if(!scstrcmp(key, _SC("resource"))){
		sq_pushroottable(v);
		sq_pushinteger(v, creep->resource);
		return 1;
	}
	else if(!scstrcmp(key, _SC("moveParts"))){
		sq_pushroottable(v);
		sq_pushinteger(v, creep->moveParts);
		return 1;
	}
	else
		return sq_throwerror(v, _SC("Couldn't find key"));
}

SQInteger Creep::sqf_move(HSQUIRRELVM v){
	wxMutexLocker ml(wxGetApp().mutex);
	SQUserPointer p;
	Creep *creep;
	if(SQ_FAILED(sq_getinstanceup(v, 1, &p, typetag)) || !(creep = *(WeakPtr<Creep>*)p))
		return sq_throwerror(v, _SC("Broken Creep instance"));
	SQInteger i;
	if(SQ_FAILED(sq_getinteger(v, 2, &i)) || i < TOP || TOP_LEFT < i)
		return sq_throwerror(v, _SC("Invalid Creep.move argument"));
	sq_pushbool(v, creep->move(i));
	return 1;
}

SQInteger Creep::sqf_harvest(HSQUIRRELVM v)
{
	wxMutexLocker ml(wxGetApp().mutex);
	SQUserPointer p;
	Creep *creep;
	if(SQ_FAILED(sq_getinstanceup(v, 1, &p, typetag)) || !(creep = *(WeakPtr<Creep>*)p))
		return sq_throwerror(v, _SC("Broken Creep instance"));
	SQInteger i;
	if(SQ_FAILED(sq_getinteger(v, 2, &i)) || i < TOP || TOP_LEFT < i)
		return sq_throwerror(v, _SC("Invalid Creep.move argument"));
	sq_pushbool(v, creep->harvest(i));
	return 1;
}

SQInteger Creep::sqf_store(HSQUIRRELVM v)
{
	wxMutexLocker ml(wxGetApp().mutex);
	SQUserPointer p;
	Creep *creep;
	if(SQ_FAILED(sq_getinstanceup(v, 1, &p, typetag)) || !(creep = *(WeakPtr<Creep>*)p))
		return sq_throwerror(v, _SC("Broken Creep instance"));
	SQInteger i;
	if(SQ_FAILED(sq_getinteger(v, 2, &i)) || i < TOP || TOP_LEFT < i)
		return sq_throwerror(v, _SC("Invalid Creep.move argument"));
	sq_pushbool(v, creep->store(i));
	return 1;
}

bool Creep::move(int direction){
	static int deltas[8][2] = {
		{ 0,-1 }, // TOP = 1,
		{ 1,-1 }, // TOP_RIGHT = 2,
		{ 1,0 }, // RIGHT = 3,
		{ 1,1 }, // BOTTOM_RIGHT = 4,
		{ 0,1 }, // BOTTOM = 5,
		{ -1,1 }, // BOTTOM_LEFT = 6,
		{ -1,0 }, // LEFT = 7,
		{ -1,-1 }, // TOP_LEFT = 8,
	};
	return move(deltas[direction - 1][0], deltas[direction - 1][1]);
}

bool Creep::move(int dx, int dy){
	if(0 < fatigue){
		fatigue = std::max(fatigue - moveParts, 0);
		return false;
	}

	// Calculate amount of fatigue by carrying resource weight.
	int workCost = 1;
	int bit = 0;
	while((1 << bit) < resource) bit++;
	fatigue += bit;

	RoomPosition newPos = pos;
	newPos.x += dx;
	newPos.y += dy;
	if(game.isBlocked(newPos))
		return false;
	game.room[pos.y][pos.x].object = 0;
	pos = newPos;
	game.room[pos.y][pos.x].object = id;
	return true;
}

bool Creep::harvest(int direction)
{
	for(auto& mine : game.mines){
		int dist = std::max(std::abs(mine.pos.x - pos.x), std::abs(mine.pos.y - pos.y));
		if(dist <= 1 && 0 < mine.resource){
			int harvest_amount = 10;
			if(mine.resource < harvest_amount)
				harvest_amount = mine.resource;
			if(max_resource - resource < harvest_amount)
				harvest_amount = max_resource - resource;
			resource += harvest_amount;
			mine.resource -= harvest_amount;
			return true;
		}
	}
	return false;
}

bool Creep::store(int direction)
{
	for(auto& spawn : game.spawns){
		int dist = std::max(std::abs(spawn.pos.x - pos.x), std::abs(spawn.pos.y - pos.y));
		if(dist <= 1 && 0 < resource){
			int store_amount = resource;
			if(spawn.resource < store_amount)
				store_amount = spawn.resource;
			if(spawn.max_resource - spawn.resource < store_amount)
				store_amount = spawn.max_resource - spawn.resource;
			resource -= store_amount;
			spawn.resource += store_amount;
			return true;
		}
	}
	return false;
}

bool Creep::attack(int direction)
{
	for(auto& creep : game.creeps){
		int dist = std::max(std::abs(creep.pos.x - pos.x), std::abs(creep.pos.y - pos.y));
		// Find an enemy
		if(dist <= 1 && 0 < resource && 0 < creep.health && owner != creep.owner){
			int damage_amount = resource;
			if(creep.health < damage_amount)
				damage_amount = creep.health;
			if(0 < creep.health && creep.health - damage_amount <= 0){
				game.races[owner].kills++;
				game.races[creep.owner].deaths++;
			}
			resource -= damage_amount;
			creep.health -= damage_amount;
			return true;
		}
	}
	return false;
}

bool Creep::findPath(const RoomPosition& dest){
	auto ret = ::findPath(this->pos, dest, *this);
	if(ret.size()){
		this->path = ret;
		return true;
	}
	return false;
}

bool Creep::followPath(){
	if(0 < path.size()){
		auto& node = path.back();
		bool ret = move(-node.dx, -node.dy);
		if(ret)
			path.pop_back();
		return ret;
	}
	return false;
}

void Creep::update()
{
	if(ttl == 0)
		return;
	ttl--;
}

WrenForeignMethodFn Creep::wren_bind(WrenVM * vm, bool isStatic, const char * signature)
{
	WrenForeignMethodFn ret = st::wren_bind(vm, isStatic, signature);
	if(ret)
		return ret;
	else if(!isStatic && !strcmp(signature, "owner")){
		return [](WrenVM* vm){
			Creep* creep = wrenGetWeakPtr<Creep>(vm);
			if(!creep)
				return;
			wrenEnsureSlots(vm, 1);
			wrenSetSlotDouble(vm, 0, creep->owner);
		};
	}
	else if(!isStatic && !strcmp(signature, "ttl")){
		return [](WrenVM* vm){
			Creep* creep = wrenGetWeakPtr<Creep>(vm);
			if(!creep)
				return;
			wrenEnsureSlots(vm, 1);
			wrenSetSlotDouble(vm, 0, creep->ttl);
		};
	}
	else if(!isStatic && !strcmp(signature, "resource")){
		return [](WrenVM* vm){
			Creep* creep = wrenGetWeakPtr<Creep>(vm);
			if(!creep)
				return;
			wrenEnsureSlots(vm, 1);
			wrenSetSlotDouble(vm, 0, creep->resource);
		};
	}
	else if(!isStatic && !strcmp(signature, "moveParts")){
		return [](WrenVM* vm){
			Creep* creep = wrenGetWeakPtr<Creep>(vm);
			if(!creep)
				return;
			wrenEnsureSlots(vm, 1);
			wrenSetSlotDouble(vm, 0, creep->moveParts);
		};
	}
	else if(!isStatic && !strcmp(signature, "memory")){
		return [](WrenVM* vm){
			Creep* creep = wrenGetWeakPtr<Creep>(vm);
			if(!creep)
				return;
			wrenEnsureSlots(vm, 1);
			if(creep->whMemory)
				wrenSetSlotHandle(vm, 0, creep->whMemory);
			else
				wrenSetSlotNull(vm, 0);
		};
	}
	else if(!isStatic && !strcmp(signature, "memory=(_)")){
		return [](WrenVM* vm){
			Creep* creep = wrenGetWeakPtr<Creep>(vm);
			if(!creep)
				return;
			creep->whMemory = wrenGetSlotHandle(vm, 1);
		};
	}
	else if(!isStatic && !strcmp(signature, "move(_)")){
		return [](WrenVM* vm){
			wxMutexLocker ml(wxGetApp().mutex);
			Creep* creep = wrenGetWeakPtr<Creep>(vm);
			if(!creep)
				return;
			if(WREN_TYPE_NUM != wrenGetSlotType(vm, 1)){
				return;
			}
			int i = (int)wrenGetSlotDouble(vm, 1);
			bool ret = creep->move(i);
			wrenEnsureSlots(vm, 1);
			wrenSetSlotBool(vm, 0, ret);
		};
	}
	else if(!isStatic && !strcmp(signature, "harvest(_)")){
		return [](WrenVM* vm){
			wxMutexLocker ml(wxGetApp().mutex);
			Creep* creep = wrenGetWeakPtr<Creep>(vm);
			if(!creep)
				return;
			if(WREN_TYPE_NUM != wrenGetSlotType(vm, 1)){
				return;
			}
			int i = (int)wrenGetSlotDouble(vm, 1);
			bool ret = creep->harvest(i);
			wrenEnsureSlots(vm, 1);
			wrenSetSlotBool(vm, 0, ret);
		};
	}
	else if(!isStatic && !strcmp(signature, "store(_)")){
		return [](WrenVM* vm){
			wxMutexLocker ml(wxGetApp().mutex);
			Creep* creep = wrenGetWeakPtr<Creep>(vm);
			if(!creep)
				return;
			if(WREN_TYPE_NUM != wrenGetSlotType(vm, 1)){
				return;
			}
			int i = (int)wrenGetSlotDouble(vm, 1);
			bool ret = creep->store(i);
			wrenEnsureSlots(vm, 1);
			wrenSetSlotBool(vm, 0, ret);
		};
	}
	else if(!isStatic && !strcmp(signature, "attack(_)")){
		return [](WrenVM* vm){
			wxMutexLocker ml(wxGetApp().mutex);
			Creep* creep = wrenGetWeakPtr<Creep>(vm);
			if(!creep)
				return;
			if(WREN_TYPE_NUM != wrenGetSlotType(vm, 1)){
				return;
			}
			int i = (int)wrenGetSlotDouble(vm, 1);
			bool ret = creep->attack(i);
			wrenEnsureSlots(vm, 1);
			wrenSetSlotBool(vm, 0, ret);
		};
	}
	else if(!isStatic && !strcmp(signature, "findPath(_)")){
		return [](WrenVM* vm){
			wxMutexLocker ml(wxGetApp().mutex);
			wrenEnsureSlots(vm, 4);
			Creep* creep = wrenGetWeakPtr<Creep>(vm);
			if(!creep)
				return;
			if(WREN_TYPE_FOREIGN != wrenGetSlotType(vm, 1)){
				return;
			}
			RoomPosition* pos = static_cast<RoomPosition*>(wrenGetSlotForeign(vm, 1));
			bool ret = creep->findPath(*pos);
			wrenSetSlotBool(vm, 0, ret);
		};
	}
	else if(!isStatic && !strcmp(signature, "followPath()")){
		return [](WrenVM* vm){
			wxMutexLocker ml(wxGetApp().mutex);
			Creep* creep = wrenGetWeakPtr<Creep>(vm);
			if(!creep)
				return;
			wrenSetSlotBool(vm, 0, creep->followPath());
		};
	}
	return nullptr;
}
