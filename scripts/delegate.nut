
local t = {
	"a": 101, "b": 102
}

local d = {
	function f(){print(a + b + c);}
	c = 103
}

t.setdelegate(d);

t.f();
