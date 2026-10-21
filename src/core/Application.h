#include "core/Audio.h"
#include "core/Input.h"
#include "game/World.h"
#include "gfx/VulkanPresenter.h"

namespace mario{
	class Application {
		public:
			struct Options{

				std::string levelPath = "assets/levels/grass-1-1.lvl";
				int windowScale = 3;

				/// When set, the game runs screeenshotFrame frames writes the
				/// frambuffer here, and exits. Used for automated verification
				std::string screenshotPath;
				int screeenshotFrame{120};

				///scripted input for screenshots: hold right, and optionally jump
				bool scriptRunRight{false};
				bool scriptJump{false};

				///Start Mario at this tile calumn instead of the level's spawn point
				/// -1 means "use the leve's own spawn"
				int startTileX{-1};

				///Disable vertical sync. Only useful for diagnosing frame pacing
				///with vsync off the loop runs af faster as the machine allows
				bool noVsync{false};

				///print fps / steps once a second. Diagnostics for frame pacing
				bool fpsLog{false};

				///Start with sound off. the M key toggles it at any time
				bool muted{false};

				///skip the looping background music, keeping the effects.
				bool noMusic{false};

				///Explicit window size. when either is 0 the size comes from
				/// windowScale instead the Window is always resizable at runTime
				int windowWidth{0};
				int windowHeight{0};

				//Start borderLess fullScreen F11 toggles it at any time
				bool fullscreen{false};

				///Present the game frome thought Vulkan (vk-bootstrap) instead of
				/// SDL_RenderCopy. Falls back to SDL if the vulkan stack cannot start
				bool vulkan{false};
			}
		private:
			bool loadAssets(std::string& error);

			///Build the playlist by scanning the level directory. Starting from
			/// --level if given, otherwise the first level found
			void buildPlaylist(const std::string& firstLevelPath);

			///Load playlist [playlistIndex_], keeping score/coins/lives
			bool loadCurrentLevel(std::string& error);

			void handleEvents();
			void step(float dt);
			void render();
			void presentToWindow();

			///Read the offscreen frambuffer and write it to `path` as BMP
			bool captureFramebuffer(const std::string& path, std::string& err) const;

			Options option_;
			SDL_Window* window = nullptr;
			SDL_Renderer* renderer = nullptr;

			Texture target_; /// offscreen 400x224 render target
			World world_;
			Input input_;
			Audio audio_;
			VulkanPresenter presenter_;

			//Set by the pause key. the world stops stepping but keeps drawing so
			// the screen stays live and the banner is visible.
			bool paused_ = false;

			std::vector<std::string> playlist_;
			std::size_t playlistIndex{0};
			bool firstLevelLoaded{false};

			///Renderer frames, and fixed simulation steps. These are NOT the same
			///number: a headless run with a hidden window can present for faster than
			///60Hz, so the render loop spins while the simulation still advances in
			/// 1/60s steps, anything scripted or captured must key off  `simSteps` or
			/// it becomes timing-dependent
		 	int frameCounter{0};
			int simSteps_{};
			bool running_{false};
	};
} // namespace mario
