#include "Game.h"
#include "sqscripter-test.h"

extern "C"{
#include "clib/rseq.h"
}


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

void Game::init(){
	for(int i = 0; i < ROOMSIZE; i++){
		int di = i - ROOMSIZE / 2;
		for(size_t j = 0; j < ROOMSIZE; j++){
			int dj = j - ROOMSIZE / 2;
			room[i][j].type = perlin_noise_pixel(j, i, 3) < 1. - 1. / (1. + sqrt(di * di + dj * dj) / (ROOMSIZE / 2));
		}
	}

	for(int i = 0; i < 6; i++){
		int x, y;
		do{
			x = rand() % ROOMSIZE;
			y = rand() % ROOMSIZE;
		} while(room[y][x].type != 0);
		creeps.push_back(Creep(x, y, i % 2));
		room[y][x].object = creeps.back().id;
	}

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
		if(it->ttl <= 0){
			room[it->pos.y][it->pos.x].object = 0;
			creeps.erase(it);
		}
		it = next;
	}

	for(auto& it : spawns)
		it.update();

	for(auto& it : mines)
		it.update();

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
	if((double)rand() / RAND_MAX < 0.01 && mines.size() < 3){
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
}
