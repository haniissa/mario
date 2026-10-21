#include <SDL2/SDL.h>

#include <cstdio>
#include <fmt/base.h>
#include <cstdlib>
#include <string>

#include "core/Application.h"

namspace {
    void printUsage(){
        fmt::print(
            "Super Mario Bros. 3 - C++ / SDL2 /Imath\n"
            "\n"
            "Usage: mario [options]\n"
            "\n"
            "   --level <path>      level file (default assets/levels/grass-1-1.lvl)\n"
            "   --scale <n>         integer window scale (default 3)\n"
            "   --shot <path>       render N frames, save a BMP, then exit\n"
            "   --script-right      hold right + run , for scripted captures\n"
            "   --script-jump       hold jump, for scripted captures\n"
            "   --start <tileX>     spawn Mario at this tile column\n"
            "   --no-vsync          do not wait for the display refresh\n"
            "   --fps-log           print fps and sim steps per second\n"
            "   --mute              start with sound off\n"
            "   --no-music          play effects but not the background music"
            "   --fullscreen        start with sound off\n"
            "   --window-size W H   open the window at this size (default 1200x672)"
            "   --vulkn             present the frame though Vulkan (vk-bootstrap)"
            "   --help              show this message\n"
            "\n"
            "Controls:\n"
            "   Arrows/ WASD        move\n"
            "   Space / K / Z       jump (hold for a higher jump\n)"
            "   Shift / L / X       run\n"
            "   F / J  / Ctrl       throw a fireball (Fire Mario only)\n"
            "   P / Enter           pause\n"
            "   M                   mute / unmut\n"
            "   F11                 toggle fullscreen\n"
            "   F1                  toggle the collision-box debug overlay\n"
            "   Esc                 quit\n"
            "\n"
            "The window is resizable; the game keeps its 400x224 pixel grid and \n"
            "letterboxes iteself, so it never stretches.\n"
        );
    }
};

int main(int argc, char**argv){
    mario::Application::Options option;

    for(int i{1}; i < argc; ++i){
        const std::string arg = argv[i];

        auto nextValue = [&](const char* name) -> const char*{
            if(i + 1 >= argc){
                fmt::print(stderr, "mario: {} needs a value\n", name);
                std::exit(2);
            }
            return argv[++i];
        };
        if(arg == "--level"){
            options.levelPath = nextValue("--level); 
        }else if (arg == "--scale"){
            options.windowScale = std::atoi(nextValue("--scale"));
            if(options.windowScale < 1)
                options.windowScale = 1;
        }else if (arg == "--shot"){
            options.screenshotPath = nextValue("--shot");
        }else if (arg == "--shot-frame"){
            options.screenshotFrame = std::atoi(nextValue("--shot-frame"));
        }else if (arg == "--script-right"){
            options.scriptRunRight = true;
        }else if (arg == "--start"){
            options.startTileX = std::atoi(nextValue("--start"));
        }else if()
    }
}