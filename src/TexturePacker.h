#pragma once
#include <string>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;
namespace QLE {
    namespace TextureTools {

#define DEFAULT_SHEET_SIZE 2048
        using std::string;
        using std::vector;

        // Struct to store image data and metadata
        struct ImageData {
            std::string path;
            int width, height, channels;
            uint8_t* data;
        };
        struct PackingSettings {
            // always power of two
            int MaxTextureSize = DEFAULT_SHEET_SIZE;
            // folder where the individual textures / .json files are located
            std::filesystem::path InputDirectory;
            // folder where the generated texture atlas / individual textures will be created
            std::filesystem::path OutputDirectory;
            // folder where the pivot rules are located
            std::filesystem::path RulesDirectory;
            // spritesheet default name
            std::string Group = "general";
            bool recursive = true;
            // if true, this will use pngquant compression
            bool useCompression = false;
            // by default, all pivots are set to 0.5,0.5
            // if set to true, we're going to override the pivot as long as you write a rule for it
            bool overridePivot = false;

            inline bool IsSizeWithinRange() const {
                return
                    MaxTextureSize == 16 ||
                    MaxTextureSize == 32 ||
                    MaxTextureSize == 64 ||
                    MaxTextureSize == 128 ||
                    MaxTextureSize == 256 ||       // 0.25 MB
                    MaxTextureSize == 512 ||       // 1 MB
                    MaxTextureSize == 1024 ||      // 4 MB
                    MaxTextureSize == 2048 ||      // 16 MB
                    MaxTextureSize == 4096;        // 64 MB
                //MaxTextureSize == 8192 ||    // 256 MB
                //MaxTextureSize == 16384 ||   // 1 GB <- I really hope you know what you're doing
                //MaxTextureSize == 32768;     // 4 GB
            }
        };
        enum PackingMode {
            None,
            Pack,
            Unpack,
            Error
        };
        class TexturePacker
        {
        private:
            PackingSettings settings;
            string compressionTool;

            /* Packing */
            // Load image and metadata
            ImageData loadImage(const fs::path& imagePath);
            // Function to compute the smallest power-of-two size that fits the dimensions
            int nextPowerOfTwo(int x);
            // Pack images into texture sheets and handle multiple sheets if needed
            void packImagesIntoSheets(const std::vector<fs::path>& imagePaths, const fs::path& outputDir, const std::string& Group);
            // Used when checking if a file is a valid image that we can use to pack within the spritesheet
            void checkIfCanAddImage(vector<fs::path>& images, const std::filesystem::directory_entry& entry);
            // Used to check if path 2 is found within path 1
            bool isSubdirectory(const std::filesystem::path& path1, const std::filesystem::path& path2);

            /* compression */
            void optimizePngInOutputDir(std::filesystem::path spritesheet);

            /* Unpacking */
            // Function to extract sprites from the texture sheet using data from the JSON file
            void extractSpritesFromJson(const fs::path& jsonFilePath);
            // Used when checking if a file is a valid json and if the spritesheet it's related to still exists
            void checkIfCanAddJson(vector<fs::path>& jsons, const std::filesystem::directory_entry& entry);
            void showBanner();

            /* Pivot setting */
            void overridePivot();
            void setPivot(fs::path rule);
        public:
            TexturePacker();
            bool IsCompressionToolInPath() const;
            bool IsExtensionSupported(string extension) const;
            // shows help menu
            void ShowHelp();
            /*
            * pass a vector that separates each option like "-o=<directory>"
            * returns true if everything is good
            * returns false if bad
            */ 
            bool Run(vector<string> args);
            /*
            * pass a string that contains all the options like "-p -i=<directory> -o=<directory>"
            * returns true if everything is good
            * returns false if bad
            */
            bool Run(string commands);
            /*
            * combines individual textures into atlases and exports .png and .json
            * returns true if everything is good
            * returns false if bad
            */
            bool Pack(PackingSettings settings);
            /*
            * extracts individual textures from atlases and creates their original folder directory in the chosen path
            * returns true if everything is good
            * returns false if bad
            */
            bool Unpack(PackingSettings settings);
        };
    }
}