#include "TexturePacker.h"
#include <string>
#include <vector>
using std::string;
using std::vector;

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    vector<string> args(argv + 1, argv + argc);

    // create the object to initialize its default value
    QLE::TextureTools::TexturePacker packer;

    // returns 0 if program was successful.
    //         1 if program failed.
    return packer.Run(args) ? 0 : 1;
}