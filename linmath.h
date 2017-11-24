#ifndef LINMATH_H
#define LINMATH_H

/// Mat4 are in column-major order

#include <cmath>
#include <limits>

/// Common functions & math
#ifndef M_PI
#define M_PI 3.14159265358
#endif
#ifndef M_PI_OVER_TWO
#define M_PI_OVER_TWO 1.5707963267
#define M_PI_OVER_FOUR 0.78539816339

#define M_TWO_PI 6.28318530718
#define M_FOUR_PI 12.5663706144

#define M_INV_PI 0.31830988618
#define M_INV_TWO_PI 0.15915494309
#define M_INV_FOUR_PI 0.07957747154 
#endif
// deg 2 rad : d * PI / 180
#define DEG2RAD 0.01745329252
// rad 2 deg : d * 180 / PI
#define RAD2DEG 57.2957795

#define Square(num) ((num)*(num))
#if 1
#ifndef Min
#define Min(a,b) ((a) <= (b) ? (a) : (b))
#endif
#ifndef Max
#define Max(a,b) ((a) >= (b) ? (a) : (b))
#endif
#endif
#ifndef Clamp
#define Clamp(v,a,b) (Max((a), Min((b), (v))))
#endif

template<typename T>
inline T Mix(T A, T B, float Interp)
{
    return A + (B - A) * Interp;
}

template<typename T>
T Inf()
{
    return std::numeric_limits<T>::infinity();
}

inline float HFOVtoVFOV(float Aspect, float HFOV_deg)
{
	const float rhf = HFOV_deg * DEG2RAD;
	return 2.f*atanf(tanf(rhf*.5f) / Aspect) * RAD2DEG;
}

// ----------------

template<typename T>
class vec2
{
public:
	vec2(T v = 0) : x(v), y(v) {}
	vec2(T ix, T iy) : x(ix), y(iy) {}
	vec2(const vec2<T> &v)
	{
		(*this) = v;
	}

	template<typename T2>
	vec2(const vec2<T2> &v)
	{
		x = (T) v.x;
		y = (T) v.y;
	}

	vec2<T> operator+(const vec2<T> &v) const
	{
		return vec2<T>(x + v.x, y + v.y);
	}

	vec2<T> operator-(const vec2<T> &v) const
	{
		return vec2<T>(x - v.x, y - v.y);
	}

	vec2<T> operator*(T v) const
	{
		return vec2<T>(x * v, y * v);
	}

	vec2<T> operator/(T v) const
	{
		return vec2<T>(x / v, y / v);
	}

	vec2<T>& operator+=(const vec2<T> &v)
	{
		x += v.x;
		y += v.y;
		return *this;
	}

	vec2<T>& operator-=(const vec2<T> &v)
	{
		x -= v.x;
		y -= v.y;
		return *this;
	}

	vec2<T> operator*(const vec2<T> &v) const
	{
		return vec2<T>(x * v.x, y * v.y);
	}

	vec2<T> operator/(const vec2<T> &v) const
	{
		return vec2<T>(x / v.x, y / v.y);
	}

	vec2<T>& operator*=(T v)
	{
		x *= v;
		y *= v;
		return *this;
	}

	vec2<T>& operator/=(T v)
	{
		x /= v;
		y /= v;
		return *this;
	}

	bool operator==(const vec2<T> &v) const
	{
		return fabs(float(x - v.x)) <= std::numeric_limits<T>::epsilon() &&
			fabs(float(y - v.y)) <= std::numeric_limits<T>::epsilon();
	}

	bool operator!=(const vec2<T> &v) const
	{
		return !(*this == v);
	}

	void operator=(const vec2<T> &v)
	{
		x = v.x;
		y = v.y;
	}

	vec2<T> operator-() const
	{
		return vec2<T>(-x, -y);
	}

	operator T*()
	{
		return &x;
	}

	T& operator[](int index)
	{
		return *((&x) + index);
	}

	const T& operator[](int index) const
	{
		return *((&x) + index);
	}

	T x;
	T y;
};

template<typename T>
float Dot(const vec2<T> &a, const vec2<T> &b)
{
	return a.x*b.x + a.y*b.y;
}

template<typename T>
inline vec2<T> Cross(const vec2<T> &a, const vec2<T> &b)
{
	vec2<T> r;
	r.x = a.y * b.x;
	r.y = a.x * b.y;
	return r;
}

template<typename T>
float Length(const vec2<T> &v)
{
	const float dot = Dot(v, v);
	return dot > 0.f ? sqrtf(dot) : 0.f;
}

template<typename T>
vec2<T> Normalize(const vec2<T> &v)
{
	const float len = Length(v);
	if (len > 0.f)
	{
		float k = 1.f / len;
		return v * k;
	}
	return vec2<T>(0);
}

typedef vec2<float> vec2f;
typedef vec2<int> vec2i;

template<typename T>
class vec3
{
public:
	inline vec3() {}
	inline vec3(T v) : x(v), y(v), z(v) {}
	inline vec3(T ix, T iy, T iz) : x(ix), y(iy), z(iz) {}
	inline vec3(const vec3<T> &v)
	{
		(*this) = v;
	}

	template<typename T2>
	inline vec3(const vec3<T2> &v)
	{
		x = (T) v.x;
		y = (T) v.y;
		z = (T) v.z;
	}

	inline vec3<T> operator+(const vec3<T> &v) const
	{
		return vec3<T>(x + v.x, y + v.y, z + v.z);
	}

	inline vec3<T> operator-(const vec3<T> &v) const
	{
		return vec3<T>(x - v.x, y - v.y, z - v.z);
	}

	inline vec3<T> operator*(T v) const
	{
		return vec3<T>(x * v, y * v, z * v);
	}

	inline vec3<T> operator/(T v) const
	{
		const T div = T(1) / v;
		return vec3<T>(x * div, y * div, z * div);
	}

	inline vec3<T>& operator+=(const vec3<T> &v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	inline vec3<T>& operator-=(const vec3<T> &v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	inline vec3<T>& operator*=(T v)
	{
		x *= v;
		y *= v;
		z *= v;
		return *this;
	}

	inline vec3<T>& operator*=(const vec3<T> &v)
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}

	inline vec3<T>& operator/=(T v)
	{
		const T div = T(1) / v;
		x *= div;
		y *= div;
		z *= div;
		return *this;
	}

	inline bool operator==(const vec3<T> &v) const
	{
		return fabsf(float(x - v.x)) <= std::numeric_limits<T>::epsilon() &&
			fabsf(float(y - v.y)) <= std::numeric_limits<T>::epsilon() &&
			fabsf(float(z - v.z)) <= std::numeric_limits<T>::epsilon();
	}

	inline bool operator!=(const vec3<T> &v) const
	{
		return !(*this == v);
	}

	inline void operator=(const vec3<T> &v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
	}

	inline vec3<T> operator-() const
	{
		return vec3<T>(-x, -y, -z);
	}

	inline operator T*()
	{
		return &x;
	}

	inline T& operator[](int index)
	{
		return *((&x) + index);
	}

	inline const T& operator[](int index) const
	{
		return *((&x) + index);
	}

	T x;
	T y;
	T z;
};

template<typename T>
inline float Dot(const vec3<T> &a, const vec3<T> &b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

template<typename T>
inline vec3<T> Cross(const vec3<T> &a, const vec3<T> &b)
{
	vec3<T> r;
	r.x = a.y * b.z - a.z * b.y;
	r.y = a.z * b.x - a.x * b.z;
	r.z = a.x * b.y - a.y * b.x;
	return r;
}

template<typename T>
inline float Length(const vec3<T> &v)
{
	const float dot = Dot(v, v);
	return dot > 0.f ? sqrtf(dot) : 0.f;
}

template<typename T>
inline vec3<T> Normalize(const vec3<T> &v)
{
	const float len = Length(v);
	if (len > 0.f)
	{
		float k = 1.f / len;
		return v * k;
	}
	return vec3<T>(0);
}

template<typename T>
class col3 : public vec3<T>
{
public:
	col3(T r, T g, T b) : vec3<T>(r, g, b) {}
	col3(vec3<T> v) : vec3<T>(v) {}
	const T &r() const { return this->x; }
	const T &g() const { return this->y; }
	const T &b() const { return this->z; }

	T &r() { return this->x; }
	T &g() { return this->y; }
	T &b() { return this->z; }
};

typedef vec3<float> vec3f;
typedef vec3<int> vec3i;
typedef col3<float> col3f;

inline float Luminance(const vec3f &rgb)
{
	return (0.2126f*rgb.x + 0.7152f*rgb.y + 0.0722f*rgb.z);
}

template<typename T>
class frame
{
    // NOTE - from [Frisvad2012]
    // TODO - This is for Z up, make it for Y up as well
    explicit frame(vec3<T> N)
    {
        vz = Normalize(N);
        if(vz.z < -0.9999999)
        {
            vx = vec3<T>(0, -1, 0);
            vy = vec3<T>(-1, 0, 0);
        }
        else
        {
            T a = T(1) / (T(1) + vz.z);
            T b = -vz.x * vz.y * a;
            vx = vec3<T>(T(1) - vz.x * vz.x * a, b, -vz.x);
            vy = vec3<T>(b, T(1) - vz.y * vz.y * a, -vz.y);
        }
    }

    vec3<T> vx;
    vec3<T> vy;
    vec3<T> vz;
};
/*

void vec3_lerp(vec3 r, vec3 a, vec3 b, float t) {
	for(int i = 0; i < 3; ++i)
		r[i] = a[i] * (1.f-t) + b[i] * t;
}
*/
template<typename T>
class vec4
{
public:
	vec4(T v = 0) : x(v), y(v), z(v), w(v) {}
	vec4(T ix, T iy, T iz, T iw) : x(ix), y(iy), z(iz), w(iw) {}
	vec4(const vec4<T> &v)
	{
		(*this) = v;
	}

	template<typename T2>
	vec4(const vec4<T2> &v)
	{
		x = (T) v.x;
		y = (T) v.y;
		z = (T) v.z;
		w = (T) v.w;
	}

	vec4(const vec3<T> &v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		w = (T) 1;
	}

	vec4<T> operator+(const vec4<T> &v) const
	{
		return vec4<T>(x + v.x, y + v.y, z + v.z, w + v.w);
	}

	vec4<T> operator-(const vec4<T> &v) const
	{
		return vec4<T>(x - v.x, y - v.y, z - v.z, w - v.w);
	}

	vec4<T> operator*(T v) const
	{
		return vec4<T>(x * v, y * v, z * v, w * v);
	}

	vec4<T> operator/(T v) const
	{
		return vec4<T>(x / v, y / v, z / v, w / v);
	}

	vec4<T>& operator+=(const vec4<T> &v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		w += v.w;
		return *this;
	}

	vec4<T>& operator-=(const vec4<T> &v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		w -= v.w;
		return *this;
	}

	vec4<T>& operator*=(T v)
	{
		x *= v;
		y *= v;
		z *= v;
		w *= v;
		return *this;
	}

	vec4<T>& operator/=(T v)
	{
		x /= v;
		y /= v;
		z /= v;
		w /= v;
		return *this;
	}

	bool operator==(const vec4<T> &v) const
	{
		return fabsf(float(x - v.x)) <= std::numeric_limits<T>::epsilon() &&
			fabsf(float(y - v.y)) <= std::numeric_limits<T>::epsilon() &&
			fabsf(float(z - v.z)) <= std::numeric_limits<T>::epsilon() &&
			fabsf(float(w - v.w)) <= std::numeric_limits<T>::epsilon();
	}

	bool operator!=(const vec4<T> &v) const
	{
		return !(*this == v);
	}

	void operator=(const vec4<T> &v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		w = v.w;
	}

	vec4<T> operator-() const
	{
		return vec4<T>(-x, -y, -z, -w);
	}

	operator T*()
	{
		return &x;
	}

	T& operator[](int index)
	{
		return *((&x) + index);
	}

	const T& operator[](int index) const
	{
		return *((&x) + index);
	}

#if 0
	std::string ToString() const
	{
	    std::stringstream ss;
	    ss << x << "," << y << "," << z << "," << w;

	    return ss.str();
	}
#endif

	T x;
	T y;
	T z;
	T w;
};

template<typename T>
inline float Dot(const vec3<T> &a, const vec4<T> &b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

template<typename T>
inline vec4<T> Cross(const vec4<T> &a, const vec4<T> &b)
{
	vec4<T> r;
	r.x = a.y * b.z - a.z * b.y;
	r.y = a.z * b.x - a.x * b.z;
	r.z = a.x * b.y - a.y * b.x;
	r.w = 1.f;
	return r;
}

template<typename T>
inline float Length(const vec4<T> &v)
{
	const float dot = Dot(v, v);
	return dot > 0.f ? sqrtf(dot) : 0.f;
}

template<typename T>
inline vec4<T> Normalize(const vec4<T> &v)
{
	const float len = Length(v);
	if (len > 0.f)
	{
		float k = 1.f / len;
		return v * k;
	}
	return vec4<T>(0);
}

template<typename T>
class col4 : public vec4<T>
{
public:
    col4() : col4(0,0,0,0) {}
	col4(T r, T g, T b, T a) : vec4<T>(r, g, b, a) {}
	col4(vec4<T> v) : vec4<T>(v) {}
	const T &r() const { return this->x; }
	const T &g() const { return this->y; }
	const T &b() const { return this->z; }
	const T &a() const { return this->w; }

	T &r() { return this->x; }
	T &g() { return this->y; }
	T &b() { return this->z; }
	T &a() { return this->w; }

};

typedef vec4<float> vec4f;
typedef vec4<int> vec4i;
typedef col4<float> col4f;


template<typename T>
inline T BilinearLookup(const float *floatTexture, const vec2f &coord, const vec2i &texSize)
{
	const vec2i coordI((int) floorf(texSize.y * coord.x), (int) floorf(texSize.y * coord.y));
	const vec2f offset(coord.x - coordI.x, coord.y - coordI.y);

	const T *texture = (T*) floatTexture;

	T res = texture[coordI.y * texSize.x + coordI.x];
	if (coordI.x < (texSize.x - 1) && coordI.y < (texSize.y - 1))
	{
		T res_tr = texture[coordI.y * texSize.x + coordI.x + 1];
		T res_bl = texture[(coordI.y + 1) * texSize.x + coordI.x];
		T res_br = texture[(coordI.y + 1) * texSize.x + coordI.x + 1];

		res = res * (1.f - offset.x) + res_tr * offset.x;
		res_bl = res_bl * (1.f - offset.x) + res_br * offset.x;
		res = res * (1.f - offset.y) + res_bl * offset.y;
	}
	return res;
}

template<typename T>
class mat3
{
public:
	mat3()
	{
		Identity();
	}
	mat3(vec3<T> ix, vec3<T> iy, vec3<T> iz)
	{
		M[0] = ix;
		M[1] = iy;
		M[2] = iz;
	}

	mat3(T a11, T a21, T a31,
		T a12, T a22, T a32,
		T a13, T a23, T a33)
	{
		M[0] = vec4<T>(a11, a12, a13);
		M[1] = vec4<T>(a21, a22, a23);
		M[2] = vec4<T>(a31, a32, a33);
	}


	mat3(const mat3<T> &v)
	{
		(*this) = v;
	}

	void Identity()
	{
		M[0] = vec3<T>(1, 0, 0);
		M[1] = vec3<T>(0, 1, 0);
		M[2] = vec3<T>(0, 0, 1);
	}

	vec3<T>& operator[](int index)
	{
		return M[index];
	}

	mat3<T> operator+(const mat3<T> &v) const
	{
		return mat3<T>(M[0] + v.M[0], M[1] + v.M[1], M[2] + v.M[2]);
	}

	mat3<T> operator-(const mat3<T> &v) const
	{
		return mat3<T>(M[0] - v.M[0], M[1] - v.M[1], M[2] - v.M[2]);
	}

	mat3<T> operator*(T v) const
	{
		return mat3<T>(M[0] * v, M[1] * v, M[2] * v);
	}

	mat3<T> operator/(T v) const
	{
		return mat3<T>(M[0] / v, M[1] / v, M[2] / v);
	}

	mat3<T>& operator+=(const mat3<T> &v)
	{
		M[0] += v.M[0];
		M[1] += v.M[1];
		M[2] += v.M[2];
		return *this;
	}

	mat3<T>& operator-=(const mat3<T> &v)
	{
		M[0] -= v.M[0];
		M[1] -= v.M[1];
		M[2] -= v.M[2];
		return *this;
	}

	mat3<T>& operator*=(T v)
	{
		M[0] *= v.M[0];
		M[1] *= v.M[1];
		M[2] *= v.M[2];
		return *this;
	}

	mat3<T>& operator/=(T v)
	{
		M[0] /= v.M[0];
		M[1] /= v.M[1];
		M[2] /= v.M[2];
		return *this;
	}

	bool operator==(const mat3<T> &v) const
	{
		return M[0] == v.M[0] && M[1] == v.M[1] && M[2] == v.M[2];
	}

	bool operator!=(const mat3<T> &v) const
	{
		return !(*this == v);
	}

	void operator=(const mat3<T> &v)
	{
		M[0] = v.M[0];
		M[1] = v.M[1];
		M[2] = v.M[2];
	}

	mat3<T>& operator*=(const mat3<T> &v)
	{
		(*this) = (*this) * v;
		return (*this);
	}

	operator T*()
	{
		return &M[0].x;
	}

	mat3<T> operator*(const mat3<T> &v) const
	{
		mat3<T> r;
		for (int row = 0; row < 3; ++row)
			for (int col = 0; col < 3; ++col)
			{
				r[col][row] = 0;
				for (int k = 0; k < 3; ++k)
					r[col][row] += M[k][row] * v.M[col][k];
			}

		return r;
	}

	vec3<T> operator*(const vec3<T> &v) const
	{
		vec3<T> r;
		for (int j = 0; j < 3; ++j)
		{
			r[j] = 0;
			for (int i = 0; i < 3; ++i)
			{
				r[j] += M[i][j] * v[i];
			}
		}
		return r;
	}

	mat3<T> Transpose()
	{
		int i, j;
		mat3 R;
		for (j = 0; j < 3; ++j)
		{
			for (i = 0; i < 3; ++i)
			{
				R[i][j] = M[j][i];
			}
		}
		return R;
	}

	mat3<T> Inverse()
	{
		T oneOverDet = static_cast<T>(1) / (
			+M[0][0] * (M[1][1] * M[2][2] - M[2][1] * M[1][2])
			- M[1][0] * (M[0][1] * M[2][2] - M[2][1] * M[0][2])
			+ M[2][0] * (M[0][1] * M[1][2] - M[1][1] * M[0][2]));

		mat3<T> R;
		R[0][0] = +(M[1][1] * M[2][2] - M[2][1] * M[1][2]) * oneOverDet;
		R[1][0] = -(M[1][0] * M[2][2] - M[2][0] * M[1][2]) * oneOverDet;
		R[2][0] = +(M[1][0] * M[2][1] - M[2][0] * M[1][1]) * oneOverDet;
		R[0][1] = -(M[0][1] * M[2][2] - M[2][1] * M[0][2]) * oneOverDet;
		R[1][1] = +(M[0][0] * M[2][2] - M[2][0] * M[0][2]) * oneOverDet;
		R[2][1] = -(M[0][0] * M[2][1] - M[2][0] * M[0][1]) * oneOverDet;
		R[0][2] = +(M[0][1] * M[1][2] - M[1][1] * M[0][2]) * oneOverDet;
		R[1][2] = -(M[0][0] * M[1][2] - M[1][0] * M[0][2]) * oneOverDet;
		R[2][2] = +(M[0][0] * M[1][1] - M[1][0] * M[0][1]) * oneOverDet;
	}

	vec3<T> M[3];
};

typedef mat3<float> mat3f;

template<typename T>
class mat4
{
public:
	mat4()
	{
		Identity();
	}
	mat4(vec4<T> ix, vec4<T> iy, vec4<T> iz, vec4<T> iw)
	{
		M[0] = ix;
		M[1] = iy;
		M[2] = iz;
		M[3] = iw;
	}

	mat4(T a11, T a21, T a31, T a41,
		T a12, T a22, T a32, T a42,
		T a13, T a23, T a33, T a43,
		T a14, T a24, T a34, T a44)
	{
		M[0] = vec4<T>(a11, a12, a13, a14);
		M[1] = vec4<T>(a21, a22, a23, a24);
		M[2] = vec4<T>(a31, a32, a33, a34);
		M[3] = vec4<T>(a41, a42, a43, a44);
	}


	mat4(const mat4<T> &v)
	{
		(*this) = v;
	}

	void Identity()
	{
		M[0] = vec4<T>(1, 0, 0, 0);
		M[1] = vec4<T>(0, 1, 0, 0);
		M[2] = vec4<T>(0, 0, 1, 0);
		M[3] = vec4<T>(0, 0, 0, 1);
	}

	vec4<T>& operator[](int index)
	{
		return M[index];
	}

	mat4<T> operator+(const mat4<T> &v) const
	{
		return mat4<T>(M[0] + v.M[0], M[1] + v.M[1], M[2] + v.M[2], M[3] + v.M[3]);
	}

	mat4<T> operator-(const mat4<T> &v) const
	{
		return mat4<T>(M[0] - v.M[0], M[1] - v.M[1], M[2] - v.M[2], M[3] - v.M[3]);
	}

	mat4<T> operator*(T v) const
	{
		return mat4<T>(M[0] * v, M[1] * v, M[2] * v, M[3] * v);
	}

	mat4<T> operator/(T v) const
	{
		return mat4<T>(M[0] / v, M[1] / v, M[2] / v, M[3] / v);
	}

	mat4<T>& operator+=(const mat4<T> &v)
	{
		M[0] += v.M[0];
		M[1] += v.M[1];
		M[2] += v.M[2];
		M[3] += v.M[3];
		return *this;
	}

	mat4<T>& operator-=(const mat4<T> &v)
	{
		M[0] -= v.M[0];
		M[1] -= v.M[1];
		M[2] -= v.M[2];
		M[3] -= v.M[3];
		return *this;
	}

	mat4<T>& operator*=(T v)
	{
		M[0] *= v.M[0];
		M[1] *= v.M[1];
		M[2] *= v.M[2];
		M[3] *= v.M[3];
		return *this;
	}

	mat4<T>& operator/=(T v)
	{
		M[0] /= v.M[0];
		M[1] /= v.M[1];
		M[2] /= v.M[2];
		M[3] /= v.M[3];
		return *this;
	}

	bool operator==(const mat4<T> &v) const
	{
		return M[0] == v.M[0] && M[1] == v.M[1] && M[2] == v.M[2] && M[3] == v.M[3];
	}

	bool operator!=(const mat4<T> &v) const
	{
		return !(*this == v);
	}

	void operator=(const mat4<T> &v)
	{
		M[0] = v.M[0];
		M[1] = v.M[1];
		M[2] = v.M[2];
		M[3] = v.M[3];
	}

	mat4<T>& operator*=(const mat4<T> &v)
	{
		(*this) = (*this) * v;
		return (*this);
	}

	operator T*()
	{
		return &M[0].x;
	}

	mat4<T> operator*(const mat4<T> &v) const
	{
		mat4<T> r;
		for (int row = 0; row < 4; ++row)
			for (int col = 0; col < 4; ++col)
			{
				r[col][row] = 0;
				for (int k = 0; k < 4; ++k)
					r[col][row] += M[k][row] * v.M[col][k];
			}

		return r;
	}

	vec3<T> operator*(const vec3<T> &v) const
	{
		vec3<T> r;
		for (int j = 0; j < 3; ++j)
		{
			r[j] = 0;
			for (int i = 0; i < 3; ++i)
			{
				r[j] += M[i][j] * v[i];
			}
		}
		return r;
	}

	vec4<T> operator*(const vec4<T> &v) const
	{
		vec4<T> r;
		for (int j = 0; j < 4; ++j)
		{
			r[j] = 0;
			for (int i = 0; i < 4; ++i)
			{
				r[j] += M[i][j] * v[i];
			}
		}
		return r;
	}

    void SetTranslation(vec3<T> xyz)
    {
		M[3] = vec4<T>(xyz.x, xyz.y, xyz.z, 1.f);
    }

	static mat4<T> Translation(vec3<T> xyz)
	{
		mat4<T> r;
		r[3] = vec4<T>(xyz.x, xyz.y, xyz.z, 1.f);
		return r;
	}

    void SetScale(vec3<T> xyz)
    {
		M[0].x = xyz.x;
		M[1].y = xyz.y;
		M[2].z = xyz.z;
    }

	static mat4<T> Scale(vec3<T> xyz)
	{
		mat4<T> s;
		s[0].x = xyz.x;
		s[1].y = xyz.y;
		s[2].z = xyz.z;
		return s;
	}

	void FromVec3Mult(const vec3<T> &a, const vec3<T> &b)
	{
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				M[i][j] = i < 3 && j < 3 ? a[i] * b[j] : 0.f;
	}

	void FromTRS(const vec3<T> &pos, const vec3<T> &rot, const vec3<T> scale)
	{
		*this = mat4<T>::Scale(scale);
		*this = this->RotateX(rot.x);
		*this = this->RotateY(rot.y);
		*this = this->RotateZ(rot.z);
		M[3] = vec4<T>(pos.x, pos.y, pos.z, 1.f);
	}

    void FromAxisAngle(const vec3<T> &Rot)
    {
        this->Identity();
		*this = this->RotateX(Rot.x);
		*this = this->RotateY(Rot.y);
		*this = this->RotateZ(Rot.z);
    }

	mat4<T> Rotate(const vec3<T> &axis, float angle)
	{
		float s = sinf(angle);
		float c = cosf(angle);
		axis.Normalize();

		mat4<T> r;
		r.FromVec3Mult(axis, axis);
		mat4<T> S(0, -axis[2], axis[1], 0,
			axis[2], 0, -axis[0], 0,
			-axis[1], axis[0], 0, 0,
			0, 0, 0, 0);
		S *= s;

		mat4<T> C;
		C -= r;
		C *= c;
		r += C;
		r += S;
		r[3][3] = 1;
		return M * r;
	}

	mat4<T> RotateX(float angle)
	{
		float s = sinf(angle);
		float c = cosf(angle);
        mat4<T> R(
                1, 0, 0, 0,
                0, c, -s, 0,
                0, s, c, 0,
                0, 0, 0, 1
                );
		return R * (*this);
	}

	mat4<T> RotateY(float angle)
	{
		float s = sinf(angle);
		float c = cosf(angle);
        mat4<T> R(
                c, 0, s, 0,
                0, 1, 0, 0,
                -s, 0, c, 0,
                0, 0, 0, 1
                );
		return R * (*this);
	}

	mat4<T> RotateZ(float angle)
	{
		float s = sinf(angle);
		float c = cosf(angle);
        mat4<T> R(
                c, -s, 0, 0,
                s, c, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
                );
		return R * (*this);
	}

	mat4<T> Transpose()
	{
		int i, j;
		mat4 R;
		for (j = 0; j < 4; ++j)
		{
			for (i = 0; i < 4; ++i)
			{
				R[i][j] = M[j][i];
			}
		}
		return R;
	}

	mat4<T> Inverse()
	{
		mat4<T> R;
		R[0][0] = M[1][1] * (M[2][2] * M[3][3] - M[2][3] * M[3][2]) - M[2][1] * (M[1][2] * M[3][3] - M[1][3] * M[3][2]) - M[3][1] * (M[1][3] * M[2][2] - M[1][2] * M[2][3]);
		R[0][1] = M[0][1] * (M[2][3] * M[3][2] - M[2][2] * M[3][3]) - M[2][1] * (M[0][3] * M[3][2] - M[0][2] * M[3][3]) - M[3][1] * (M[0][2] * M[2][3] - M[0][3] * M[2][2]);
		R[0][2] = M[0][1] * (M[1][2] * M[3][3] - M[1][3] * M[3][2]) - M[1][1] * (M[0][2] * M[3][3] - M[0][3] * M[3][2]) - M[3][1] * (M[0][3] * M[1][2] - M[0][2] * M[1][3]);
		R[0][3] = M[0][1] * (M[1][3] * M[2][2] - M[1][2] * M[2][3]) - M[1][1] * (M[0][3] * M[2][2] - M[0][2] * M[2][3]) - M[2][1] * (M[0][2] * M[1][3] - M[0][3] * M[1][2]);

		R[1][0] = M[1][0] * (M[2][3] * M[3][2] - M[2][2] * M[3][3]) - M[2][0] * (M[1][3] * M[3][2] - M[1][2] * M[3][3]) - M[3][0] * (M[1][2] * M[2][3] - M[1][3] * M[2][2]);
		R[1][1] = M[0][0] * (M[2][2] * M[3][3] - M[2][3] * M[3][2]) - M[2][0] * (M[0][2] * M[3][3] - M[0][3] * M[3][2]) - M[3][0] * (M[0][3] * M[2][2] - M[0][2] * M[2][3]);
		R[1][2] = M[0][0] * (M[1][3] * M[3][2] - M[1][2] * M[3][3]) - M[1][0] * (M[0][3] * M[3][2] - M[0][2] * M[3][3]) - M[3][0] * (M[0][2] * M[1][3] - M[0][3] * M[1][2]);
		R[1][3] = M[0][0] * (M[1][2] * M[2][3] - M[1][3] * M[2][2]) - M[1][0] * (M[0][2] * M[2][3] - M[0][3] * M[2][2]) - M[2][0] * (M[0][3] * M[1][2] - M[0][2] * M[1][3]);

		R[2][0] = M[1][0] * (M[2][1] * M[3][3] - M[2][3] * M[3][1]) - M[2][0] * (M[1][1] * M[3][3] - M[1][3] * M[3][1]) - M[3][0] * (M[1][3] * M[2][1] - M[1][1] * M[2][3]);
		R[2][1] = M[0][0] * (M[2][3] * M[3][1] - M[2][1] * M[3][3]) - M[2][0] * (M[0][3] * M[3][1] - M[0][1] * M[3][3]) - M[3][0] * (M[0][1] * M[2][3] - M[0][3] * M[2][1]);
		R[2][2] = M[0][0] * (M[1][1] * M[3][3] - M[1][3] * M[3][1]) - M[1][0] * (M[0][1] * M[3][3] - M[0][3] * M[3][1]) - M[3][0] * (M[0][3] * M[1][1] - M[0][1] * M[1][3]);
		R[2][3] = M[0][0] * (M[1][3] * M[2][1] - M[1][1] * M[2][3]) - M[1][0] * (M[0][3] * M[2][1] - M[0][1] * M[2][3]) - M[2][0] * (M[0][1] * M[1][3] - M[0][3] * M[1][1]);

		R[3][0] = M[1][0] * (M[2][2] * M[3][1] - M[2][1] * M[3][2]) - M[2][0] * (M[1][2] * M[3][1] - M[1][1] * M[3][2]) - M[3][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1]);
		R[3][1] = M[0][0] * (M[2][1] * M[3][2] - M[2][2] * M[3][1]) - M[2][0] * (M[0][1] * M[3][2] - M[0][2] * M[3][1]) - M[3][0] * (M[0][2] * M[2][1] - M[0][1] * M[2][2]);
		R[3][2] = M[0][0] * (M[1][2] * M[3][1] - M[1][1] * M[3][2]) - M[1][0] * (M[0][2] * M[3][1] - M[0][1] * M[3][2]) - M[3][0] * (M[0][1] * M[1][2] - M[0][2] * M[1][1]);
		R[3][3] = M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1]) - M[1][0] * (M[0][1] * M[2][2] - M[0][2] * M[2][1]) - M[2][0] * (M[0][2] * M[1][1] - M[0][1] * M[1][2]);

		return R;
	}

	static mat4<T> Frustum(float l, float r, float b, float t, float n, float f)
	{
		mat4<T> R;
		R[0][0] = 2.f*n / (r - l);
		R[0][1] = R[0][2] = R[0][3] = 0.f;

		R[1][1] = 2.f*n / (t - b);
		R[1][0] = R[1][2] = R[1][3] = 0.f;

		R[2][0] = (r + l) / (r - l);
		R[2][1] = (t + b) / (t - b);
		R[2][2] = -(f + n) / (f - n);
		R[2][3] = -1.f;

		R[3][2] = -2.f*(f*n) / (f - n);
		R[3][0] = R[3][1] = R[3][3] = 0.f;
		return R;
	}

	static mat4<T> Perspective(float fov_x, float aspect, float n, float f)
	{
		float fov_rad = HFOVtoVFOV(aspect, fov_x) * DEG2RAD;
		float tanHalfFovy = tanf(fov_rad * 0.5f);

		float sx = 1.f / (aspect * tanHalfFovy);
		float sy = 1.f / tanHalfFovy;
		float sz = -(f + n) / (f - n);
		float pz = -(2.f*f*n) / (f - n);

		mat4<T> R;
		R[0][0] = sx;
		R[1][1] = sy;
		R[2][2] = sz;
		R[3][2] = pz;
		R[2][3] = -1.f;
		R[3][3] = 0.f;

		return R;
	}

	static mat4<T> Ortho(float l, float r, float b, float t, float n, float f)
	{
		mat4<T> R;

		R[0][0] = 2.f / (r - l);
		R[0][1] = R[0][2] = R[0][3] = 0.f;

		R[1][1] = 2.f / (t - b);
		R[1][0] = R[1][2] = R[1][3] = 0.f;

		R[2][2] = -1.f / (f - n);
		R[2][0] = R[2][1] = R[2][3] = 0.f;

		R[3][0] = -(r + l) / (r - l);
		R[3][1] = -(t + b) / (t - b);
		R[3][2] = -(n) / (f - n);
		R[3][3] = 1.f;

		return R;
	}

	static mat4<T> LookAt(const vec3<T> &cam_pos, const vec3<T> &target, const vec3<T> &up)
	{
		// forward
		vec3<T> f = Normalize(target - cam_pos);

		// right
		vec3<T> r = Normalize(Cross(f, up)); // s

		// up
		vec3<T> u = Normalize(Cross(r, f)); // u

		// translation
		mat4<T> Tr = mat4<T>::Translation(-cam_pos);

		// rotation
		mat4<T> R;
		R[0][0] = r[0];    R[1][0] = r[1];    R[2][0] = r[2];
		R[0][1] = u[0];    R[1][1] = u[1];    R[2][1] = u[2];
		R[0][2] = -f[0];   R[1][2] = -f[1];   R[2][2] = -f[2];

		R *= Tr;
		return R;
	}

#if 0
	std::string ToString() const
	{
	    std::stringstream ss;
	    ss << M[0][0] << "," << M[0][1] << "," << M[0][2] << "," << M[0][3] << std::endl
	       << M[1][0] << "," << M[1][1] << "," << M[1][2] << "," << M[1][3] << std::endl
	       << M[2][0] << "," << M[2][1] << "," << M[2][2] << "," << M[2][3] << std::endl
	       << M[3][0] << "," << M[3][1] << "," << M[3][2] << "," << M[3][3];

	    return ss.str();
	}
#endif

	vec4<T> M[4];
};

typedef mat4<float> mat4f;
typedef mat4<int> mat4i;

class complex {
public:
    complex() : r(0.f), i(0.f) {}
    complex(float a, float b) : r(a), i(b) {}

    complex operator*(const complex &c) const
    {
        return complex(this->r*c.r - this->i*c.i, this->r*c.i + this->i*c.r);
    }
    complex operator+(const complex& c) const
    {
        return complex(this->r + c.r, this->i + c.i);
    }
    complex operator-(const complex& c) const
    {
        return complex(this->r - c.r, this->i - c.i);
    }
    complex operator*(const float c) const
    {
        return complex(this->r*c, this->i*c);
    }
    complex& operator=(const complex& c)
    {
        this->r = c.r;
        this->i = c.i;
        return *this;
    }

    float r, i;
};

inline complex Conjugate(const complex &c)
{
    return complex(c.r, -c.i);
}

/*
void mat4_row(vec4 r, mat4 M, int i) {
	int k;
	for(k=0; k<4; ++k)
		r[k] = M[k][i];
}
void mat4_col(vec4 r, mat4 M, int i) {
	int k;
	for(k=0; k<4; ++k)
		r[k] = M[i][k];
}
// print in column-order, in given string
void mat4_print(str256 dst, mat4 const M) {
	sprintf(dst, "%g %g %g %g\n"
				 "%g %g %g %g\n"
				 "%g %g %g %g\n"
				 "%g %g %g %g"  ,
				 M[0][0], M[1][0], M[2][0], M[3][0],
				 M[0][1], M[1][1], M[2][1], M[3][1],
				 M[0][2], M[1][2], M[2][2], M[3][2],
				 M[0][3], M[1][3], M[2][3], M[3][3]);
}

void quaternion_identity(quaternion q) {
	q[0] = q[1] = q[2] = 0.;
	q[3] = 1.;
}
void quaternion_cpy(quaternion d, quaternion s) {
	for(int i = 0; i < 4; ++i)
		d[i] = s[i];
}
void quaternion_add(quaternion r, quaternion a, quaternion b) {
	for(int i=0; i<4; ++i)
		r[i] = a[i] + b[i];
}
void quaternion_sub(quaternion r, quaternion a, quaternion b) {
	for(int i=0; i<4; ++i)
		r[i] = a[i] - b[i];
}
void quaternion_mul(quaternion r, quaternion p, quaternion q) {
	vec3 w;
	vec3_cross(r, p, q);
	vec3_scale(w, p, q[3]);
	vec3_addv(r, r, w);
	vec3_scale(w, q, p[3]);
	vec3_addv(r, r, w);
	r[3] = p[3]*q[3] - vec3_dot(p, q);
}
void quaternion_scale(quaternion r, quaternion v, float s) {
	for(int i=0; i<4; ++i)
		r[i] = v[i] * s;
}
float quaternion_dot(quaternion const a, quaternion const b) {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
}
void quaternion_conj(quaternion r, quaternion q) {
	for(int i=0; i<3; ++i)
		r[i] = -q[i];
	r[3] = q[3];
}
#define quaternion_norm vec4_norm
void quaternion_mul_vec3(vec3 r, quaternion q, vec3 v) {
	quaternion q_;
	quaternion v_ = {v[0], v[1], v[2], 0.};

	quaternion_conj(q_, q);
	quaternion_norm(q_, q_);
	quaternion_mul(q_, v_, q_);
	quaternion_mul(q_, q, q_);
	memcpy(r, q_, sizeof(vec3));
}
void mat4_from_quaternion(mat4 M, quaternion q) {
	float a = q[3];
	float b = q[0];
	float c = q[1];
	float d = q[2];
	float a2 = a*a;
	float b2 = b*b;
	float c2 = c*c;
	float d2 = d*d;

	M[0][0] = a2 + b2 - c2 - d2;
	M[0][1] = 2*(b*c + a*d);
	M[0][2] = 2*(b*d - a*c);
	M[0][3] = 0.;

	M[1][0] = 2*(b*c - a*d);
	M[1][1] = a2 - b2 + c2 - d2;
	M[1][2] = 2*(c*d + a*b);
	M[1][3] = 0.;

	M[2][0] = 2*(b*d + a*c);
	M[2][1] = 2*(c*d - a*b);
	M[2][2] = a2 - b2 - c2 + d2;
	M[2][3] = 0.;

	M[3][0] = M[3][1] = M[3][2] = 0.;
	M[3][3] = 1.;
}
void mat4_mul_quaternion(mat4 R, mat4 M, quaternion q) {
	quaternion_mul_vec3(R[0], M[0], q);
	quaternion_mul_vec3(R[1], M[1], q);
	quaternion_mul_vec3(R[2], M[2], q);

	R[3][0] = R[3][1] = R[3][2] = 0.;
	R[3][3] = 1.;
}
void quaternion_from_mat4(quaternion q, mat4 M) {
	float r=0.;
	int i;

	int perm[] = { 0, 1, 2, 0, 1 };
	int *p = perm;

	for(i = 0; i<3; i++) {
		float m = M[i][i];
		if(m < r)
			continue;
		m = r;
		p = &perm[i];
	}

	r = sqrt(1.f + M[p[0]][p[0]] - M[p[1]][p[1]] - M[p[2]][p[2]]);

	if(r < 1e-6) {
		q[0] = 1.;
		q[1] = q[2] = q[3] = 0.;
		return;
	}

	q[0] = r/2.f;
	q[1] = (M[p[0]][p[1]] - M[p[1]][p[0]])/(2.f*r);
	q[2] = (M[p[2]][p[0]] - M[p[0]][p[2]])/(2.f*r);
	q[3] = (M[p[2]][p[1]] - M[p[1]][p[2]])/(2.f*r);
}
void quaternion_lerp(quaternion r, quaternion a, quaternion b, float t) {
	quaternion qa;

	// avoid long-way around
	if(quaternion_dot(a,b) < 0.f)
		quaternion_scale(qa, a, -1.f);
	else
		quaternion_cpy(qa, a);

	for(int i = 0; i < 4; ++i)
		r[i] = qa[i] * (1.f-t) + b[i] * t;
}
void quaternion_nlerp(quaternion r, quaternion a, quaternion b, float t) {
	quaternion_lerp(r, a, b, t);
	quaternion_norm(r, r);
}
void quaternion_slerp(quaternion r, quaternion a, quaternion b, float t) {
	quaternion q0, q1;
	quaternion_cpy(q0, a);
	quaternion_cpy(q1, b);

	float cos_half_theta = quaternion_dot(q0, q1);
	if(cos_half_theta < 0.f) {
		for(int i = 0; i < 4; ++i)
			q0[i] *= -1.f;
		cos_half_theta = quaternion_dot(q0, q1);
	}

	if(fabs(cos_half_theta) >= 1.f)
		quaternion_cpy(r, q0);

	float sin_half_theta = sqrt(1.f - cos_half_theta*cos_half_theta);
	// if theta == 180deg, not fully defined result. return lerp
	if(fabs(sin_half_theta) < 1e-3f) {
		for(int i = 0; i < 4;++i)
			r[i] = (1.f - t) * q0[i] + t * q1[i];
	}

	// return slerp
	float half_theta = acosf(cos_half_theta);
	float t0 = sinf((1.f-t)*half_theta) / sin_half_theta;
	float t1 = sinf(t*half_theta) / sin_half_theta;
	for(int i = 0; i < 4; ++i)
		r[i] = q0[i] * t0 + q1[i] * t1;
}

/// Returns 1 if src can fit in dst, 0 otherwise.
int rectangle_fit_in(const rectangle dst, const rectangle src) {
	if(src.x >= dst.x && (src.x+src.w) <= (dst.x+dst.w))
		if(src.y >= dst.y && (src.y+src.h) <= (dst.y+dst.h))
			return 1;
	return 0;
}

/// Returns 1 if src can fit in dst, 0 otherwise. Only check sizes
int rectangle_smaller(const rectangle dst, const rectangle src) {
	return src.w <= dst.w && src.h <= dst.h;
}

/// Returns 1 if rectangle sizes are equal
int rectangle_equal_size(const rectangle a, const rectangle b) {
	return a.w == b.w && a.h == b.h;
}

#undef INLINE
*/
struct quaternion
{
    float x, y, z, w;
};

vec3f QuaternionToEulerAngle(quaternion const &Q)
{
    vec3f R;
    float sinRoll = 2.f * (Q.w * Q.x + Q.y * Q.z);
    float cosRoll = 1.f - 2.f * (Q.x * Q.x + Q.y * Q.y);
    R.y = std::atan2(sinRoll, cosRoll);

    float sinPitch = 2.f * (Q.w * Q.y - Q.z * Q.x);
    if(std::abs(sinPitch) >= 1.f)
        R.x = std::copysign(M_PI_OVER_TWO, sinPitch);
    else
        R.x = std::asin(sinPitch);

    float sinYaw = 2.f * (Q.w * Q.z - Q.x * Q.y);
    float cosYaw = 1.f - 2.f * (Q.y * Q.y + Q.z * Q.z);
    R.z = std::atan2(sinYaw, cosYaw);
    return R;
}

// {theta, phi} -> [x, y, z]
inline vec3f SphericalToCartesian(float Theta, float Phi)
{
    float SinTheta = sinf(Theta);
    return vec3f(SinTheta * cosf(Phi), cosf(Theta), SinTheta * sinf(Phi));
}

// [x, y, z] -> {theta, phi}
inline vec2f CartesianToSpherical(vec3f const &V)
{
    return vec2f(atan2f(sqrtf(Square(V.x) + Square(V.z)), V.y), atan2f(V.z, V.x));
}
#endif
