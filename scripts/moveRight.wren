{
var circleFibers = []
for(it in Game.creeps){
	circleFibers.add(Fiber.new {
		var creep = it
		var dir = 0
		while(true){
			creep.move(3)
			//Game.print("amicalled dir:%(dir), pos:%(Game.creep(0).pos)\n")
			Fiber.yield()
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