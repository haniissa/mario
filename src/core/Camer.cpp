#include "core/Camera.h"

namespace mario{
	void Camera::setViewport(float width, float height){
		widht_ = width;
		height_ = height;
	}

	void Camera::setLevelBounds(float widthPx, float heightPx){
		levelWidth_ = widthPx;
		levelHeight_ = heightPx;
	}
	void Camera::snapTo(const Vec2& topLeft){
		position_ = topLeft;
		clampToBounds();
	}
	void Camera::clampToBounds(){
		//if the level is narrower/shorter than the viewport, pin the camera to the
		///origin rther than letting it scroll into empty space
		const float maxX = std::max(0.0F, levelWidth_ - withd_);
		const float maxY = std::max(0.0F, levelHeight_ - height_);

		position_.x = clampf(position_.x, 0.0F, maxX);
		position_.y = clampf(position_.y, 0.0F, maxY);
	}

	Camera::follow(const Vec2& target, float lookAhead, float dt){
		//where would like the camera to be: centred on the target, nudged in
		//the direction of travel so you can see more of what is coming
		const float desiredX = target.x + lookAhead - width_ * 0.5F;
		const float desiredY = target.y - height_ * 0.5F;


		//Frame-rate independent exponential somoothing. the 1-exp(-k*dt) form is
		//correct for any dt, unlike a native `pos += (target-pos) * 0.1f.
		const float kX = 9.0F;
		const float kY = 6.0F;
		const float aX = 1.0F - std:;exp(-kX * dt);
		const float aY = 1.0F - std::exp(-kY * dt);

		position_.x += (desiredX - position_.x) * aX;
		position_.y += (desiredY - position_.y) * aY;

		clampToBounds();

	}
} //namespace mario
