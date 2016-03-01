// Linear algebra library implementation test

/// @brief 3-D vector
class Vec{
	x = 0;
	y = 0;
	z = 0;
	constructor(x = 0, y = 0, z = 0){
		this.x = x;
		this.y = y;
		this.z = z;
	}
	function _tostring(){
		return "[" + x.tostring() + "," + y.tostring() + "," + z.tostring() + "]";
	}
	function _add(o){
		return Vec(x + o.x, y + o.y, z + o.z);
	}
	function _sub(o){
		return Vec(x - o.x, y - o.y, z - o.z);
	}
	function _unm(){
		return Vec(-x, -y, -z);
	}
	function _mul(o){
		if(typeof(o) == "integer" || typeof(o) == "float")
			return Vec(x * o, y * o, z * o);
		else
			throw "Vector multiplicaton with non-numeric value";
	}
	function _div(o){
		if(typeof(o) == "integer" || typeof(o) == "float")
			return Vec(x / o.tofloat(), y / o.tofloat(), z / o.tofloat());
		else
			throw "Vector multiplicaton with non-numeric value";
	}
	function _nexti(i){
		if(i == null)
			return "x";
		return {
			["x"] = "y",
			["y"] = "z",
			["z"] = null,
		}[i];
	}
	lengthSq = @() x * x + y * y + z * z;
	length = @() sqrt(lengthSq());
	function normalize(){
		local il = 1. / length();
		x *= il; y *= il; z *= il;
		return this;
	}
	normalized = @() (clone this).normalize();
	dot = @(o) x * o.x + y * o.y + z * o.z;
	cross = @(o) Vec(y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x);
}

/// @brief 3-D augmented matrix
class Mat{
	a = null;
	constructor(){
		a = [1,0,0,0,1,0,0,0,1,0,0,0];
	}
	function _tostring(){
		local ret = "[";
		for(local i = 0; i < a.len(); i++)
			ret += (i == a.len() - 1 ? a[i] : a[i] + ",");
		return ret + "]";
	}
	function _mul(o){
		if(typeof o == "integer" || typeof o == "float"){
			local ret = ::Mat();
			foreach(i,k in a)
				ret.a[i] = a[i] * o;
			return ret;
		}
		else if(typeof o == "array"){
			return ::Vec(
				a[0] * o[0] + a[3] * o[1] + a[6] * o[2] + a[9],
				a[1] * o[0] + a[4] * o[1] + a[7] * o[2] + a[10],
				a[2] * o[0] + a[5] * o[1] + a[8] * o[2] + a[11]);
		}
		else if(o instanceof ::Vec){
			return ::Vec(
				a[0] * o.x + a[3] * o.y + a[6] * o.z + a[9],
				a[1] * o.x + a[4] * o.y + a[7] * o.z + a[10],
				a[2] * o.x + a[5] * o.y + a[8] * o.z + a[11]);
		}
		else if(o instanceof ::Mat){
			local ret = ::Mat();
			for(local i = 0; i < 3; i++)
				ret.setColumn(i, this * o.column(i));
			return ret;
		}
		else
			throw "Unsupported multiplication operand for Mat";
	}
	function _div(o){
		if(typeof o == "integer" || typeof o == "float"){
			local f = o.tofloat();
			local ret = ::Mat();
			foreach(i,k in a)
				ret.a[i] = a[i] / f;
			return ret;
		}
		else
			throw "Unsupported division operand for Mat";
	}
	function _nexti(i){
		if(i == null)
			return 0;
		else if(i == a.len()-1)
			return null;
		else
			return i+1;
	}
	function _get(i){
		if(0 <= i && i < a.len())
			return a[i];
		else
			throw null;
	}
	function _set(i, v){
		if(0 <= i && i < a.len())
			a[i] = v;
		else
			throw null;
	}
	function column(i){
		return ::Vec(a[i * 3], a[i * 3 + 1], a[i * 3 + 2]);
	}
	function setColumn(i,v){
		if(typeof v == "array"){
			a[i * 3] = v[0]; a[i * 3 + 1] = v[1]; a[i * 3 + 2] = v[2];
		}
		else{
			a[i * 3] = v.x; a[i * 3 + 1] = v.y; a[i * 3 + 2] = v.z;
		}
	}
	static function rotation(axis, angle){
		local ret = Mat();
		for(local i = 0; i < 3; i++){
			local a = array(3);
			a[axis % 3] = [cos(angle), -sin(angle), 0][i];
			a[(axis+1) % 3] = [sin(angle), cos(angle), 0][i];
			a[(axis+2) % 3] = [0,0,1][i];
			ret.setColumn((i + axis) % 3, a);
		}
		return ret;
	}
}

local v = -Vec(1,2,3);
v /= 3;
print("Vector: " + v);
print("Squared length: " + v.lengthSq());
print("Length: " + v.length());
print("Copy normalization: " + v.normalized());
print("Length after copy normalization: " + v.length());
v.normalize();
print("Length after normalization: " + v.length());
print("Subtracted vector: " + (v - Vec(4,5,6)));
print("Dot product: " + Vec(1,2,0).dot(Vec(3,1,0)));
print("Cross product: " + Vec(1,0,0).cross(Vec(0,1,0)));

local m = Mat();
m[5] = 2;
print("Matrix: " + m)
print("Identity matrix: " + Mat())
print("Scaled matrix: " + m / 3);

print("Transformed vector: " + m * v);
print("Matrix multiplication: " + m * Mat.rotation(0, PI/3));
print("Transforming array vector: " + m * [1,2,3]);

print("Dumping vector contents: ");
foreach(k,v in Vec(10,11,12))
	print(k + ": " + v);

print("Dumping matrix contents: ");
foreach(k,v in m)
	print(k + ": " + v);

print("Dumping vector methods and default members: ");
foreach(k,v in Vec)
	print(k + ": " + v);

print("Dumping matrix methods and default members: ");
foreach(k,v in Mat)
	print(k + ": " + v);
