#include "Input.h"
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_scancode.h>


namespace mario{
	bool Input::mapScancode(SDL_Scancode scancode, Action& out){
		switch(scancode){
			//Arrows and WASD both move.
			case SDL_SCANCODE_LEFT:
			case SDL_SCANCODE_A:
				out = Action::Left;
				return true;
			case SDL_SCANCODE_RIGHT:
			case SDL_SCANCODE_D:
				out = Action::Right;
				return true;
			case SDL_SCANCODE_UP:
			case SDL_SCANCODE_W:
				out = Action::Up;
				return true;

			case SDL_SCANCODE_DOWN:
			case SDL_SCANCODE_S:
				out  = Action::Down;
				return true;

			//Jump and run, in the calssic layout.
			case SDL_SCANCODE_SPACE:
			case SDL_SCANCODE_K:
			case SDL_SCANCODE_Z:
				out = Action::Jump;
				return true;
			case SDL_SCANCODE_LSHIFT:
			case SDL_SCANCODE_RSHIFT:
			case SDL_SCANCODE_L:
			case SDL_SCANCODE_X:
				out = Action::Run;
				return true;
			case SDL_SCANCODE_RETURN:
			case SDL_SCANCODE_ESCAPE:
			case SDL_SCANCODE_P:
				out = Action::Start;
				return true;
			case SDL_SCANCODE_M:
				out = Action::Mute;
				return true;
			case SDL_SCANCODE_F11:
				out = Action::Fullscreen;
				return true;
			///Throw a fireball (only does anything when Fire Mario)
			case SDL_SCANCODE_F:
			case SDL_SCANCODE_J:
			case SDL_SCANCODE_LCTRL:
				out = Action::Fire;
				return true;

			case SDL_SCANCODE_F1:
				out = Action::DebugToggle;
				return true;
			default:
				return false;

		}
	}
	void Input::beginFrame() {previous_ = current_;}
	bool Input::handleEvent(const SDL_Event& event){
		switch(event.type){
			case SDL_QUIT:
				quit_ = true;
				return false;
			case SDL_KEYDOWN:
			case SDL_KEYUP:{
				//Ignore auto-repeat: a held key must not look like a fresh press.
				if(event.type == SDL_KEYDOWN and event.key.repeat != 0){
					return true;
				}
				Action action{};
				if(mapScancode(event.key.keysym.scancode, action)){
					current_[index(action)] = (event.type == SDL_KEYDOWN);
				}
				if(event.type == SDL_KEYDOWN and event.key.keysym.scancode == SDL_SCANCODE_ESCAPE){
					quit_ = true;
				}
				return true;
			}
			default:
				return true;
		}
	}
		void Input::endFrame(){
			//Nothing to do yet; edges are computed against `previous_` which is
			// refreshed in beginFrame(). kept as a hook for future device polling
		}
		float Input::moveAxis()const{
			float axis = 0.0F;
			if(held(Action::Left)) axis -= 1.0F;
			if(held(Action::Right)) axis += 1.0F;
			return axis;
		}

} // namespace mario
