local circleFibers = []
foreach(it in Game.creeps){
	local creep = it
	circleFibers.append((function(){
		local dir = 0
		while(true){
			creep.move(dir + 1)
			//Game.print("amicalled dir:%(dir), pos:%(Game.creep(0).pos)\n")
			yield
			if(creep.owner != 0){
				dir = (dir + 8 - 1) % 8
			}else{
				dir = (dir + 1) % 8
			}
		}
	})())
}

function main(){
	//Game.print("amicalling\n")
	foreach(it in circleFibers){
		resume it
	}
}
