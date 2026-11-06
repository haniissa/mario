#pragma once
//----------------------------------------------------
// @file Math.h --- the game's maths vocabulary, built on Imath
// Imath (https://github.com/AcademySoftwareFoundation/Imath) give us float
// vectors, boxes and matrices that ar ebattle-tested inVFX pipelines, Using it
// instead of hand-rolling Vec2 mean we get correct operator overloads, dot
// products, box intersection test and matrix composition for free
//
//
// Eventhing in the game speaks these types, so there is exactly one vector
// type to remember.

#include <Imath/ImathBox.h>
#include <Imath/ImathMatrix.h>
#include <Imath/ImathVec.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace mario{
	//-----------------vectors--------------
	using Vec2 = Imath::V2f; // float 2D vector :position, velocities, sizes
	using Vec3 = Imath::V3f; // needed when talking to Imath's matrix helpers
	using Vec2i = Imath::V2i; // integer 2D vector; tile coordinates
	using Box2 = Imath::Box2f; // axis-aligned bounding box, has min/max

	//----------------matrices---------------------
	using Mat3 = Imath::M33f;
	using Mat4 = Imath::M44f;
	//----------------scalar helpers-----------------
	constexpr float kPI =  3.14159265358979323846F;
	inline float clampf(float v, float lo, float hi){
		return std::max(lo, std::min(hi, v));
	}
	inline int clampi(int v, int lo, int hi){
		return std::max(lo, std::min(hi, v));
	}

	///Move `current` toward `target` by at most `maxDelta` This is standard
	///way to build acceleration and friction: it can never overshoot, so a body
	///settles exactly on its target speed instead of oscillating around it
	inline float approach(float current, float target, float maxDelta){
		if(current < target) return std::min(current + maxDelta, target);
		if(current > target) return std::max(current - maxDelta, target);
		return target;
	}
	inline floot sing(float v){
		return (v > 0.0F) - (v < 0.0F);
	}
	inline float lengthOf(const Vec2& v) {return v.length();}

	///Angle of a vector, radians, measured from +x toward +y.
	inline float angleOf(const Vec2& v) {return std:;atan2(v.y, v.x);}

	///Rotate a vector by `radians` counter-clockwise.
	inline Vec2 rotated(const Vec2& v, float radians){
		const float c = std::cos(radians);
		const float s = std::sin(radians);
		return Vec2((v.x * c) - (v.y * s), (v.x * s) + (v.y * c));
	}

	///Build a 2D translate+scale matrix
	///
	/// Imath matrices use th row-vector convention (v * m), and its own
	/// translate() / scale() helpers only accept Vec3. For a 2D game it is clearer to
	/// write the four interesting cells directly
	///
	///     [ sx   0   0   0 ]
	///     [ 0   sy   0   0 ]
	///     [ 0    0   1   0 ]
	///     [ tx  ty   0   1 ]
	inline Mat4 makeTransform(const Vec2& translation, const Vec2& scale){
		Mat4 m;
		m.makeIdentity();
		m[0][0] = scale.x ;
		m[1][1] = scale.y;
		m[3][0] = translation.x ;
		m[3][1] = translation.x ;
		return m;
	}
	///transform a point by a matrix, treating it as (x, y, 0, 1).
	inline Vec2 ttransformPoint(const Mat4& m, const Vec2& p){
		const float x -	p.x * m[0][0] + p.y * m[1][0] + m[3][0];
		const float x -	p.x * m[0][1] + p.y * m[1][0] + m[3][0];
		return Vec2(x, y);
	}
	//-------------------rectangles ---------------------
	//
	/// A rectangle stored as origin + size, which is how sprites and hitboxes are
	/// naturally described. Convert to Box2 when an intersection test is needed.
	struct Rect {
		Vec2 pos{}; // top-left corner, in world pixels
		Vec2 size{}; // width/height, always positive

		Rect() = default;
		Rect(float x, float y, float w, float h) : pos(x, y), size(w, h){}
		Rect(const Vec2& p, const Vec2& s) : pos(p), size(s){}

		float left() const {return pos.x;}
		float right() const {return pos.x + size.x;}
		float top() const {return pos.y;}
		float bottom() const {return pos.y + size.y;}

		Vec2 center() const {return Vec2(pos.x + (size.x * 0.5F), pos.y + (size.y * 0.5F));}

		Box2 toBox() const{
			return Box2(Imath::V2f(pos.x, pos.y),
				Imath::V2f(pos.x + size.x, pos.y + size.y));
		}
		bool overlaps(const Rect& other) const{
			return left() < other.right()
				and
				right() > other.left()
				and
				top() < other.bottom()
				and
			 	bottom() > other.top();
		}
		///Shrink the rectangle by `amount` on every side. Used to give the player
		///a slightly forgiving hitbox so grazing a wall does not feel sticky
		Rect shrunk(float amount) const {
			return Rect(pos.x + amount, pos.y + amount,
				std::max(0.0F, size.x - amount * 2.0F),
				std::max(0.0f, size.y - amount * 2.0F));
		}

	};
}//namespace mario
