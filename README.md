<div style="text-align:center">
  <img src="media/Texture Packer logo.jpg" alt="Texture Packer logo" width="256" height="256"/>
  <h1>Texture Packer</h1>
  <caption>by<br/>Quality Level Entertainment</caption>
</div>

## Summary
A C++20 tool meant to pack textures into spritesheets, extract sprites from spritesheets, and set pivots to your sprites.

Supported image types are `.png`,`.jpg`,`.jpeg`,`.jfif`, `.tga`, and `.bmp`.

## Features
1. Packs images into spritesheets and exports `.png` and `.json`.
2. Unpacking spritesheets and exports back to their original extension.
3. Ready to use within your coding projects or in your Command Line Interface.
4. Supports spaces within paths.
5. `.png` compression with [pngquant](https://github.com/kornelski/pngquant).
6. Free.

## Usage

### Overview
1. Download the TexturePacker binary or compile it yourself.
2. Create an automation script to call the TexturePacker along with the arguments custom to your project.
3. Whenever you need to update your spritesheet, just run your automation script to pass commands.
4. If you want to update the pivot points of your sprites, just make the pivot `.json` file. See **Setting the Pivot** section below

### Example usage
In your terminal / command prompt:

```console
TexturePacker.exe -p -i="C:\input_folder" -o="C:\output_folder"
TexturePacker.exe -p -i="C:\input_folder" -o="textures\output_folder" -s=2048 -group="general" -nonrecursive -compress
TexturePacker.exe -u -i="textures\output_folder" -o="C:\input_folder"
TexturePacker.exe -help
```

In your code:

```cpp
#include "TexturePacker.h"
using namespace QLE::TextureTools;

// then, in a function

// change the value as needed
string commands = "-p -i=\"C:\\input_folder\" -o=\"C:\\output_folder\"";
TexturePacker packer;
if(packer.Run(commands)){
    cout << "All good" << endl;
}
```

## Arguments

### Processing Modes

Pick one at a time. Passing all these options will only make the last argument succeed.
```
-p      | enable packing mode. reads images and exports .png and .json
-u      | enable unpacking mode. reads .json files and exports individual textures
```

### Required

Directories can be relative paths. Just make sure to call it relative to where you're calling the script to call the TexturePacker tool
```
-i=<input_directory>        | folder path containing the images you want to pack/unpack
-o=<output_directory>       | folder path where you want the exported files placed
```
### Optional
```
-compress                   | Compresses the spritesheet after packing using "pngquant"
-nonrecursive               | Makes the packing/unpacking non-recursive
-size=<spritesheet_size>    | Defaults to 2048. Valid sizes - 16 up to 4096
-group=<spritesheet_name>   | Defaults to "general". Used to group sprites together
-pivot=<pivot_directory>    | folder path containing a single or multiple .json files which sets a custom pivot for a given sprite
```
### Extra
```
-help = displays the help menu
```

## Packing information

When you pack your images, you create two files which are `.png` and `.json`.
1. `.png` - contains your combined textures. 
2. `.json` - contains information about your sprites found within the atlas/sheet.

Both file names are generated using the group and the spritesheet index.
Example: `Vegetable40.png`, `Vegetable40.json`

The `.json` would have the following structure:

```json
{
  "texture": "fruit_0.png",
  "group": "fruit",
  "sprites": [
    {
      "name": "apple",
      "extension": ".png",
      "position": { "x": 0, "y": 0 },
      "size": { "width": 100, "height": 100 },
      "pivot": { "width": 0.5, "height": 0.5 }
    },
    {
      // same as above
    },
  ]
}
```
This `.json` is meant to be read by your custom tool or game engine which will be used to fetch your individual sprites from the spritesheet.

For convenience, you can use [Spritesheet](sample/Spritesheet.h) and [Spritesheet Reader](sample/SpritesheetReader.h) classes when you're parsing from your tool / engine. The implementation of how you're going to read the data from the spritesheet depends on the tool you're working in or your engine.

### Algorithm

A valid texture is considered with the following conditions:
1. smaller or equal to (<=) max texture size
2. existing file
3. image type is supported by the tool

For packing:
1. Fetch valid textures.
2. Read textures.
3. Combine texture data into a single spritesheet from left to right until max texture size has been reached.
4. If image still needs more space, expand downwards until max texture size has been reached.
5. If the image still needs more space, create a new texture sheet and put the image in there.
6. Repeat for all images.

## Setting the Pivot

You can override the pivot points for the individual sprites you generate with this tool.
By default, all pivot points are set to `0.5` for both the x and y axis.

For you to be able to override the pivot points, you need to do two things which are:

1. Make a `general.json` file and have the following structure:
```json
{
    "rules":[
        {
            "name":"apple",
            "pivot": { "x": 0.5, "y": 0.25 }
        },
        // add here
    ]
}
```
2. Pass the rules folder when calling the tool using `-pivot=<pivot_directory>`

### Notes

1. The `.json`'s file name must match the group name you set when calling the tool. By default, group's value is `general`.
1. `name` must match exactly the same (case-sensitive) as the original file name without extension.
1. You can place the `.json` anywhere inside the rule_directory. Feel free to organize the rules into their own folders.
1. If you did not set the pivot for a sprite, the sprite will continue to retain its pivot to be `0.5` for both xy axis.

## Libraries used

1. [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h)
1. [stb_image_write.h](https://github.com/nothings/stb/blob/master/stb_image_write.h)
1. [stb_rect_pack.h](https://github.com/nothings/stb/blob/master/stb_rect_pack.h)
1. [nholmann/json.hpp](https://github.com/nlohmann/json/blob/develop/single_include/nlohmann/json.hpp)

## Compression

- You'll need to download the pngquant binary from [pngquant.org](https://pngquant.org/) if you want to compress your images after packing the spritesheet. In case their website is down, this is their [Github Repo](https://github.com/kornelski/pngquant).
- Just know that once you compressed your images, extracting the sprites from the spritesheet would have a slightly different color due to compression.
- Be sure to add this in your environment path so the tool can execute a system call to the tool.
- You'll need to add the `-compress` if you want to compress the spritesheets upon export.

## Frequently Asked Questions

- I want to create spritesheets per folder. How can I do that?
  - You can call the tool for each folder you want and make sure to add a **unique group name** using the (`-group=`) argument. Otherwise, you'll be overwriting your json files in your output folder.
  - Here is an example batch script that you can modify to your liking:
  
  File - `ExportSpritesheet.bat`
  ```console
  TexturePacker.exe -p -i="FolderA" -o="OutputFolder" -compress -group="GroupA"
  TexturePacker.exe -p -i="FolderB" -o="OutputFolder" -compress -group="GroupB"
  ```
  In the code, you can just call the `TexturePacker::Run(string commands)` multiple times with the same arguments as above.

- Can I contribute to the project?
  - Sure. You can submit a pull request. I'll check it when I can.
- I want to increase the sheet size from `4096`. How can I do that?
  - You can download the code, add another power of two value in `TexturePacker.h`'s `PackingSettings`. Then, compile it for yourself.
  - By design, `4096` is the max texture size I've set since most spritesheets are capped around `2048`. If you still need a bigger spritesheet, you may want to reconsider and down-size your textures or separate them accordingly. Not all devices can handle a giant image.
- I want to use a different image compression tool. How can I do it?
  - You can modify the `COMPRESSION_TOOL` value which is set inside `TexturePacker.cpp`.
- Will the images I enter into the tool remain the same after I extract it from my newly made spritesheet?
  - Technically, no. Once the input files are packed, they are now modified with `stb_image_write_.h` code. They will be different in file size once you export it out from the spritesheet.
  - Aesthetically, yes. They look the same.
- Can you add this custom image file type?
  - If `stb_image.h` supports it, then yes. I can add it.
  - Otherwise, no.
    - You'll either have to convert your image using [ImageMagick](https://github.com/ImageMagick/ImageMagick) or similar safe converters OR
    - adjust the code yourself.
- Can I request X feature?
  - If it's not a big hassle for me, then probably yes. I'm busy on most days inventing tools that I need or will be needing.

## Disclaimer

This is a simple texture packer and is not meant to compete with other premium or proprietary texture packers.
Its sole existence is because its hard to pack images within existing game engines and be used in a CLI environment.
One of which has one but doesn't expose the button to let us pack it when we need one. Thus, this Texture Packer was compiled.

---
made with 🍹