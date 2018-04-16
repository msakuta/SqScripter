local directions = [
	[0,-1], // TOP = 1,
	[1,-1], // TOP_RIGHT = 2,
	[1,0], // RIGHT = 3,
	[1,1], // BOTTOM_RIGHT = 4,
	[0,1], // BOTTOM = 5,
	[-1,1], // BOTTOM_LEFT = 6,
	[-1,0], // LEFT = 7,
	[-1,-1], // TOP_LEFT = 8,
]

function main(){
	local hasCreep = 5 < Game.creeps.len()
	if(!hasCreep){
		foreach(s in Game.spawns){
			local c = s.createCreep()
			if(c){
				local lastC = Game.creeps[Game.creeps.len()-1]
				print("Spawn created: " + lastC.id.tostring() + " when resource is " + s.resource.tostring() + ", ttl: " + lastC.ttl.tostring())
			}
		}
	}

	local function max(a,b){
		return a < b ? b : a
	}

	local function distanceOf(c, m){
		local diffx = c.pos.x - m.pos.x
		local diffy = c.pos.y - m.pos.y
		return max(abs(diffx), abs(diffy))
	}

	local function tryApproach(c, m){
		if(Game.time % 10 == 0){
			local ret = c.findPath(m.pos)
			// If path finding failed, clear the bad memory
			if(!ret){
				c.memory = null
			}
		}
		c.followPath()
	}

	foreach(c in Game.creeps){
		if(c.resource < 100){
			local mines = Game.mines
			local m = (function(){
				if((c.memory == null || !c.memory.alive) && mines.len())
					c.memory = mines[rand() % mines.len()]
				if(c.memory != null && !c.memory.alive)
					c.memory = null
				return c.memory
			})()
			if(m == null)
				continue
			local dist = distanceOf(c, m)
			if(dist <= 1){
				c.harvest(1)
				print("harvesting " + c.id.tostring() + ": resource: " + c.resource.tostring() + ", dist: " + dist)
			}
			else{
				tryApproach(c, m)
			}
		}
		else{
			c.memory = null
			local s = Game.spawns[c.owner]
			local dist = distanceOf(c, s)
			if(dist <= 1){
				c.store(1)
				print("storing " + c.id.tostring() + ": resource: " + c.resource.tostring() + ", dist: " + dist)
			}
			else
				tryApproach(c, s)
		}
	}
}
