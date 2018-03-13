{
var circleFibers = []
for(it in Game.creeps){
	circleFibers.add(Fiber.new {
		var creep = it
		var dir = 0
		while(true){
			creep.move(dir + 1)
			//Game.print("amicalled dir:%(dir), pos:%(Game.creep(0).pos)\n")
			Fiber.yield()
			dir = (dir + 1) % 8
		}
	})
}

Game.main = Fn.new {
	//Game.print("amicalling\n")
	for(it in circleFibers){
		it.call()
	}
}
}