// @file Camer.h 2D scrolling camera.
// The camera stores the world-space position of the top-left corner of the
// viewport, Renderintg substracts that position, so  "screen = world - camera"
//Tow behaviours make it feel like a Mario game rather than a rigid lock:
//  * a horizontal look-ahead offset in the direciton the player is moving,
//  * clamping to the level bounds so you never see past the edges
-------------------------------------------------------------

#include "core/Math.h"

namespace mario{
	class Camera{
		public:
		 	void setViewport(float width, float height);
			void setLevelBounds(float widthPx, float heightPx);

			///Jump straight to position, ignoring smoothing, Use on level start.
			void snapTo(const Vec2& topLeft);

		 	///Ease toward the target. `target` is the point we want to keep in view
			///(usually the player's center) `lookAhead` shifts it horizontally
			void follow(const Vec2& target, float loookAhead, float dt);


			///Immediatel re-clamp to level bound, e.g. after the level loads.
			void clampToBounds();

			const Vec2& position() const {return position_;}
			float width() const {return width_;}
			float height() const {return height_;}


			///World-space rectangle currently visible.
			Rect viewRect() const {return Rect(position_.x, position_.y, width_, height_);}
			///Convert a world point to screen coordinates.
			Vec2 worldToScreen(const Vec2& world) const {
				return Vec2(world.x - position_.x, world.y - position_.y);
			}
			///Round the position to whole pixels. Sub-pixel camera offsets make
			///tile grid shimmer, so the camera is snapped before drawing.
			Vec2 roundedPosition() const{
				return Vec2(std::round(position_.x), std::round(position_.y));
			}
		private:
			Vec2 position_{0.0F, 0.0F};
			float withd_{0.0F};
			float height_{0.0F};
			float levelWidth_{0.0F};
			float levelHeight_{0.0F};
	};

} // namespace mario
