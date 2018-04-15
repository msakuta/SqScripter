Game.creeps[0].memory = "Hey at %(Game.time)\n"

Game.main = Fn.new {
	if(Game.creeps.count < 1){
		Game.spawns[0].createCreep()
	}
	Game.print("Creep's memory: " + Game.creeps[0].memory)
}
