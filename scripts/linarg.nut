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

/// @brief Quaternion for representing 3-D rotation.
class Quat{
	x = 0; // Imaginary part i
	y = 0; // Imaginary part j
	z = 0; // Imaginary part k
	w = 1; // Real part
	constructor(x = 0, y = 0, z = 0, w = 1){
		this.x = x;
		this.y = y;
		this.z = z;
		this.w = w;
	}
	function _tostring(){
		return "[" + x.tostring() + "," + y.tostring() + "," + z.tostring() + "," + w.tostring() + "]";
	}
	function _add(o){
		return Quat(x + o.x, y + o.y, z + o.z, w + o.w);
	}
	function _sub(o){
		return Quat(x - o.x, y - o.y, z - o.z, w - o.w);
	}
	function _unm(){
		return Quat(-x, -y, -z, -w);
	}
	function _mul(o){
		if(typeof(o) == "integer" || typeof(o) == "float")
			return Quat(x * o, y * o, z * o, w * o);
		else if(o instanceof Quat)
			return Quat(y * o.z - z * o.y + x * o.w + w * o.x,
				z * o.x - x * o.z + y * o.w + w * o.y,
				x * o.y - y * o.x + z * o.w + w * o.z,
				-x * o.x - y * o.y - z * o.z + w * o.w);
		else
			throw "Quaternion multiplicaton with non-numeric value";
	}
	function _div(o){
		// Should quaternion scale even allowed!?
		if(typeof(o) == "integer" || typeof(o) == "float")
			return Quat(x / o.tofloat(), y / o.tofloat(), z / o.tofloat(), w / o.tofloat());
		else if(o instanceof Quat)
			return this * o.cnj();
		else
			throw "Quaternion multiplicaton with non-supported operandnon-numeric value";
	}
	function _nexti(i){
		if(i == null)
			return "x";
		return {
			["x"] = "y",
			["y"] = "z",
			["z"] = "w",
			w = null,
		}[i];
	}
	lengthSq = @() x * x + y * y + z * z + w * w;
	length = @() sqrt(lengthSq());
	function normalize(){
		local il = 1. / length();
		x *= il; y *= il; z *= il; w *= il;
		return this;
	}
	normalized = @() (clone this).normalize();
	cnj = @() Quat(-x,-y,-z,w);
	dot = @(o) x * o.x + y * o.y + z * o.z + w * o.w;

	/// @brief Transformation of a vector by this quaternion as rotation
	///
	/// This is not overloaded in _mul because multiplication and transformation
	/// are not the same thing in case of quaternion.
	function trans(o){
		if(o instanceof Vec){
			local qc = cnj();
			local r = Quat(o.x, o.y, o.z, 0);
			local qr = this * r;
			local qret = qr * qc;
			return Vec(qret.x, qret.y, qret.z);
		}
		else
			throw "Quat.trans() needs vector argument";
	}

	/// @brief Returns rotation quaternion around given axis with given angle
	static function rotation(axis, angle){
		local len = sin(angle / 2.);
		return Quat(len * axis.x, len * axis.y, len * axis.z, cos(angle / 2.));
	}

	/// @brief Returns rotation which makes [0,0,1] to point given direction vector
	static function direction(dir){
		if(!(dir instanceof Vec))
			throw "Quat.direction() needs vector argument";
		local epsilon = 1e-5;
		local ret = Quat();
		local dr = dir.normalized();

		/* half-angle formula of trigonometry replaces expensive tri-functions to square root */
		ret.w = ::sqrt((dr.z + 1) / 2) /*cos(acos(dr[2]) / 2.)*/;

		if(1 - epsilon < ret.w){
			ret.x = ret.y = ret.z = 0;
		}
		else if(ret.w < epsilon){
			ret.x = ret.z = 0;
			ret.y = 1;
		}
		else{
			local v = Vec(0,0,1).cross(dr);
			local p = sqrt(1 - ret.w * ret.w) / (v.length());
			ret.x = v.x * p;
			ret.y = v.y * p;
			ret.z = v.z * p;
		}
		return ret;
	}

	/// @brief Spherical linear interpolation between two quaternions q, r with ratio t in range [0,1]
	static function slerp(q, r, t){
		local epsilon = 1e-5;
		local ret;
		local qr = q.dot(this);
		local ss = 1.0 - qr * qr;

		if (ss <= epsilon) {
			return q;
		}
		else if(q == r){
			return q;
		}
		else {
			local sp = ::sqrt(ss);

			local ph = ::acos(qr);
			local pt = ph * t;
			local t1 = ::sin(pt) / sp;
			local t0 = ::sin(ph - pt) / sp;

			// Long path case
			if(qr < 0)
				t1 *= -1;

			return Quat(
				q.x * t0 + r.x * t1,
				q.y * t0 + r.y * t1,
				q.z * t0 + r.z * t1,
				q.w * t0 + r.w * t1);
		}
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

local q = Quat();
print("Identity quaternion: " + q);
q = Quat.rotation(Vec(0,1,0), PI / 2);
print("Rotation quaternion: " + q);
print("Rotated vector: " + q.trans(Vec(1,0,0)));
local q2 = Quat.rotation(Vec(0,0,1), PI / 2);
print("Combined rotation: " + (q * q2));
print("Rotated vector by combined rotation: " + (q * q2).trans(Vec(1,0,0)));
print("Rotation to face direction: " + Quat.direction(Vec(1,1,1)));
print("Slerp between two quaternions: " + Quat.slerp(q, q2, 0.5));

print("Dumping vector contents: ");
foreach(k,v in Vec(10,11,12))
	print("\t" + k + ": " + v);

print("Dumping matrix contents: ");
foreach(k,v in m)
	print("\t" + k + ": " + v);

print("Dumping quaternion contents: ");
foreach(k,v in q)
	print("\t" + k + ": " + v);

print("Dumping vector methods and default members: ");
foreach(k,v in Vec)
	print("\t" + k + ": " + v);

print("Dumping matrix methods and default members: ");
foreach(k,v in Mat)
	print("\t" + k + ": " + v);

print("Dumping quaternion methods and default members: ");
foreach(k,v in Quat)
	print("\t" + k + ": " + v);
