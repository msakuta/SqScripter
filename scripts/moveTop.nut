local circleFibers = []
foreach(it in Game.creeps){
	local creep = it
	circleFibers.append((function(){
		local dir = 0
		while(true){
			creep.move(5)
			//Game.print("amicalled dir:%(dir), pos:%(Game.creep(0).pos)\n")
			yield
		}
	})())
}

function main(){
	//Game.print("amicalling\n")
	foreach(it in circleFibers){
		resume it
	}
}
