function main(){
	local hasCreep = 1 < Game.creeps.len()
	if(!hasCreep){
		foreach(s in Game.spawns){
			local c = s.createCreep()
			if(c){
				local lastC = Game.creeps[Game.creeps.len()-1]
				print("Spawn created: " + lastC.id.tostring() + " when resource is " + s.resource.tostring() + ", ttl: " + lastC.ttl.tostring())
			}
		}
	}
}
