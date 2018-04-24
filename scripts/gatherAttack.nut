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
	foreach(s in Game.spawns){
		local creepCount = Game.creeps.filter(@(idx,creep) creep.owner == s.owner).len()
		if(creepCount < 3){
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
		try{
			if(c.memory == null)
				c.memory = {"mode": null, "target": null}

			if(c.memory.mode == null){
				if(c.resource < 100)
					c.memory.mode = "gather"
				else{
					local options = ["attack"]
					local spawn = Game.spawns[c.owner]
					if(spawn.resource < 10000)
						options.append("store")
					if(options.len())
						c.memory.mode = options[rand() % options.len()]
					else
						c.memory.mode = null
				}
			}

			if(c.memory.mode == "gather"){
				local mines = Game.mines
				local m = (function(){
					if(c.memory == null)
						c.memory = {"mode": "gather", "target": null}
					if((c.memory.target == null || !c.memory.target.alive) && mines.len())
						c.memory.target = mines[rand() % mines.len()]
					if(c.memory.target != null && !c.memory.target.alive)
						c.memory.target = null
					return c.memory.target
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
				if(100 <= c.resource)
					c.memory.mode = null
			}
			else if(c.memory.mode == "store"){
				if(c.resource == 0)
					c.memory.mode = null
				c.memory.target = null
				local s = Game.spawns[c.owner]
				if(10000 <= s.resource)
					c.memory.mode = null
				local dist = distanceOf(c, s)
				if(dist <= 1){
					c.store(1)
					print("storing " + c.id.tostring() + ": resource: " + c.resource.tostring() + ", dist: " + dist)
				}
				else
					tryApproach(c, s)
			}
			else if(c.memory.mode == "attack"){
				if(c.resource == 0)
					c.memory.mode = null
				else{
					local enemy = Game.creeps.reduce(@(e,c2) c2.owner != c.owner ? c2 : null)
					if(enemy != null){
						local dist = distanceOf(c, enemy)
						if(dist <= 1){
							c.attack(1)
							print("attacking " + c.id.tostring() + ": resource: " + c.resource.tostring() + ", dist: " + dist)
						}
						else
							tryApproach(c, enemy)
					}
					else
						c.memory.mode = null
				}
			}
		}
		catch(e){
			print("Error: " + e.tostring() + " on " + c.id)
		}
	}
}
