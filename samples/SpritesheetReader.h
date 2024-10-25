#pragma once
#pragma once
#include "Spritesheet.h"

namespace QLE {
    namespace TextureTools {
        class SpritesheetReader {
        public:
            static std::vector<Spritesheet> ReadFromPath(std::string folderPath); 
            static Spritesheet ReadFile(std::string filePathPng,std::string filePathJson); 
        };
    }
}