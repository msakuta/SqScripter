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
}
