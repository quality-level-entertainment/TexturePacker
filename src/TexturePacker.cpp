#include "TexturePacker.h"
#include <iostream>
#include <algorithm>

// don't include stb libraries into the header files. it will cause LNK2005 errors
// also, if you decide to use stb libraries as part of your code, be sure to set these defines once only
#define STB_RECT_PACK_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "../include/stb_image.h" // For reading images
#include "../include/stb_image_write.h"  // For saving the output image
#include "../include/stb_rect_pack.h" // For packing the textures within a certain rectangle

#include "../include/json.hpp" // For exporting/importing json
#include <fstream> // For reading .json
#include <sstream> // For separating options from a single string

namespace fs = std::filesystem;
using std::cout;
using std::cerr;
using std::endl;

#if defined(_WIN32) || defined(_WIN64)
#define TP_COMPRESSION_TOOL "pngquant.exe";
#else
#define TP_COMPRESSION_TOOL "pngquant";
#endif

namespace QLE {
    namespace TextureTools {
        TexturePacker::TexturePacker()
        {
            showBanner();
            compressionTool = TP_COMPRESSION_TOOL;
        }
#pragma region General
        void TexturePacker::showBanner()
        {
            cout << endl << endl;
            cout << "     _______________________________" << endl;
            cout << "     [                             ]" << endl;
            cout << "     [        Texture Packer       ]" << endl;
            cout << "     [              by             ]" << endl;
            cout << "     [ Quality Level Entertainment ]" << endl;
            cout << "     [_____________________________]" << endl;
            cout << endl << endl;
        }
        bool TexturePacker::isSubdirectory(const fs::path& path1, const fs::path& path2)
        {
            // Normalize paths to ensure consistent comparison
            fs::path normalized_path1 = path1.lexically_normal();
            fs::path normalized_path2 = path2.lexically_normal();

            // Check if path1 starts with path2
            return normalized_path1.string().starts_with(normalized_path2.string());
        }
        bool TexturePacker::IsExtensionSupported(string extension) const
        {
            return  extension == ".png" ||
                    extension == ".jpg" ||
                    extension == ".jpeg" ||
                    extension == ".jfif" ||
                    extension == ".bmp" ||
                    extension == ".tga";
        }
        void TexturePacker::ShowHelp()
        {
            showBanner();
            cout << "Usage:" << endl;
            cout << "\t-help = displays this help menu" << endl;
            cout << "\t-p = enable packing mode. reads images and exports .png and .json" << endl;
            cout << "\t-u = enable unpacking mode. reads .json files and exports individual textures" << endl;
            cout << "\t-i <input_directory>" << endl;
            cout << "\t-o <output_directory>" << endl << endl;
            cout << "  [ Optional ]" << endl;
            cout << "\t-size=<spritesheet_size>    | Defaults to " << DEFAULT_SHEET_SIZE << endl;
            cout << "\t-group=<spritesheet_name>   | Defaults to sheet" << endl;
            cout << "\t-pivot=<pivot_directory>    | folder path containing .json to override the pivot points of sprites" << endl;
            cout << "\t-compress                   | Compresses the output folder using \""<< compressionTool <<"\"" << endl;
            cout << "\t-nonrecursive               | Uses the input folder path only" << endl << endl;
            cout << "  [ Examples ]" << endl;
            cout << "TexturePacker.exe -p -i=\"C:\\input_folder\" -o=\"C:\\output_folder\"" << endl;
            cout << "TexturePacker.exe -p -i=\"C:\\input_folder\" -o=\"C:\\output_folder\" -size=2048 -group=\"sheet\"" << endl;
            cout << "TexturePacker.exe -u -i=\"C:\\input_folder\" -o=\"C:\\output_folder\"" << endl;
            cout << endl << endl;
        }
#pragma endregion

#pragma region Packing
        ImageData TexturePacker::loadImage(const fs::path& imagePath) {
            ImageData img;
            img.path = imagePath.string();
            img.data = stbi_load(imagePath.string().c_str(), &img.width, &img.height, &img.channels, STBI_rgb_alpha); // Force 4 channels (RGBA)
            if (!img.data) {
                throw std::runtime_error("Failed to load image: " + imagePath.string());
            }
            bool isOpaque = img.channels == 3;
            if (isOpaque) {
                for (int i = 0; i < img.width * img.height; ++i)
                    img.data[i * STBI_rgb_alpha + 3] = 255; // Set alpha channel to 255
            }
            return img;
        }
        // Function to compute the smallest power-of-two size that fits the dimensions
        int TexturePacker::nextPowerOfTwo(int x) {
            int power = 1;
            while (power < x) power *= 2;
            return power;
        }
        // Function to export sprite information to a JSON file
        void exportSpriteInfoToJson(const std::vector<stbrp_rect>& rects, const std::vector<ImageData>& images,
            const fs::path& outputTextureFileName, const PackingSettings settings) {
            nlohmann::json jsonOutput;
            jsonOutput["texture"] = outputTextureFileName.string();
            jsonOutput["group"] = settings.Group;

            for (const auto& rect : rects) {
                const ImageData& img = images[rect.id];

                // Get file name without extension
                std::string fileName = fs::path(img.path).stem().string();

                // Create a JSON object for this sprite
                nlohmann::json spriteInfo;
                spriteInfo["name"] = fileName;
                spriteInfo["extension"] = fs::path(img.path).extension();
                spriteInfo["position"] = { {"x", rect.x}, {"y", rect.y} };
                spriteInfo["pivot"] = { {"x", .5f}, {"y", .5f} };
                spriteInfo["size"] = { {"width", rect.w}, {"height", rect.h} };

                jsonOutput["sprites"].push_back(spriteInfo);
            }

            // Write the JSON data to a file
            const fs::path& outputJsonPath = fs::path(outputTextureFileName).remove_filename() / outputTextureFileName.filename().replace_extension(".json");
            std::ofstream outputFile(outputJsonPath);
            if (!outputFile.is_open()) {
                throw std::runtime_error("Failed to open JSON file for writing: " + outputJsonPath.string());
            }

            outputFile << jsonOutput.dump(4);  // Pretty print with indentation
            outputFile.close();
            cout << "[Info] JSON file saved to " << outputJsonPath << endl;
        }
        // Pack images into texture sheets and handle multiple sheets if needed
        void TexturePacker::packImagesIntoSheets(const std::vector<fs::path>& imagePaths, const fs::path& outputDir, const std::string& Group) {
            int textureIndex = 0;
            int start = 0;

            // Ensure output directory exists
            if (!fs::exists(outputDir)) fs::create_directories(outputDir);

            while (start < imagePaths.size()) {
                vector<ImageData> images;
                vector<stbrp_rect> rects;
                int current_count = 0;

                // Initialize packing context with max possible size
                int sheet_width = settings.MaxTextureSize, sheet_height = settings.MaxTextureSize;
                stbrp_context context;
                vector<stbrp_node> nodes(sheet_width);
                stbrp_init_target(&context, sheet_width, sheet_height, nodes.data(), sheet_width);

                // Try packing images into this sheet
                for (int i = start,spriteIndex = 0; i < imagePaths.size(); ++i,spriteIndex++) {
                    images.push_back(loadImage(imagePaths[i]));

                    stbrp_rect rect;
                    rect.w = images[spriteIndex].width;
                    rect.h = images[spriteIndex].height;
                    rect.id = spriteIndex;

                    if (stbrp_pack_rects(&context, &rect, 1)) {
                        rects.push_back(rect);
                        current_count++;
                    }
                    else break;
                }

                // Determine the actual minimum required texture size based on the packed rectangles
                int required_width = 0, required_height = 0;
                for (const auto& rect : rects) {
                    required_width = std::max(required_width, rect.x + rect.w);
                    required_height = std::max(required_height, rect.y + rect.h);
                }

                // Adjust the size to the nearest power of two
                sheet_width = nextPowerOfTwo(required_width);
                sheet_height = nextPowerOfTwo(required_height);

                // Create blank texture sheet (RGBA)
                std::vector<unsigned char> sheet(sheet_width * sheet_height * STBI_rgb_alpha, 0);

                // Copy packed images into the texture sheet
                for (const auto& rect : rects) {
                    const ImageData& img = images[rect.id];
                    for (int y = 0; y < img.height; ++y) {
                        for (int x = 0; x < img.width; ++x) {
                            int sheet_index = (rect.y + y) * sheet_width + (rect.x + x);
                            int img_index = y * img.width + x;
                            for (int c = 0; c < STBI_rgb_alpha; ++c) {
                                sheet[sheet_index * STBI_rgb_alpha + c] = img.data[img_index * STBI_rgb_alpha + c];
                            }
                        }
                    }
                    cout << "[Info]     adding \"" << img.path << "\"" << endl;
                    stbi_image_free(img.data);  // Free the image data after use
                }

                // Create output file path with postfix
                std::string outputFileName = Group + "_" + std::to_string(textureIndex) + ".png";
                fs::path outputFilePath = outputDir / outputFileName;

                if (!stbi_write_png(
                    outputFilePath.string().c_str(),
                    sheet_width, sheet_height, STBI_rgb_alpha,
                    sheet.data(), sheet_width * STBI_rgb_alpha)) {
                    throw std::runtime_error("Failed to save texture sheet: " + outputFilePath.string());
                }
                cout << "[Info] Texture sheet saved to " << outputFilePath.string() << endl;

                if (settings.useCompression) optimizePngInOutputDir(outputFilePath);
                exportSpriteInfoToJson(rects, images, outputFilePath,settings);

                // Move to the next batch of images
                start += current_count;
                textureIndex++;
            }
        }

        void TexturePacker::checkIfCanAddImage(vector<fs::path>& images,const std::filesystem::directory_entry& entry)
        {
            if (!entry.is_regular_file()) return;

            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (!IsExtensionSupported(ext)) return;
         
            try {
                images.push_back(entry.path());
            }
            catch (const std::exception& e) {
                cerr << "[Error] " << e.what() << endl;
            }
        }
        bool TexturePacker::Pack(PackingSettings settings) {
            this->settings = settings;

            cout << "[Info] Texture Packing Started" << endl;

            std::vector<fs::path> images;

            // Traverse directories and subdirectories
            if (settings.recursive) {
                for (const auto& entry : fs::recursive_directory_iterator(settings.InputDirectory))
                    checkIfCanAddImage(images, entry);
            }
            else {
                for (const auto& entry : fs::directory_iterator(settings.InputDirectory))
                    checkIfCanAddImage(images, entry);
            }

            if (images.empty()) {
                cerr << "[Error] No images found in the directory." << endl;
                return false;
            }

            try {
                packImagesIntoSheets(images, settings.OutputDirectory, settings.Group);
                if(settings.overridePivot) overridePivot();
            }
            catch (const std::exception& e) {
                cerr << "[Error] " << e.what() << endl;
                return false;
            }

            cout << "[Info] Texture Packing Completed" << endl;
            return true;
        }

        void TexturePacker::optimizePngInOutputDir(fs::path spritesheet)
        {
            if (!IsCompressionToolInPath()) {
                std::cerr << "[Error] Could not locate " << compressionTool << ". Did you add it in the environment path?" << std::endl;
                return;
            }

            // get command to call the compression tool based off OS binary
            std::string command = compressionTool + " --force --ext .png --speed 1 --skip-if-larger " + spritesheet.string();

            // Execute the command
            int result = std::system(command.c_str());

            if (result == 0) std::cout << "[Info] PNG optimization complete for " << spritesheet << std::endl;
            else std::cerr << "[Info] Failed to optimize PNG " << spritesheet << std::endl;
        }
        bool TexturePacker::IsCompressionToolInPath() const
        {
#if defined(_WIN32) || defined(_WIN64)
            std::string cmd = "where " + compressionTool + " >nul 2>nul";
#else
            std::string cmd = "which " + compressionTool + " >/dev/null 2>&1";
#endif
            int result = std::system(cmd.c_str());
            return (result == 0);
        }

#pragma endregion
#pragma region Unpacking
        // Function to extract sprites from the texture sheet using data from the JSON file
        void TexturePacker::extractSpritesFromJson(const fs::path& jsonFilePath) {
            // Load the JSON file
            std::ifstream inputFile(jsonFilePath);
            if (!inputFile.is_open()) {
                throw std::runtime_error("Failed to open JSON file: " + jsonFilePath.string());
            }
            nlohmann::json jsonInput;
            inputFile >> jsonInput;
            inputFile.close();

            // Load the texture sheet
            const fs::path& textureSheetPath = jsonFilePath.parent_path() / jsonFilePath.filename().replace_extension(".png");
            int texWidth, texHeight, texChannels;
            unsigned char* textureData = stbi_load(textureSheetPath.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            if (!textureData)
                throw std::runtime_error("Failed to load texture sheet: " + textureSheetPath.string() + ". Check if the image is missing or is corrupt.");

            // Process each sprite from the JSON data
            std::string group = jsonInput["group"];

            // Create the directories if needed
            fs::path outputPath = settings.OutputDirectory / fs::path(group);
            if (!fs::exists(outputPath)) fs::create_directories(outputPath);

            // grab a sprite and export it
            for (const auto& sprite : jsonInput["sprites"]) {
                std::string fileName = sprite["name"];
                std::string extension = sprite["extension"];
                int x = sprite["position"]["x"];
                int y = sprite["position"]["y"];
                int width = sprite["size"]["width"];
                int height = sprite["size"]["height"];

                // Extract the sprite data from the texture
                std::vector<unsigned char> spriteData(width * height * STBI_rgb_alpha);
                for (int row = 0; row < height; ++row) {
                    for (int col = 0; col < width; ++col) {
                        int spriteIndex = (row * width + col) * STBI_rgb_alpha;
                        int texIndex = ((y + row) * texWidth + (x + col)) * STBI_rgb_alpha;

                        for (int c = 0; c < STBI_rgb_alpha; ++c) {
                            spriteData[spriteIndex + c] = textureData[texIndex + c];
                        }
                    }
                }

                outputPath = settings.OutputDirectory / fs::path(group) / (fileName + extension);
                // Export the sprite
                if (extension.find("png") == 1) {
                    if (!stbi_write_png(outputPath.string().c_str(), width, height, STBI_rgb_alpha, spriteData.data(), width * STBI_rgb_alpha)) {
                        throw std::runtime_error("Failed to write sprite: " + outputPath.string());
                    }
                }
                else if (extension.find("j") == 1) {
                    if (!stbi_write_jpg(outputPath.string().c_str(), width, height, STBI_rgb_alpha, spriteData.data(), width * STBI_rgb_alpha)) {
                        throw std::runtime_error("Failed to write sprite: " + outputPath.string());
                    }
                }
                else if (extension.find("tga") == 1) {
                    if (!stbi_write_tga(outputPath.string().c_str(), width, height, STBI_rgb_alpha, spriteData.data())) {
                        throw std::runtime_error("Failed to write sprite: " + outputPath.string());
                    }
                }
                else if (extension.find("bmp") == 1) {
                    if (!stbi_write_bmp(outputPath.string().c_str(), width, height, STBI_rgb_alpha, spriteData.data())) {
                        throw std::runtime_error("Failed to write sprite: " + outputPath.string());
                    }
                }

                cout << "[Info]     Sprite saved to " << outputPath << endl;
            }
            cout << "[Info] Finished exporting Sprites from " << jsonFilePath << endl;

            stbi_image_free(textureData);  // Free the texture sheet data
        }
        void TexturePacker::checkIfCanAddJson(vector<fs::path>& jsons, const std::filesystem::directory_entry& entry)
        {
            if (!entry.is_regular_file()) return;

            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext != ".json") return;

            // check if spritesheet still exists
            fs::path spritesheet = fs::path(entry).remove_filename() / fs::path(entry).filename().replace_extension(".png");
            if (fs::exists(spritesheet)) jsons.push_back(entry);
            else cerr << "[Error] Spritesheet (" << spritesheet.string() << ") is missing but there's a json file. Find the spritesheet or delete " << entry.path() << endl;
        }

        bool TexturePacker::Unpack(PackingSettings settings) {
            this->settings = settings;
            cout << "[Info] Texture Unpacking Started" << endl;
            // TODO: search .json files in directory path
            vector<fs::path> jsons;
            if (settings.recursive) {
                for (const auto& entry : fs::recursive_directory_iterator(settings.InputDirectory))
                    checkIfCanAddJson(jsons, entry);
            }
            else {
                for (const auto& entry : fs::directory_iterator(settings.InputDirectory))
                    checkIfCanAddJson(jsons, entry);
            }
            if (jsons.empty()) {
                cerr << "[Error] No .json found in the " << settings.InputDirectory << endl;
                return false;
            }
            try {
                for (int i = 0; i < jsons.size(); i++)
                    extractSpritesFromJson(jsons[i]);
            }
            catch (std::exception e) {
                cerr << "[Error] " << e.what() << endl;
            }
            cout << "[Info] Texture Unpacking Completed" << endl;
            return true;
        }
#pragma endregion
#pragma region Input
        bool TexturePacker::Run(vector<string> args)
        {
            PackingSettings settings;
            PackingMode mode = PackingMode::None;
            for (const string& arg : args) {
                if (arg.starts_with("-help")) {
                    mode = PackingMode::Error;
                    return true;
                }
                else if (arg == "-p") mode = PackingMode::Pack;
                else if (arg == "-u") mode = PackingMode::Unpack;
                else if (arg.starts_with("-i=")) {
                    string path = arg.substr(3);
                    std::replace(path.begin(), path.end(), '\"', ' ');
                    if (!std::filesystem::is_directory(path)) {
                        cerr << "[Error] Invalid input directory (" << path << "). Unable to proceed" << endl;
                        mode = PackingMode::Error;
                        break;
                    }
                    settings.InputDirectory = path;
                }
                else if (arg.starts_with("-o=")) {
                    string path = arg.substr(3);
                    std::replace(path.begin(), path.end(), '\"', ' ');
                    if (!std::filesystem::is_directory(path)) {
                        cerr << "[Error] Invalid output directory (" << path << "). Unable to proceed" << endl;
                        mode = PackingMode::Error;
                        break;
                    }
                    settings.OutputDirectory = path;
                }
                else if (arg == "-nonrecursive") settings.recursive = false;
                else if (arg == "-compress") settings.useCompression = true;
                else if (arg.starts_with("-size=")) {
                    try {
                        settings.MaxTextureSize = std::stoi(arg.substr(6));
                        if (!settings.IsSizeWithinRange()) {
                            cerr << "[Error] Invalid size. Defaulting to " << DEFAULT_SHEET_SIZE << endl;
                            settings.MaxTextureSize = DEFAULT_SHEET_SIZE;
                        }
                    }
                    catch (std::invalid_argument e) {
                        cerr << "[Error] Invalid input for size. Defaulting to " << DEFAULT_SHEET_SIZE << endl;
                        settings.MaxTextureSize = DEFAULT_SHEET_SIZE;
                    }
                }
                else if (arg.starts_with("-group=")) {
                    settings.Group = arg.substr(7);
                }
                else if (arg.starts_with("-pivot=")) {
                    settings.RulesDirectory = arg.substr(7);
                    if (!std::filesystem::is_directory(settings.RulesDirectory)) {
                        cerr << "[Error] Invalid output directory (" << settings.RulesDirectory << "). Unable to proceed" << endl;
                        mode = PackingMode::Error;
                        break;
                    }
                    settings.overridePivot = true;
                }
                else cerr << "[Error] Unknown argument: " << arg << endl;
            }

            // validate paths
            if (mode != PackingMode::Error) {
                if (settings.InputDirectory.empty()) {
                    cerr << "[Error] Input directory is empty. Do set it using the \"-i=\"). Unable to proceed" << endl;
                    mode = PackingMode::Error;
                }
                if (settings.OutputDirectory.empty()) {
                    cerr << "[Error] Output directory is empty. Do set it using the \"-o=\"). Unable to proceed" << endl;
                    mode = PackingMode::Error;
                }
                if (mode == PackingMode::Pack && isSubdirectory(settings.InputDirectory, settings.OutputDirectory)) {
                    cerr << "[Error] You cannot have the output directory be inside the input directory. ";
                    cerr << "This may recursively pack the output files you generate. ";
                    cerr << "Change the output directory to be outside of the input directory" << endl;
                    mode = PackingMode::Error;
                }
            }

            // process action
            if (mode == PackingMode::Error) return false;
            if (mode == PackingMode::None)
            {
                cerr << "[Error] You forgot to pass \"-p\" or \"-u\" to tell the tool to pack or unpack" << endl;
                return false;
            }

            if (mode == PackingMode::Pack) return Pack(settings);
            if (mode == PackingMode::Unpack) return Unpack(settings);

            cerr << "[Error] Unprocessed action. See code TexturePacker::Run(vector<string>)" << endl;
            return false;
        }
        bool TexturePacker::Run(string commands)
        {
            // slice up the string between spaces while respecting the "paths" in quotes
            std::vector<std::string> args;
            std::stringstream token;
            bool inside_quotes = false;

            for (char ch : commands) {
                if (ch == '"') {
                    inside_quotes = !inside_quotes; // Toggle quote state
                }
                else if (ch == ' ' && !inside_quotes) {
                    if (!token.str().empty()) {
                        args.push_back(token.str()); // Push current token to result
                        token.str(""); // Reset token
                        token.clear();
                    }
                }
                else {
                    token << ch; // Append character to token
                }
            }

            // Add the last token if not empty
            if (!token.str().empty()) {
                args.push_back(token.str());
            }
            return Run(args);
        }
#pragma endregion
#pragma region Set pivot

        void TexturePacker::overridePivot()
        {
            vector<fs::path> jsonRulesPaths;
            string targetGroupRule = settings.Group + ".json";
            for (const auto& entry : fs::recursive_directory_iterator(settings.RulesDirectory))
            {
                if (!entry.is_regular_file()) continue;
                if (entry.path().filename() != targetGroupRule) continue;
                
                jsonRulesPaths.push_back(entry.path());
            }
            if (jsonRulesPaths.empty()) {
                cerr << "[Error] Unable to find any .json for pivot setting"<< endl;
                return;
            }
            for (auto path : jsonRulesPaths)
                setPivot(path);
        }

        void TexturePacker::setPivot(fs::path jsonRulesPath)
        {
            // Load the JSON file
            std::ifstream inputFile(jsonRulesPath);
            if (!inputFile.is_open()) throw std::runtime_error("Failed to open rules file: " + jsonRulesPath.string());
            nlohmann::json jsonRules;
            inputFile >> jsonRules;
            inputFile.close();

            vector<fs::path> spritesheetJsons;
            if (settings.recursive) {
                for (const auto& entry : fs::recursive_directory_iterator(settings.OutputDirectory))
                    checkIfCanAddJson(spritesheetJsons, entry);
            }
            else {
                for (const auto& entry : fs::directory_iterator(settings.OutputDirectory))
                    checkIfCanAddJson(spritesheetJsons, entry);
            }
            // Process each sprite from the JSON data
            for (auto spritesheetJson : spritesheetJsons)
            {
                bool hasChanges = false;
                inputFile = std::ifstream(spritesheetJson);
                if (!inputFile.is_open()) {
                    throw std::runtime_error("Failed to open spritesheet file: " + spritesheetJson.string());
                }
                nlohmann::json spritesheetData;
                inputFile >> spritesheetData;
                inputFile.close();

                if (spritesheetData["group"] != settings.Group) continue;
                for (const auto rule : jsonRules["rules"]) {
                    int spriteIndex = -1;
                    for (const auto& sprite : spritesheetData["sprites"]) {
                        spriteIndex++;
                        if (sprite["name"] != rule["name"]) continue;
                        spritesheetData["sprites"][spriteIndex]["pivot"]["x"] = rule["pivot"]["x"];
                        spritesheetData["sprites"][spriteIndex]["pivot"]["y"] = rule["pivot"]["y"];
                        hasChanges = true;
                    }
                }
                if (!hasChanges) continue;
                std::ofstream updatedSpritesheet(spritesheetJson);
                updatedSpritesheet << spritesheetData.dump(4);  // Pretty print with indentation
                updatedSpritesheet.close();
            }
            cout << "[Info] Finished updating pivots for " << jsonRulesPath << endl;
        }

#pragma endregion

    }
}