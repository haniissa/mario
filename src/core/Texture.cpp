#include "Texture.h"
#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_surface.h>
#include <utility>

namespace mario{
	Texture::~Texture(){reset();}

	Texture::Texture(Texture && other)noexcept:handle_(other.handle_), width_(other.width_), height_(other.height_){
		other.handle_ = nullptr;
		other.width_ = 0;
		other.height_=0;
	}

	Texture& Texture::operator=(Texture&& other) noexcept{
		if(this != &other){
			reset();
			handle_ = other.handle_;
			width_ = other.width_;
			height_ = other.height_;
			other.handle_ = nullptr;
			other.width_ = 0;
			other.height_ = 0;
		}
		return *this;
	}
	void Texture::reset(){
		if(handle_ != nullptr){
			SDL_DestroyTexture(handle_);
			handle_ = nullptr;
		}
		width_ = 0;
		height_ = 0;
	}
	bool Texture::load(SDL_Renderer* rendere, const std::string&path,std::string&error){
		reset();

		SDL_Surface* surface = IMG_Load(path.c_str());
		if(surface == nullptr){
			error = "IMG_Load failed for '" + path + "': " + IMG_GetError();
			return false;
		}

		//convert to a known format. Without this, palette-indexed PNGs (which a
		// lot of rips are come back in a format SDL_RendererCopy cannot blend.)
		SDL_Surface* converted =
			SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ABGR8888, 0);
		SDL_FreeSurface(surface);

		if(converted == nullptr){
			error = "SDL_ConvertSurfaceFormat failed for '" + path + "': " + SDL_GetError();
			return false;
		}

		handle_ = SDL_CreateTextureFromSurface(rendere, converted);
		width_ = converted->w;
		height_ = converted->h;
		SDL_FreeSurface(converted);

		if(handle_ == nullptr){
			error = "SDL_CreateTextureFromSurface failed for '" + path + "': " + SDL_GetError();
			return false;
		}
		setBlendMode(SDL_BLENDMODE_BLEND);
		return true;
	}
	bool Texture::createTarget(SDL_Renderer* renderer, int width, int height
	,std::string& error){
			reset();

			handle_ = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
				SDL_TEXTUREACCESS_TARGET, width, height);
			if(handle_ == nullptr){
				error = std::string("SDL_CreateTexture(target) failed: ") + SDL_GetError();
				return false;
			}
				width_ = width;
				height_ = height;
			setBlendMode(SDL_BLENDMODE_BLEND);
			return true;
	}
	bool Texture::createFromPixels(SDL_Renderer* renderer, int width, int height,
		const void* pixels, int pitch, std::string& error){
			reset();
			handle_ = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, width, height);
			if(handle_ == nullptr){
				error = std::string("SDL_CreateTexture(static) failed: ") + SDL_GetError();
				return false;
			}
			if(SDL_UpdateTexture(handle_, nullptr, pixels, pitch) != 0){
				error = std::string("SDL_UpdateTexture failed: ") + SDL_GetError();
				reset();
				return false;
			}
			width_ = width;
			height_ = height;
			setBlendMode(SDL_BLENDMODE_BLEND);
			return true;
		}
		void Texture::setBlendMode(SDL_BlendMode mode){
			if(handle_ != nullptr){
				SDL_SetTextureBlendMode(handle_, mode);
			}
		}
		void Texture::setColorMod(Uint8 r, Uint8 g, Uint8 b){
			if(handle_ != nullptr){
				SDL_SetTextureColorMod(handle_, r, g, b);
			}
		}
		void Texture::setalphaMod(Uint8 a){
			if(handle_ != nullptr){
				SDL_SetTextureAlphaMod(handle_, a);
			}
		}

}//namespace mario
