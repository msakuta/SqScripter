
// Since Squirrel 3.0, JSON is part of the language, so we can simply
// execute a JSON string to get the object representation.
function parseJson(json)
{
	local c = compilestring("return " + json);
	return c();
}
