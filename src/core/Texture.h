#pragma once
//-----------------------------------------------
// @file Texture.h -- a small RAII wrapper around SDL_Texture
// SDL's C API hands you a pointer you must remember to SDL_DestroyTexture()
// Wrapping it means the texture is freed when is goes out of scope, copies are
// fobidden (a texture owns exactly one handle ), and moves transfer ownership

#include <SDL2/SDL.h>
#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <string>

namespace mario{
	class Texture{
		public:
			Texture() = default;
			~Texture();

			Texture(const Texture&) =delete;
			Texture& operator=(const Texture&) = delete;

			Texture(Texture&& other) noexcept;

			///load an image from disk. Returns false and sets `error` on failure
			bool load(SDL_Renderer* renderer, const std::string& path, std::string& error);

			///Create a black texture, typically a render target
			bool createTarget(SDL_Renderer* renderer, int with, int height, std::string& error);

			///Create a texture from on in-memory RGBA pixel buffer
			bool createFromPixels(SDL_Renderer* renderer, int width, int height,const void* pixels, int pitch, std::string&  error);

			void reset();

			bool valid() const{return handle_ != nullptr;}
			SDL_Texture* handle() const{return handle_;}
			int width() const {return width_;}
			int height() const {return height_;}

			///set the blend mode used when this texture is drawn.
			void setBlendMode(SDL_BlendMode mode);

			///Se the colour modulation applied when this texture is drawn. Passing
			///white (255, 255, 255,255) is a no-op. Used for the damage flash.
			void setColorMod(Uint8 r, Uint8 g, Uint8 b);
			void setalphaMod(Uint8 a);
		private:
			SDL_Texture* handle_{nullptr};
			int width_{};
			int height_{};
	};
}
