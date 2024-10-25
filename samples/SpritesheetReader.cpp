#include "SpritesheetReader.h"
#include "../include/json.hpp"

#include <filesystem>
#include <iostream>
#include <fstream>
namespace QLE {
    namespace TextureTools {
        namespace fs = std::filesystem;
        using std::cout;
        using std::cerr;
        using std::endl;
        std::vector<Spritesheet> SpritesheetReader::ReadFromPath(std::string folderPath)
        {
            std::vector<Spritesheet> sheets;
            std::vector<fs::path> pngsPath;
            std::vector<fs::path> jsonsPath;
            if(!fs::is_directory(folderPath)) {
                cerr << "[Error] Unable to read spritesheets from " << folderPath << " as it is not a valid folder" << endl;  
                return sheets;
            }

            for (const auto& file : fs::recursive_directory_iterator(folderPath))
            {
                if (!file.is_regular_file()) continue;

                if (file.path().extension() != ".png") continue;
                
                fs::path json = file.path().extension().replace_extension(".json");
                if(!fs::exists(json)) continue;

                fs::path png = file.path();
                pngsPath.push_back(json);
                jsonsPath.push_back(json);
            }
            for (int i = 0; i < pngsPath.size(); i++)
                sheets.push_back(ReadFile(pngsPath[i].string(),jsonsPath[i].string()));
            cout << "Found " << sheets.size() << " spritesheets in " << folderPath << endl;
            return sheets;
        }
        Spritesheet SpritesheetReader::ReadFile(std::string filePathPng,std::string filePathJson){
            if(!fs::is_regular_file(filePathPng))
                throw std::runtime_error("Failed to read spritesheet: " + filePathPng);
            
            // Load the JSON file
            std::ifstream inputFile(filePathJson);
            if (!inputFile.is_open()) {
                throw std::runtime_error("Failed to open JSON file when reading the spritesheet: " + filePathJson);
            }
            nlohmann::json jsonInput;
            inputFile >> jsonInput;
            inputFile.close();

            Spritesheet sheet;
            sheet.group = jsonInput["group"];
            sheet.texture = jsonInput["texture"];
            
            for (const auto& sprite : jsonInput["sprites"]) {
                SpriteInfo spriteInfo;
                spriteInfo.name = sprite["name"];
                spriteInfo.extension = sprite["extension"];
                spriteInfo.position.x = sprite["position"]["x"];
                spriteInfo.position.y = sprite["position"]["y"];
                spriteInfo.size.width = sprite["size"]["width"];
                spriteInfo.size.height = sprite["size"]["height"];
                spriteInfo.pivot.x = sprite["pivot"]["x"];
                spriteInfo.pivot.y = sprite["pivot"]["y"];

                // TODO: implement how you're going to load the sprite

                sheet.spriteInfos.push_back(spriteInfo);
            }
            return sheet;
        }
    }
}