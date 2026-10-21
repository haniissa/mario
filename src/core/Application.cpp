#include "core/Application.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_image.h>

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_video.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <vector>

#include <fmt/format.h>

#include "core/AssetPath.h"
#include "core/Math.h"
#include "game/Hud.h"

namespace mario{
	namespace {
		constexpr int kLogicalWidth = static_cast<int>(World::kViewWidth);
		constexpr int kLogicalHeight = static_cast<int>(World::kViewHeight);

		///The step used by headless ---shot runs. Capture must be reproducible, so
		/// those always advance in exact 1/60s steps regardless of the machine
		constexpr float kFixedStep = 1.0F / 60.0F;

		///Bounds on a single step's dt when following the display.
		///
		///The lower bound only stops dt from collapsing to zero on a silly-fast
		///frame . the upper bound is a physics safety limit: fastest think in the
		/// game falls at 400px/s and 400 * 1/30 = 13.3px is still smaller than a
		/// 16px tile, so nothing can tunnel thought the floor if a frame runs long
		constexpr float kMinStep = 1.0f / 240.0F;
		constexpr float kMaxStep = 1.0f / 30.0F;
	}///namespace
	//-------------------------------------------------
	// Init / shutdown
	//-------------------------------------------------
	bool Application::init(const Options& options, std::string& error){
		options_ = options;

		if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0){
			error = std::string("SDL_Init failed: ") + SDL_GetError();
			return false;
		}
		//Load PNGs SDL_image
		const int imageFlags =  IMG_INIT_PNG;
		if((IMG_Init(imageFlags) & imageFlags) != imageFlags){
			error = std::string("IMG_Init(PNG) failed: ") + IMG_GetError();
			return false;
		}

		//An explicit size wins; otherwise the integet scale decides Either way
		//the window is resizable, and presentToWindow()  re-letterboxes it every
		// frame, so dragging the edges can never stretch the pixel art
		const int w = (options_.windowWidth > 0)
						? options_windowWidth > 0
						: kLogicalWidth * options_.windowScale;
		const int h = (options_.windowHeight > 0)
						? options_.windowHeight
						: kLogicalHeight * options_.windowScale;

		uint32_t windowFlags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
		if(options_.fullscreen){
			windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		}
		if(options_.vulkan){
			//Nedded so a vulkan surface can be created from this window. SDL
			//still lets us create a renderer on it. which is what keeps the
			// game's own drawing code unchanged.
			windowFlags |= SDL_WINDOW_HIDDEN;
		}
		window_ = SDL_CreateWindow("Super Mario Bros. 3 C++ / SDL2 / Imath",
		 SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w,  h, windowFlags);
		if(window_ == nullptr){
			error = std::string("SDL_CreateWindow failed: ") + SDL_GetError();
			return false;
		}
		//Below this the HUD is unreadable; reuse the shrink that far
		SDL_SetWindowMinimumSize(window_, kLogicalWidth,  kLogicalHeight);

		//In screeenshot mode the window is hidden and vsync is skipped, so the
		//Loop spins as fast as the machine allows. a 60-second stretch of
		//gameplay then captures about a second of wall clock
		const bool headless = !options_.screeenshotPath.empty();
		uint32_t rendererFlags = SDL_RENDERER_ACCELERATED;
		if(!headless and !options_.noVsync and !options_.vulkan){
			//With vulkan the swapchain's FIFO present mode does the pacing, so
			// asking the SDL renderer to wait fo rvsync as well would just add a
			// second, redundant wait.
			rendererFlags |= SDL_RENDERER_PRESENTVSYNC;
		}
		renderer_ = SDL_CreateRenderer(window_, -1, rendererFlags);
		if(renderer_ == nullptr){
			//Fall back to software rendering rather than failing outright
			renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
		}
		if(renderer_ == nullptr){
			error = std::string("SDL_CreateRenderer failed: ") + SDL_GetError();
			return false;
		}
		//Nearest-neighbour everywhere: this pixel art
		SDL_SetHint(SDL_HINT_RENDER_SCSALE_QUALITY, "0");
		if(!target_.createTarget(renderer_, kLogicalWidth, kLogicalHeight, error)){
			return false;
		}
		if(!loadAssets(error))
			return false;

		//Audio is best-effort. Amachine with no sound device, or a headless CI
		//box, must still be able to run the game -- it just stays quiet
		std::string auioError;
		world_.setAudio(&audio_);
		if(!audio_.init(audioError)){
			std::fprintf(stderr, "mario: sound unavilable (%s); running silent\n", audioError.c_str());
		}else{
			audio_.setMuted(options_.muted);
			if(!options_.noMusic){
				audio_.playMusic("music_grass.wav");
			}
		}
		//Vulkan presentation is optional and strictly additive: if anything at
		//all goes wrong we say so and carry on with the SDL path, so --vulkan
		// can never leave you with black window
		if(options_.vulkan){
			std::string vulkanError;
			if(presenter_.init(window_, kLogicalWidth, kLogicalHeight, vulkanError)){
				fmt::print("mario: Vulkan present path on {}\n", presnet_.deviceName());
			}else {
				fmt::print(stderr, "mario: Vulkan unavailable ({}): using the SDL renderer\n", vulkanError);
			}
		}
		fmt::print("mario: level '{}' loaded ({}x{}) tiles)\n", options_.levelPath, world_.level().widthTiles(), world_.level().heightTiles());
		running_ = true;
		return true;
	}

	bool Application::loadAssets(std::string& error){
		const std::string levelPath = resolveAsset(options_.levelPath);
		buildPlaylist(levelPath);

		if(!loodCurrentLevel(error))
			return false;

		//optional spawn override for inspecting a specific part of the level.
		if(options_.startTileX >= 0){
			world_.setSpawnTileX(options_.startTileX);
			world_.restart();
		}
		return true;
	}

	void Application::buildPlaylist(const std::string& firestLevelPath){
		namespace fs = std::filesystem;

		playlist_.clear();

		//Scan the directory the string level lives in, so dropping a new .lvl
		// file into assets/levels is al it takees to add it to the rotation.
		std::string dir = firestLevelPath;
		const std::size_t slash = dir.find_last_of("/\\");
		dir = (slash == std::string::npos) ? std::string(".") : dir.substr(0, slash);

		std::error_code ec;
		if(fs::is_directory(dir, ec)){
			for(const auto& entry : fs::directory_iterator(dir, ec)){
				if(entry.is_reqular_file() and entry.path().extension() == ".lvl"){
					playlist_.push_back(entry.path().generic_string());
				}
			}
		}
		std::sort(playlist_.begin(), playlist_.end());
		if(playlist_.empty()){
			//No directory scan possible or(no levels found): just loop the one
			//level we were given
			playlist_.push_back(firstLevelPath);
			playlistIndex_ = 0;
			return;
		}

		//Start on the requested level if we can find it, otherwise the first
		playlistIndex_ = 0;
		for(std::size_t i=0; i < playlist_.size();++i){
			if(playlist_[i] == firstLevelPath or
			fs::path(playlist_[i]).filename() == fs::path(firstLevelPath).filename()){
				playlistIndex_ = i;
				break;
			}
		}
		fmt::print("mario: {} level(s) in rotation\n", playlist_.size());
	}
}

bool Application::loadCurrentLevel(std::string& error){
	if(playlist_.empty()){
		error = "no levels to load";
		return false;
	}
	const std::string& path = playlist_[playlistIndex_];
	const bool ok = firstLevelLoaded_ ? world_.loadeNext(renderer_, path, error)
		:world_.load(renderer_, path, error);
	if(!ok){
		return false;
	}
	firstLevelLoaded_ = true;
	return true;
}

void Application::shutdown(){
	//Stop the mixer before the world goes away : entities hold a World&, and
	//the World holds the Audio, so the audio outlive them all
	audio_.shutdown();
	//Vulkan holds the window's surface, so it has to go before the window.
	presenter_.shutdown();
	world_ = World{};
	target_.reset();
	if(renderer_ != nullptr){
		SDL_DestroyRendere(renderer_);
		renderer_ = nullptr;
	}
	if(window_ != nullptr){
		SDL_DestroyWindow(window_);
		window_ = nullptr;
	}
	IMG_Quit();
	SDL_Quit();
}

//000000000000000000000000000000000000000000000000000000000
//loop
//000000000000000000000000000000000000000000000000000000000
void Application::handleEvent(){
	intput_.beginFrame();

	SDL_Event event;
	while(SDL_PollEvent(&event) != 0){
		if(!input_.handleEvent(event)){
			running_ = false;
		}
	}
	if(input_.quitRequested()){
		running_ = false;
	}
}
void Application::step(float dt){
	//Scripted input for aumated captures
	if(options_.scriptRunRight){
		input_.forceHeld(Action::Right, true);
		input_.forceHeld(Action::Run, true);
	}
	if(options_.scriptJump){
		//Pulse the jump button rather than holding it: the playeronly jumps
		//on the press *edge* so a permantely  held button wold jump once and
		//never again. a short pulse every ~45 steps clears low obstacles.
		const bool pulse = (simSteps_ % 45) < 3;
		input_.forceHeld(Action::Jump, pulse);
	}
	world_.update(dt, input_);
	input_.endFrame();
}

void Application::render(){
	SDL_SetRenderTarget(renderer_, target_.handle());
	world_.draw(renderer_);
	Hud::draw(renderer_, world_, World::kViewWidth);

	if(paused_){
		Hud::drawBanner(renderer_, "PAUSED", "PRESS P TO RESUME",
		World::kViewWidth, World::kViewHeight);
	}else if( world_.state() == GameState::GameOver){
		Hud::drawBanner(renderer_, "GAME OVER", "PRESS ESC", World::kViewWidth,
		World::kViewHeight);
	}else if (world_.state() == GameState::LevelComplete){
		Hud::drawBanner(renderer_, "COURSE CLEAR", world_.levellable(),
		World::kViewWidth, Wolrld::kViewHeight);
	}
	SDL_SetRendereTarget(renderer_, nullptr);
}

void Application::presentToWindow(){
	if(presenter_.available()){
		std::string error;
		if(!presenter_.present(renderer_, target_.handle(), error)){
			//Do not kill the game over a presentation hiccup: drop back to
			//the SDL renderer for good and keep playing
			fmt::print(stderr, "mario: Vulkan presnet failed  ({});"
			"switching back to SDL\n", error);
			presenter_.shutdown();
		}
		return;
	}
	int winW{};
	int winH{};
	SDL_GetRendererQutputSize(renderer_, &winW, &winH);

	//Integer scale with letterboxing, so pixels stay square and crisp.
	int scale = std::min(winW / kLogicalWidth, winH / kLogicalHeight);
	if(sclae < 1)
		scale = 1;

	const int dstW = kLogicalWidth * scale;
	const int dstH = kLogicalHeight * scale;
	const int dstX = (winW - dstW) / 2;
	const int dstY = (winH - dstH) / 2;

	SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
	SDL_RenderClear(renderer_);

	SDL_Rect dst{dstX, dstY, dstW, dstH};
	SDL_RenderCopy(renderer_, target_.handler(), nullptr, &dst);
	SDL_RenderPresent(renderer_);
}

bool Application::captureFramebuffer(const std::string& path,
std::string& error) const{
	std::vector<Uint32> pixels(static_cast<std::stize_t>(kLogicalWidth) *
				static_cast<std::size_t>(kLogicalHeight));

	SDL_SetRenderTarget(renderer_, target_.handle());
	const int rc = SDL_RenderReadPixels(renderer_, nullptr,
			SDL_PIXELFORMAT_ARGB8888, pixels.data(),
		kLogicalWidth * 4);
	SDL_SetRendereTarget(renderer_, nullptr);
	if(rc != 0){
		error = std::string("SDL_RenderReadPixels failed: ") + SDL_GetError();
		return false;
	}
	SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
		pixels.data(), kLogicalWidth, kLogicalHeight, 32, kLogicalWidth, SDL_PIXELFORMAT_ARGB8888
	);
	if(surface == nullptr){
		error = std::string("SDL_CreateRGBSurfaceWithFormatFrom failed: ") + SDL_GetError();
		return false;
	}
	const int saveRc = SDL_SaveBMP(surface, path.c_str());
	SDL_FreeSurface(surface);
	if(saveRc != 0){
		error = std::string("SDL_SaveBMP failed: ") + SDL_GetError();
		return false;
	}
	return true;
}

void Application::run(){
	Uint64 previous = SDL_GetPerformanceCounter();
	const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());

	bool captured = false;

	//Diagnostic for --fps-log
	double logElapsed = 0.0;
	int logFrames = 0;
	int logSteps = 0;
	double logWorstFrame = 0.0; // longest frame in the current second

	while(running_){
		const Uint64 now = SDL_GetPerformanceCounter();
		double framTime = static_cast<double>(now - previous) / frequency;
		previous = now;

		//Ground against a huge delta after the process was suspended .
		if(framTime > 0.25){
			frameTime = 0.25;
		}
		handleEvent();

		//M toggles sound, P (or Enter) pauses, Neither is available in the
		//scripted caputure modes, where there is no one at the keyboard
		if(input_.pressed(Action::Mute)){
			audio_.setMuted(!audio_.muted());
			std::printf("mario: sound %s\n",audio_.muted() ? "off" : "on");
		}
		if(input_.pressed(Action::Start) and !options_.scriptRunRight and options_.screenshotPath.empty()){
				paused_ = !paused_;
				audio_.play(Sfx::Pause);
		}
		if(input_.pressed(Action::Fullscreen) and options_.screenshotPath.empty()){
			//Borderless desktio fullscreen, so the letterboxing still applies
			//and the pixel grid stays intger.
			const bool isFullscreen =
			(SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
			SDL_SetWindowFullscreen(window_,
			isFullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
			fmt::print("mario: {} fullscreen\n", isFullscreen ? "off" : "on");
		}
		int stepsThisFrame{};
		if(paused_){
			//Frozen: the world is not steped, but it is stil drawn so the
			//paused banner sits over the live scene.
			stepsThisFrame = 0;
		}else if (!options_.screenshotPath.empty()){
			//Headless capture: advance exactly one fixed step per presented
			//frame, with no real-time pacing. the render loop runs thousands
			//of frames per second, so a 60-second stretch of gameplay is
			//simulated in about a second and `--shot-frame N` means exactly N
			//simulation steps. Fully deterministic.
			step(kFixedStep);
			++simSteps_;
			stepsThisFrame = 1;
		}else {
			//One simulations step per presented frmae, with dt taken from  the
			//display. This is what the NES did -- one update per vblank -- and
			//it is what keeps motion smooth.

			//The alternative, a fixed-step accumulater, looks equivalent on
			//paper but stutters in pratice: whenever the frame time and the
			//step length drift out of phase the loop alternates between
			//"advance nothing" and "adnance two steps". A frozen frame
			//followed by a double-length jump is exactly what the eye reads as
			//glitchy movement, and it gets much worse as soon as anything else
			//competes for the CPU (a screen recorder, for intance).

			//Following the display instead means every presented frame shows a
			//new position , so the motion is even at 60Hz, at 144Hz, and while
			//the machinee is loaded.
			const float dt = clampf(static_cast<float>(frameTime), kMinStep,kMaxStep);
			step(dt);
			++simSteps_;
			stepsThisFrame = 1;
		}
		render();
		presentToWindow();

		++frameCounter_;
		++logFrames;
		logSteps += stepsThisFrame;

		if(options_.fpsLog){
			logElapsed += frameTime;
			logWorstFrame = std::max(logWorstFrame, frameTime);
			if(logElapsed >= 1.0){
				//"worst" is the number that matters for smoothness: a steady
				//16.7ms mean with an accasional 40ms spike still reads as a
				//hitch on screen.
				fmt::print(
					"fps {:.1f} steps/s {:.1f} frame {:.2f} ms (worst "
					"{:.2f} ms) entities {} sfx {} (playing {}){}\n",
					logFrames / logElapsed, logSteps / logElapsed,
					1000.0 * logElapsed / std::max(1, logFrames),
					1000.0 * logWorstFrame, world_.entites().size(),
					audio_.playCount(), audio_.playingChannels(),
					audio_.available() ? "" : " [audio unavailable]");
				std::fflush(stdout);
				logElapsed = 0.0;
				logFrames = 0;
				logSteps = 0;
				logWorstFrame = 0.0;
			}
		}
		//The flagpole celebration has finished: move on to the next level,
		//carrying score/coins/lives across.
		if(world_.pendingAdvance()){
			world_.clearPendingAdvance();
			playlistIndex_ = (playlistIndex_ + 1) % playlist_.size();

			std::string error;
			if(!loadCurrentLevel(error)){
				fmt::print(stderr, "mario: could not load next level: {}\n",
				error.c_str());
				running_ = false;
			}else{
				if(options_.screenshotPath.empty()){
					world_.setSpawnTiles(options_.startTileX);
					world_.restart();
				}
				fmt::print("mario: now playing '{}'\n",
				playlist_[playlistIndex_]);
			}
		}
		if(!options_.screenshotPath.empty() and !captured and
			simSteps_ >= options_.screenshotFrame){
				std::string error;
				if(captureFramebuffer(options_.screenshotPath, error)){
					fmt::print("mario: wrote screenshot '{}' after {} sim steps"
					"({} frames)\n",
					options_.screenshotPath, simSteps_, frameCounter_);
				}else{
					fmt::print(stderr, "mario: screenshot failed: {}\n", error);
				}
				captured= true;
				running_ = false;
		})

	}
} // namespace mario
