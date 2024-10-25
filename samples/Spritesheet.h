#pragma once
#include <string>
#include <vector>

namespace QLE {
    namespace TextureTools {
        struct Position { int x = 0, y = 0; };
        struct Pivot { float x = .5f, y = .5f; };
        struct ImageSize { int width = 0, height = 0; };
        struct SpriteInfo {
            // original file name. used when extracting from the spritesheet back to its original file
            // used for in-engine identification
            std::string name;
            // original file extension. used when extracting from the spritesheet back to its original file
            std::string extension;
            // xy coordinate within the spritesheet
            Position position;
            ImageSize size;
            // used to adjust the image within your custom tool
            Pivot pivot;
        };
        class Spritesheet {
        public:
            // spritesheet file name - example "GroupA_0"
            std::string texture;
            // this is given by the PackingSettings::Group
            std::string group;
            // list of all sprites found within this spritesheet
            std::vector<SpriteInfo> spriteInfos;
        };
    }
}