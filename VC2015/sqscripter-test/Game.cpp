#include "Game.h"
#include "sqscripter-test.h"

extern "C"{
#include "clib/rseq.h"
}

const SQUserPointer Game::Race::typetag = _SC("Race");
int Game::Race::id_gen = 0;

Game game;

static double noise_pixel(int x, int y, int bit){
	struct random_sequence rs;
	initfull_rseq(&rs, x + (bit << 16), y);
	return drseq(&rs);
}

double perlin_noise_pixel(int x, int y, int bit){
	int ret = 0, i;
	double sum = 0., maxv = 0., f = 1.;
	double persistence = 0.5;
	for(i = 3; 0 <= i; i--){
		int cell = 1 << i;
		double a00, a01, a10, a11, fx, fy;
		a00 = noise_pixel(x / cell, y / cell, bit);
		a01 = noise_pixel(x / cell, y / cell + 1, bit);
		a10 = noise_pixel(x / cell + 1, y / cell, bit);
		a11 = noise_pixel(x / cell + 1, y / cell + 1, bit);
		fx = (double)(x % cell) / cell;
		fy = (double)(y % cell) / cell;
		sum += ((a00 * (1. - fx) + a10 * fx) * (1. - fy)
			+ (a01 * (1. - fx) + a11 * fx) * fy) * f;
		maxv += f;
		f *= persistence;
	}
	return sum / maxv;
}


bool Game::isBlocked(const RoomPosition & pos)const
{
	if(pos.x < 0 || ROOMSIZE <= pos.x || pos.y < 0 || ROOMSIZE <= pos.y)
		return true;
	const Tile& tile = room[pos.y][pos.x];
	if(tile.type || tile.object)
		return true;
	/*	for(auto& it : creeps){
	if(it.pos == pos)
	return true;
	}
	for(auto& it : spawns){
	if(it.pos == pos)
	return true;
	}
	for(auto& it : mines){
	if(it.pos == pos)
	return true;
	}*/
	return false;
}

/// Fill cost field of room tiles from a given position.
/// Essentially the same algorithm with findPath() except that it doesn't have the "to" parameter.
/// If there are unreachable tiles in the room, they will be painted with INT_MAX.
/// Used for filling unreachable tiles with walls, since randomly generated objects may happen to be
/// located on these tiles.
static void paintReachable(const RoomPosition& from, const RoomObject* self){
	// List of positions to start calculating cost.
	// Using std::vector to cache the next candidates is surprisingly efficient,
	// because the number of target positions in one iteration is orders of magnitude smaller than the total number of tiles.
	// Also surprisingly, we don't need std::set to exclude duplicates, because the same candidate is already marked solved by assigning
	// non-infinity cost.  If we used std::set, it would take cost in performance to build hash table.
	std::vector<RoomPosition> targetList[2];
	auto currentTargets = &targetList[0];
	auto nextTargets = &targetList[1];

	auto internal = [self, &currentTargets, &nextTargets](){
		int changes = 0;
		for(auto& it : *currentTargets){
			int x = it.x;
			int y = it.y;
			Tile &tile = game.room[y][x];
			if(tile.cost == INT_MAX)
				continue;
			for(int y1 = std::max(y - 1, 0); y1 <= std::min(y + 1, ROOMSIZE - 1); y1++){
				for(int x1 = std::max(x - 1, 0); x1 <= std::min(x + 1, ROOMSIZE - 1); x1++){
					Tile &tile1 = game.room[y1][x1];

					// Check if the path is blocked by another creep.
					if(tile1.object != 0 && self && tile1.object != self->id)
						continue;

					if(tile1.type == 0 && tile.cost + 1 < tile1.cost){
						tile1.cost = tile.cost + 1;
						nextTargets->push_back(RoomPosition(x1, y1));
						changes++;
					}
				}
			}
		}
		return changes;
	};

	currentTargets->push_back(from);

	for(int y = 0; y < ROOMSIZE; y++){
		for(int x = 0; x < ROOMSIZE; x++){
			Tile &tile = game.room[y][x];
			tile.cost = (x == from.x && y == from.y ? 0 : INT_MAX);
		}
	}

	std::vector<PathNode> ret;

	while(internal()){
		// Swap the buffers for the next iteration.
		// std::vector::clear() keeps the reserved buffer, which is very efficient to reuse.
		currentTargets->clear();
		std::swap(currentTargets, nextTargets);
	}
}


void Game::init(){
	for(int i = 0; i < ROOMSIZE; i++){
		int di = i - ROOMSIZE / 2;
		for(size_t j = 0; j < ROOMSIZE; j++){
			int dj = j - ROOMSIZE / 2;
			room[i][j].type = perlin_noise_pixel(j, i, 3) < 1. - 1. / (1. + sqrt(di * di + dj * dj) / (ROOMSIZE / 2));
		}
	}

	paintReachable(RoomPosition(ROOMSIZE / 2, ROOMSIZE / 2), nullptr);

	// Paint unreachable tiles from the center of the room black.
	// It could be anywhere, but walkable tiles must be singly connected.
	for(int i = 0; i < ROOMSIZE; i++){
		int di = i - ROOMSIZE / 2;
		for(size_t j = 0; j < ROOMSIZE; j++){
			int dj = j - ROOMSIZE / 2;
			if(room[i][j].type != 1 && room[i][j].cost == INT_MAX)
				room[i][j].type = 1;
		}
	}

#if 0
	for(int i = 0; i < 6; i++){
		int x, y;
		do{
			x = rand() % ROOMSIZE;
			y = rand() % ROOMSIZE;
		} while(room[y][x].type != 0);
		creeps.push_back(Creep(x, y, i % 2));
		room[y][x].object = creeps.back().id;
	}
#endif

	for(int i = 0; i < 2; i++){
		int x, y;
		do{
			x = rand() % ROOMSIZE;
			y = rand() % ROOMSIZE;
		} while(room[y][x].type != 0);
		spawns.push_back(Spawn(x, y, i));
		room[y][x].object = spawns.back().id;
	}

	for(int i = 0; i < 2; i++){
		int x, y;
		do{
			x = rand() % ROOMSIZE;
			y = rand() % ROOMSIZE;
		} while(room[y][x].type != 0);
		mines.push_back(Mine(x, y));
		room[y][x].object = mines.back().id;
	}
}

static
Game::RoomPositionT operator+(const Game::RoomPositionT& a, const Game::RoomPositionT& b) {
	return { a[0] + b[0], a[1] + b[1] };
}

void Game::update(){

	global_time++;

	for(auto& it : creeps){
		it.update();
		/*		if(it.path.size() == 0){
		Spawn *spawn = [](int owner){
		for(auto& it : spawns)
		if(it.owner == owner)
		return &it;
		return (Spawn*)NULL;
		}(it.owner);
		if(spawn)
		it.path = findPath(it.pos, spawn->pos);
		}
		if(it.path.size()){
		RoomPosition newPos = it.path.back().pos;
		if(isBlocked(newPos))
		continue;
		it.path.pop_back();
		it.pos = newPos;
		}
		else{
		RoomPosition newPos(it.pos.x + rand() % 3 - 1, it.pos.y + rand() % 3 - 1);
		if(isBlocked(newPos))
		continue;
		it.pos = newPos;
		}*/
	}

	// Delete pass
	for(auto it = creeps.begin(); it != creeps.end();){
		auto next = it;
		++next;
		if(it->ttl <= 0 || it->health <= 0){
			room[it->pos.y][it->pos.x].object = 0;
			creeps.erase(it);
		}
		it = next;
	}

	for(auto& it : spawns)
		it.update();

	for(auto& it : mines)
		it.update();

	auto& first = spawns.begin();
	if (first != spawns.end() && room[first->pos.y][first->pos.x].tag == 0) {
		room[first->pos.y][first->pos.x].tag = 1;
		visitList.emplace(RoomPositionT{ first->pos.x, first->pos.y }, 0.);
		for (auto& it : distanceMap) {
			for (auto& it2 : it) {
				it2 = DBL_MAX;
			}
		}
		distanceMap[first->pos.y][first->pos.x] = 0.;
	}

	if(!visitList.empty()){
		auto closest = visitList.top();
		visitList.pop();

		auto boundary = [](const RoomPositionT& pos) {
			RoomPositionT ret = pos;
			for (int i = 0; i < 2; i++) {
				if (ret[i] < 0)
					ret[i] = 0;
				else if (ROOMSIZE <= ret[i])
					ret[i] = ROOMSIZE - 1;
			}
			return ret;
		};

		static const RoomPositionT nextDirs[] = {
			{-1, 0}, {0, -1}, {1, 0}, {0, 1},
		};
		for (auto& dir : nextDirs) {
			auto nextPos = RoomPositionT(closest);
			nextPos[0] += dir[0];
			nextPos[1] += dir[1];
			if (0 <= nextPos[0] && nextPos[0] < ROOMSIZE && 0 <= nextPos[1] && nextPos[1] < ROOMSIZE &&
				room[nextPos[1]][nextPos[0]].type == 0 &&
				room[nextPos[1]][nextPos[0]].tag == 0) {

				auto left = boundary(nextPos + std::array<int, 2>{-1, 0});
				auto right = boundary(nextPos + std::array<int, 2>{1, 0});
				auto up = boundary(nextPos + std::array<int, 2>{0, -1});
				auto down = boundary(nextPos + std::array<int, 2>{0, 1});
				double dx = std::min(distanceMap[left[1]][left[0]], distanceMap[right[1]][right[0]]);
				double dy = std::min(distanceMap[up[1]][up[0]], distanceMap[down[1]][down[0]]);
				double Delta = 2. - (dx - dy) * (dx - dy);
				double nextDist;
				if(Delta >= 0)
					nextDist = (dx + dy + sqrt(Delta)) / 2;
				else
					nextDist = std::min(dx + 1, dy + 1);

				visitList.emplace(nextPos, nextDist);
				room[nextPos[1]][nextPos[0]].tag = (int)nextDist;
				distanceMap[nextPos[1]][nextPos[0]] = nextDist;
			}
		}
	}

	// Delete pass
	for(auto it = mines.begin(); it != mines.end();){
		auto next = it;
		++next;
		if(it->resource <= 0){
			room[it->pos.y][it->pos.x].object = 0;
			mines.erase(it);
		}
		it = next;
	}

	// Create a random mine every 100 frames
	if((double)rand() / RAND_MAX < 0.03 && mines.size() < 3){
		int x, y, tries = 0;
		static const int max_tries = 100;
		do{
			x = rand() % ROOMSIZE;
			y = rand() % ROOMSIZE;
		} while(isBlocked(RoomPosition(x, y)) && tries++ < max_tries);
		if(tries < max_tries){
			mines.push_back(Mine(x, y));
			room[y][x].object = mines.back().id;
		}
	}
}

void Game::sq_define(HSQUIRRELVM sqvm){
	sq_pushstring(sqvm, _SC("Race"), -1);
	sq_newclass(sqvm, SQFalse);
	sq_settypetag(sqvm, -1, Race::typetag);
	sq_setclassudsize(sqvm, -1, sizeof(Race));
	sq_pushstring(sqvm, _SC("_get"), -1);
	sq_newclosure(sqvm, [](HSQUIRRELVM v){
		wxMutexLocker ml(wxGetApp().mutex);
		SQUserPointer p;
		Race *race;
		if(SQ_FAILED(sq_getinstanceup(v, 1, &p, Race::typetag)) || !(race = static_cast<Race*>(p)))
			return sq_throwerror(v, _SC("Broken Creep instance"));
		const SQChar *key;
		if(SQ_FAILED(sq_getstring(v, -1, &key)))
			return sq_throwerror(v, _SC("get key is missing"));
		if(!scstrcmp(key, _SC("id"))){
			sq_pushinteger(v, race->id);
			return SQInteger(1);
		}
		else if(!scstrcmp(key, _SC("id"))){
			sq_pushinteger(v, race->id);
			return SQInteger(1);
		}
		else if(!scstrcmp(key, _SC("kills"))){
			sq_pushinteger(v, race->kills);
			return SQInteger(1);
		}
		else if(!scstrcmp(key, _SC("deaths"))){
			sq_pushinteger(v, race->deaths);
			return SQInteger(1);
		}
		return SQInteger(1);
	}, 0);
	sq_newslot(sqvm, -3, SQFalse);
	sq_newslot(sqvm, -3, SQFalse);

	sq_pushstring(sqvm, _SC("Game"), -1);
	sq_newclass(sqvm, SQFalse);
	sq_settypetag(sqvm, -1, "Game");
	sq_pushstring(sqvm, _SC("_get"), -1);
	sq_newclosure(sqvm, &Game::sqf_get, 0);
	sq_newslot(sqvm, -3, SQTrue);
	// Register the singleton instance instead of the class
	// because _get() method only works for instances.
	sq_createinstance(sqvm, -1);
	sq_remove(sqvm, -2);
	sq_newslot(sqvm, -3, SQFalse);
}

SQInteger Game::sqf_get(HSQUIRRELVM v){
	wxMutexLocker ml(wxGetApp().mutex);
	const SQChar *key;
	if(SQ_FAILED(sq_getstring(v, -1, &key)))
		return sq_throwerror(v, _SC("get key is missing"));
	if(!scstrcmp(key, _SC("time"))){
		sq_pushinteger(v, game.global_time);
		return 1;
	}
	else if(!scstrcmp(key, _SC("creeps"))){
		size_t sz = game.creeps.size();
		SQInteger i = 0;
		sq_newarray(v, sz);
		for(auto& it : game.creeps){
			sq_pushroottable(v); // root
			sq_pushstring(v, _SC("Creep"), -1); // root "Creep"
			if(SQ_FAILED(sq_get(v, -2))) // root Creep i
				return sq_throwerror(v, _SC("Failed to create creeps array"));
			sq_pushinteger(v, i++); // root Creep i
			sq_createinstance(v, -2); // root Creep i inst
			SQUserPointer up;
			sq_getinstanceup(v, -1, &up, Creep::typetag); // array Spawn i inst
			WeakPtr<Creep>* wp = new(up) WeakPtr<Creep>(&it);
			sq_setreleasehook(v, -1, [](SQUserPointer up, SQInteger size){
				WeakPtr<Creep>* wp = (WeakPtr<Creep>*)up;
				wp->~WeakPtr<Creep>();
				return SQInteger(1);
			});
			if(SQ_FAILED(sq_set(v, -5))) // root Creep
				return sq_throwerror(v, _SC("Failed to create creeps array"));
			sq_pop(v, 2);
		}
		return 1;
	}
	else if(!scstrcmp(key, _SC("spawns"))){
		size_t sz = game.spawns.size();
		SQInteger i = 0;
		sq_newarray(v, sz); // array
		for(auto& it : game.spawns){
			sq_pushobject(v, Spawn::spawnClass); // array Spawn
			sq_pushinteger(v, i++); // array Spawn i
			sq_createinstance(v, -2); // array Spawn i inst
			SQUserPointer up;
			sq_getinstanceup(v, -1, &up, Spawn::typetag); // array Spawn i inst
			WeakPtr<Spawn>* wp = new(up) WeakPtr<Spawn>(&it);
			sq_setreleasehook(v, -1, [](SQUserPointer up, SQInteger size){
				WeakPtr<Spawn>* wp = (WeakPtr<Spawn>*)up;
				wp->~WeakPtr<Spawn>();
				return SQInteger(1);
			});
			if(SQ_FAILED(sq_set(v, -4))) // array Spawn
				return sq_throwerror(v, _SC("Failed to create creeps array"));
			sq_pop(v, 1); // array
		}
		return 1;
	}
	else if(!scstrcmp(key, _SC("mines"))){
		size_t sz = game.mines.size();
		SQInteger i = 0;
		sq_newarray(v, sz); // array
		for(auto& it : game.mines){
			sq_pushobject(v, Mine::mineClass); // array Mine
			sq_pushinteger(v, i++); // array Mine i
			sq_createinstance(v, -2); // array Mine i inst
			SQUserPointer up;
			sq_getinstanceup(v, -1, &up, Mine::typetag); // array Mine i inst
			WeakPtr<Mine>* wp = new(up) WeakPtr<Mine>(&it);
			sq_setreleasehook(v, -1, [](SQUserPointer up, SQInteger size){
				WeakPtr<Mine>* wp = (WeakPtr<Mine>*)up;
				wp->~WeakPtr<Mine>();
				return SQInteger(1);
			});
			if(SQ_FAILED(sq_set(v, -4))) // array Mine
				return sq_throwerror(v, _SC("Failed to create creeps array"));
			sq_pop(v, 1); // array
		}
		return 1;
	}
	else if(!scstrcmp(key, _SC("races"))){
		size_t sz = sizeof(game.races) / sizeof(*game.races);
		SQInteger i = 0;
		sq_newarray(v, sz); // array
		for(auto& it : game.races){
			sq_pushroottable(v); // array root
			sq_pushstring(v, _SC("Race"), -1); // array root "Race"
			if(SQ_FAILED(sq_get(v, -2))) // array root Race
				return sq_throwerror(v, _SC("Failed to find Race class"));
			//sq_pushobject(v, Mine::mineClass); // array Mine
			sq_remove(v, -2); // array Race
			sq_pushinteger(v, i++); // array Race i
			sq_createinstance(v, -2); // array Race i inst
			SQUserPointer up;
			if(SQ_FAILED(sq_getinstanceup(v, -1, &up, Race::typetag))) // array Race i inst
				return sq_throwerror(v, _SC("Failed to create Race instance"));
			Race* race = new(up) Race(it);
			if(SQ_FAILED(sq_set(v, -4))) // array Race
				return sq_throwerror(v, _SC("Failed to create races array"));
			sq_pop(v, 1); // array
		}
		return 1;
	}
	else
		return sq_throwerror(v, _SC("Index not found"));
}


static void wrenPrint(WrenVM* vm)
{
	if(WREN_TYPE_STRING != wrenGetSlotType(vm, 1)){
		PrintProc(sw, "Game.print() needs a string argument");
		return;
	}
	const char *s = wrenGetSlotString(vm, 1);
	PrintProc(sw, s);
}

static void wrenGetTime(WrenVM* vm)
{
	wrenEnsureSlots(vm, 1);
	wrenSetSlotDouble(vm, 0, game.global_time);
}

WrenForeignMethodFn Game::wren_bind(WrenVM * vm, bool isStatic, const char * signature){
	if(isStatic && strcmp(signature, "print(_)") == 0)
	{
		return wrenPrint;
	}
	else if(isStatic && strcmp(signature, "time") == 0)
	{
		return wrenGetTime;
	}
	else if(isStatic && strcmp(signature, "creep(_)") == 0)
	{
		return [](WrenVM* vm){
			if(WREN_TYPE_NUM != wrenGetSlotType(vm, 1)){
				return;
			}
			int idx = (int)wrenGetSlotDouble(vm, 1);
			if(idx < 0 || game.creeps.size() <= idx){
				return;
			}
			wrenEnsureSlots(vm, 2);
			wrenGetVariable(vm, "main", "Creep", 1);
			void *pp = wrenSetSlotNewForeign(vm, 0, 1, sizeof(WeakPtr<Creep>));
			auto cp = game.creeps.begin();
			for(int i = 0; i < idx && cp != game.creeps.end(); ++cp, ++i);
			new(pp) WeakPtr<Creep>(&*cp);
		};
	}
	else if(isStatic && strcmp(signature, "creeps") == 0)
	{
		return [](WrenVM* vm){
			// Slots: [array] main.Creep [instance]
			wrenEnsureSlots(vm, 3);
			wrenSetSlotNewList(vm, 0);
			wrenGetVariable(vm, "main", "Creep", 1);
			for(auto& it : game.creeps){
				void *pp = wrenSetSlotNewForeign(vm, 2, 1, sizeof(WeakPtr<Creep>));
				new(pp) WeakPtr<Creep>(&it);
				wrenInsertInList(vm, 0, -1, 2);
			}
		};
	}
	else if(isStatic && strcmp(signature, "spawns") == 0){
		return [](WrenVM* vm){
			// Slots: [array] main.Creep [instance]
			wrenEnsureSlots(vm, 3);
			wrenSetSlotNewList(vm, 0);
			wrenGetVariable(vm, "main", "Spawn", 1);
			for(auto& it : game.spawns){
				void *pp = wrenSetSlotNewForeign(vm, 2, 1, sizeof(WeakPtr<Spawn>));
				new(pp) WeakPtr<Spawn>(&it);
				wrenInsertInList(vm, 0, -1, 2);
			}
		};
	}
	else if(isStatic && strcmp(signature, "mines") == 0){
		return [](WrenVM* vm){
			// Slots: [array] main.Creep [instance]
			wrenEnsureSlots(vm, 3);
			wrenSetSlotNewList(vm, 0);
			wrenGetVariable(vm, "main", "Mine", 1);
			for(auto& it : game.mines){
				void *pp = wrenSetSlotNewForeign(vm, 2, 1, sizeof(WeakPtr<Mine>));
				new(pp) WeakPtr<Mine>(&it);
				wrenInsertInList(vm, 0, -1, 2);
			}
		};
	}
	else if(isStatic && strcmp(signature, "races") == 0){
		return [](WrenVM* vm){
			// Slots: [array] main.Creep [instance]
			wrenEnsureSlots(vm, 3);
			wrenSetSlotNewList(vm, 0);
			wrenGetVariable(vm, "main", "Race", 1);
			for(auto& it : game.races){
				void *pp = wrenSetSlotNewForeign(vm, 2, 1, sizeof(Race));
				new(pp) Race(it);
				wrenInsertInList(vm, 0, -1, 2);
			}
		};
	}
}
