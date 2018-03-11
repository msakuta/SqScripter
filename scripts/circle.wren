Game.main = Fn.new {
	Game.creep(0).move(Game.time % 8 + 1)
	Game.creep(1).move((-Game.time - (-Game.time / 8) * 8 + 8) % 8 + 1)
}
