Game.creeps[0].memory = "Hey at " + Game.time.tostring()

function main(){
	if(Game.creeps.len() < 1)
		Game.spawns[0].createCreep()
	print("Creep's memory: " + Game.creeps[0].memory)
}
