{
import "random" for Random

var directions = [
	[0,-1], // TOP = 1,
	[1,-1], // TOP_RIGHT = 2,
	[1,0], // RIGHT = 3,
	[1,1], // BOTTOM_RIGHT = 4,
	[0,1], // BOTTOM = 5,
	[-1,1], // BOTTOM_LEFT = 6,
	[-1,0], // LEFT = 7,
	[-1,-1], // TOP_LEFT = 8,
]

var random = Random.new(12345)

Game.main = Fn.new {
	for(s in Game.spawns){
		var creepCount = Game.creeps.count {|c| c.owner == s.owner}
		if(creepCount < 3){
			var c = s.createCreep()
			if(c){
				var lastC = Game.creeps[-1]
				Game.print("Spawn created: %(lastC.id) when resource is %(s.resource), ttl: %(lastC.ttl)\n")
			}
		}
	}

	var max = Fn.new {|a,b|
		return a < b ? b : a
	}

	var distanceOf = Fn.new {|c, m|
		var diffx = c.pos.x - m.pos.x
		var diffy = c.pos.y - m.pos.y
		//Game.print("diff: %(diffx), %(diffy), max: %(max.call(diffx.abs, diffy.abs) <= 1)\n")
		return max.call(diffx.abs, diffy.abs)
	}

	var tryApproach = Fn.new {|c, m|
		var ret = true
		if(Game.time % 10 == 0){
			ret = c.findPath(m.pos)
			// If path finding failed, clear the bad memory
			if(!ret){
				c.memory = null
			}
		}
		c.followPath()
		return ret
	}

	for(c in Game.creeps){
		if(c.memory == null){
			c.memory = {"mode": null, "target": null}
		}

		if(c.memory["mode"] == null){
			if(c.resource < 100){
				c.memory["mode"] = "gather"
			}else{
				var options = ["attack"]
				var spawn = Game.spawns[c.owner]
				if(spawn.resource < 10000){
					options.add("store")
				}
				if(0 < options.count){
					c.memory["mode"] = options[random.int(options.count)]
				}else{
					c.memory["mode"] = null
				}
			}
		}

		if(c.memory["mode"] == "gather"){
			var mines = Game.mines
			var m = Fn.new {
				if((c.memory["target"] == null || !c.memory["target"].alive) && mines.count){
					c.memory["target"] = 0 < mines.count ? mines[random.int(mines.count)] : null
				}
				if(c.memory["target"] != null && !c.memory["target"].alive){
					c.memory["target"] = null
				}
				return c.memory["target"]
			}.call()
			if(m != null){
				var dist = distanceOf.call(c, m)
				if(dist <= 1){
					c.harvest(1)
					Game.print("harvesting %(c.id): resource: %(c.resource), dist: %(dist)\n")
				}else{
					tryApproach.call(c, m)
				}
				if(100 <= c.resource){
					c.memory["mode"] = null
				}
			}
		}else if(c.memory["mode"] == "store"){
			if(c.resource == 0){
				c.memory["mode"] = null
			}
			c.memory["target"] = null
			var s = Game.spawns[c.owner]
			if(10000 <= s.resource){
				c.memory["mode"] = null
			}
			var dist = distanceOf.call(c, s)
			if(dist <= 1){
				c.store(1)
				Game.print("storing %(c.id): resource: %(c.resource), dist: %(dist)\n")
			}else{
				tryApproach.call(c, s)
			}
		}else if(c.memory["mode"] == "attack"){
			if(c.resource == 0){
				c.memory["mode"] = null
			}else{
				var enemies = Game.creeps.where {|c2| c2.owner != c.owner}.toList
				if(0 < enemies.count){
					var dist = distanceOf.call(c, enemies[0])
					if(dist <= 1){
						c.attack(1)
						Game.print("attacking %(c.id): resource: %(c.resource), dist: %(dist)\n")
					}else{
						tryApproach.call(c, enemies[0])
					}
				}else{
					c.memory["mode"] = null
				}
			}
		}
	}
}

}