
function main(){
	Game.creeps[0].move(Game.time % 8 + 1)
	Game.creeps[1].move((-Game.time - (-Game.time / 8) * 8 + 8) % 8 + 1)
}
