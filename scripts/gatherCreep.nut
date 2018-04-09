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
	local hasCreep = 0 < Game.creeps.len()
	if(!hasCreep){
		foreach(s in Game.spawns){
			local c = s.createCreep()
			if(c){
				local lastC = Game.creeps[Game.creeps.len()-1]
				print("Spawn created: " + lastC.id.tostring() + " when resource is " + s.resource.tostring() + ", ttl: " + lastC.ttl.tostring())
			}
		}
	}

	local function distanceOf(c, m){
		local diffx = c.pos.x - m.pos.x
		local diffy = c.pos.y - m.pos.y
		return abs(diffx) + abs(diffy)
	}

	local function tryApproach(c, m){
		local dx = m.pos.x < c.pos.x ? -1 : m.pos.x > c.pos.x ? 1 : 0
		local dy = m.pos.y < c.pos.y ? -1 : m.pos.y > c.pos.y ? 1 : 0
		for(local i = 0; i < directions.len(); i++){
			if(directions[i][0] == dx && directions[i][1] == dy){
				c.move(i+1)
				print("Moving " + c.id.tostring() + ": " + i)
				break
			}
		}
	}

	foreach(c in Game.creeps){
		if(c.resource < 100){
			local m = Game.mines[0]
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
