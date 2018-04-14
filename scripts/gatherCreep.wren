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
	var hasCreep = 1 < Game.creeps.count
	if(!hasCreep){
		for(s in Game.spawns){
			var c = s.createCreep()
			if(c){
				var lastC = Game.creeps[-1]
				Game.print("Spawn created: %(lastC.id) when resource is %(s.resource), ttl: %(lastC.ttl)\n")
			}
		}
	}

	var distanceOf = Fn.new {|c, m|
		var diffx = c.pos[0] - m.pos[0]
		var diffy = c.pos[1] - m.pos[1]
		return diffx.abs + diffy.abs
	}

	var tryApproach = Fn.new {|c, m|
		if(Game.time % 10 == 0){
			c.findPath(m.pos)
		}
		c.followPath()
	}

	for(c in Game.creeps){
		if(c.resource < 100){
			var mines = Game.mines
			var m = mines[random.int(mines.count)]
			var dist = distanceOf.call(c, m)
			if(dist <= 1){
				c.harvest(1)
				Game.print("harvesting %(c.id): resource: %(c.resource), dist: %(dist)\n")
			}else{
				tryApproach.call(c, m)
			}
		}else{
			var s = Game.spawns[c.owner]
			var dist = distanceOf.call(c, s)
			if(dist <= 1){
				c.store(1)
				Game.print("storing %(c.id): resource: %(c.resource), dist: %(dist)\n")
			}else{
				tryApproach.call(c, s)
			}
		}
	}
}

}