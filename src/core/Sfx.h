#pragma once

//---------------------------------------------------
// @file Sfx.h --- the list of sounds the game can make.
// Deliberately separate from Audio.h gameplay code (Player, World, Enemies)
// only every names a sound, it never touches SDL_mixer. That keeps the mixer
// headers out of the game logic and makes it obvious at a glance which events
// are meant to audible.
// ---------------------------------------------------
//
#include <cstddef>

namespace mario {
	enum class Sfx : std::size_t {
		//-----------player---------------------
		Jump,      // Mario leaves the ground
		Fireball,  // a fireball is thrown
		PowerUp,   //Mario grows or gains fire
		PowerDown,     //Mario is hit and shrinks
		Death,       //Mario dies
		OneUp,     //an extra life
		Star,      //invinciblity collected
		//---------enemies-----------------------
		Stomp,            // an enemy if flattened underfoot
		Kick, 			  // a shell is kicked (or an enemy bumped from below)
		EnemyHit,         // an enemy is knocked out by a shell, star or fireball
		ShellStop,        //a sliding shell comes to rest

		//-------------world: blocks, coins and goal-------------
		Coin, 			//a coin is banked
		Bump, 			//a block is head-butted but does not break
		Break,  		//a brick shatters
		PowerUpAppear,  //a mushroom/flower pops out of a block
		Flagpole,		//the goal pole is grabbed
		CourseClear,    //the level is finished
		GameOver,       //no lives left
		Pause,          //pause toggled

		count
	};
}// namespace mario
