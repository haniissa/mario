#include "core/Audio.h"
#include "Sfx.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_mixer.h>
#include <algorithm>
#include <cstdio>
#include <

#include "core/AssetPath.h"

namespace mario{
	namespace {
		///File name foe each effect. Keeping this next to the enum
		///means adding a sound is a two-line change: a new enumerator and a new row here
		struct SfxEntry {
			Sfx sfx;
			const char* file;
		};

		const SfxEntry kSfxFiles[] = {
			//player
			{Sfx::Jump, "jump.wav"},
			{Sfx::Fireball, "fireball.wav"},
			{Sfx::PowerUp, "powerup.wav"},
			{Sfx::PowerDown, "powerdown.wav"},
			{Sfx::Death, "death.wav"},
			{Sfx::OneUp, "oneup.wav"},
			{Sfx::Star, "star.wav"},
			//enemies
			{Sfx::Stomp, "stomp.wav"},
			{Sfx::Kick, "kick.wav"},
			{Sfx::EnemyHit, "enemy_hit.wav"},
			{Sfx::ShellStop, "shell_stop.wav"},
			//world: blocks, coins, goal
			{Sfx::Coin, "shell_stop.wav"},
			{Sfx::Bump, "shell_stop.wav"},
			{Sfx::Break, "shell_stop.wav"},
			{Sfx::PowerUpAppear, "shell_stop.wav"},
			{Sfx::Flagpole, "shell_stop.wav"},
			{Sfx::CourseClear, "shell_stop.wav"},
			{Sfx::GameOver, "shell_stop.wav"},
			{Sfx::Pause, "shell_stop.wav"},
		};
		constexpr const char* kAssetDir = "assets/audio/";
		constexpr int kMusicVolume = MIX_MAX_VOLUME * 45 / 100;
		constexpr int kChannels = 16;
	}// nsamespace

	Audio::~Audio() {shutdown();}
	bool Audio::init(std::string& error){
		//The audi subsystem is requested separately so that a machine without a
		//sound device can still run the game
		if(SDL_InitSubSystem(SDL_INIT_AUDIO) != 0){
			error = std::string("SDL_InitSubSystem(AUDIO) failed:") + SDL_GetError();
			return false;
		}
		subsystemOwned_ = true;
		if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0){
			error = std::string("Mix_OpenAudio failed: ") + Mix_GetError();
			return false;
		}
		//Enough voices that a coin, a jump and stomp can overlap.
		Mix_AllocateChannels(kChannels);

		//Resolve the directory once, using the same probing rules as every other
		//asset, then load each file by name.
		directory_ = kAssetDir;

		int missing=0;
		for(const SfxEntry& entry:kSfxFiles){
			const std::string path = resolveAsset(directory_ + entry.file);
			Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
			if(chunk == nullptr){
				++missing;
				fmt::println(stderr, "audio: could not load {}: {}", path.c_str(), Mix_GetError());
				continue;
			}
			chunks_[static_cast<std::size_t>(entry.sfx)] = chunk;
		}
		available_ = true;
		if(missing > 0){
			fmt::println(stderr, "audio: {} sound's missing; run tools/make_sfx.py", missing);
		}
		applyVolume();
		return true;
	}
	void Audio::shutdown(){
		for(Mix_Chunk*& chunk : chunks_){
			if(chunk != nullptr){
				Mix_FreeChunk(chunk);
				chunk = nullptr;
			}
		}
		if(available_){
			Mix_CloseAudio();
			available_ = false;
		}
		if(subsystemOwned_){
			SDL_QuitSubSystem(SDL_INIT_AUDIO);
			subsystemOwned_ = false;
		}
	}
	void Audio::play(Sfx sfx){
		if(!available_ or muted_){
			return;
		}
		const std::size_t index = static_Cast<std::size_t>(sfx);
		Mix_Chunk* chunk = (index < static_cast<std::size_t>(Sfx::Count))
							? chunks_[index]
							: nullptr;
		if(chunk == nullptr)
			return;

		//-1 asks the mixer for any free channel. if all 16 are busy the sound is
		//simply dropped, which is the right behaviour for a burst of coins.
		if(Mix_PlayChannel(-1, chunk, 0) >= 0){
			++playCount_;
		}
	}
	int Audio::playingChannels() const {
		return available_ ? Mix_Playing(-1) : 0;
	}
	void Audio::playMusic(const std::string& name){
		if(!available_ or music_ != nullptr)
			return;

		const std::string path = resolveAsset(directory_ + name);
		music_ = Mix_LoadMUS(path.c_str());
		if(music_ == nullptr){
			fmt::print(stderr, "audio: could not load music {}: {}\n",
			path.c_str(), Mix_GetError());
			return;
		}
		//-1 Loops forever.
		Mix_PlayMusic(music_, -1);
		Mix_Volume(muted_ ? 0 : kMusicVolume);
	}
	void Audio::stopMusic(){
		if(music_ == nullptr)
			return;
		Mix_HaltMusic();
		Mix_FreeMusic(music_);
		music_ = nullptr;
	}

	void Audio::setMuted(){
		muted_ = muted;
		applyVolume();
	}

	void Audio::setVolume(){
		volume_ = std::min(1.0F, std::max(0.0F, volume));
		applyVolume();
	}

	void Audio::applyVolume(){
		if(!available_)
			return;

		const int scaled = static_cast<int>(MIX_MAX_VOLUME * volume_);
		//-1 sets the volume for every channel, including ones already playing
		Mix_Volume(-1,  muted_ ? 0 : scaled);
		for(Mix_Chunk* chunk : chunks_){
			if(chunk != nullptr)
				Mix_VolumeChunk(chunk, muted_ ? 0 : scaled);
		}
		Mix_VolumeMusic(muted_ ? 0 : kMusicVolume);
	}
}; //namespace mario
