# CustomHeadsetOpenVR
This is a custom headset driver for SteamVR that allows the MeganeX 8K and Dream Air to operate as native SteamVR headsets. It provides significant image customization to all native SteamVR headsets through the custom shader.  


<picture><img src="./CustomHeadsetGUI/public/CustomHeadsetCropped.png" height="96"><img/></picture>

## Installing
1. Download the latest release from the [releases page](https://github.com/sboys3/CustomHeadsetOpenVR/releases/latest)
2. Extract the whole folder within the zip to a place you want it.
3. Run `custom-headset-gui.exe` in `CustomHeadsetGUI` to finish the installation and configure settings.
![Installation Tutorial](Docs/Media/CustomHeadsetInstall.webp)
<details>
<summary>Manual Driver Installation</summary>
You do not need to do this if you installed the driver with the GUI.

1. Copy the `CustomHeadsetOpenVR` folder from inside the zip into `C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers` 
2. Disable the MeganeXSuperlight driver in SteamVR
3. Restart SteamVR
</details>

## Updating
1. Download the latest release from the [releases page](https://github.com/sboys3/CustomHeadsetOpenVR/releases/latest)
2. Extract the whole folder within the zip to a place you want it.
3. Run `custom-headset-gui.exe` in `CustomHeadsetGUI`
4. Go to the about page and install the new driver.

## Configuring
Run `custom-headset-gui.exe` to configure settings.
Almost all settings will be immediately applied when the file is saved without restarting SteamVR.  


## Donations and Affiliates

If you find this project helpful, consider supporting it through donations or using the affiliate links below:

### Donation Links
If my project improved your VR experience, consider donating.  
[<img src="./CustomHeadsetGUI/public/patreon-logo.svg" height="24" alt="Patreon"> Patreon](https://patreon.com/SBoys3)  
[<img src="./CustomHeadsetGUI/public/ko-fi-logo.svg" height="24" alt="Ko-fi"> Ko-fi](https://ko-fi.com/sboys3)  

### Affiliate Links
If my driver is a major factor in your decision of purchasing the Dream Air, use my affiliate link at no cost to you. You can use the affiliate links for other Pimax headsets to support me as well.  
[<img src="./CustomHeadsetGUI/public/DreamAirHD.png" height="24" alt="Pimax"> Pimax (25SBOY3 for $25 off)](https://pimax.com/products/pimax-dream-air/?discount=25SBOY3&ref=sboys3)  

## Building
1. Clone the repository wit `git clone https://github.com/sboys3/CustomHeadsetOpenVR.git`
2. Navigate to the repository folder with `cd CustomHeadsetOpenVR` in the terminal.
3. Pull the submodules with `git submodule update --init --recursive` then `git pull --recurse-submodules`
4. Open the solution in Visual Studio.
5. Change the configuration to Release.
6. Build the solution. It will create the driver folder in `output` and copy it to the SteamVR drivers folder.

To pull the latest changes, run `git pull --recurse-submodules`

### Building on Linux
1. Follow the cloning steps above
2. `cd CustomHeadsetOpenVR/CustomHeadsetOpenVR`
3. `mkdir build && cd build`
4. `cmake .. -DCMAKE_BUILD_TYPE=Release`
5. `make -j16`
6. `rsync -a --delete CustomHeadsetOpenVR/ ~/.steam/steam/steamapps/common/SteamVR/drivers/CustomHeadsetOpenVR/`

### Manual Configuration
Enter `%APPDATA%/CustomHeadset` into the file browser top bar to get to the settings folder.  
Edit the `settings.json` file based on the [Config header file](./CustomHeadsetOpenVR/src/Config/Config.h)  
Distortion profiles go in a folder named `Distortion` and they are referenced by their name.  

### Closed Source Components
The DRM related functionality on the Dream Air is not open source, but the released driver can be used with a compiled from source driver.

The `onlyHandlePrivateFunctionality` property allows you to run a compiled source driver alongside the closed-source driver. When set to `true` in the root of the config, it disables all functionality in the driver that contains private code except for the private code itself. This allows a compiled source driver to handle all the important parts of the driver. A driver without the closed source components is unaffected by the option.

To use this feature:

1. Build the driver from source following the steps above
2. Rename the released driver folder, the DLL, and the name in `driver.vrdrivermanifest` to something else (e.g., `CustomHeadsetOpenVR_Private`)
3. Set `onlyHandlePrivateFunctionality: true` in the root of your `settings.json` config file
4. The closed-source driver will handle private functionality while your source-built driver handles everything else


## Features
- MeganeX and Dream Air
	- [x] Running as a native headset
	- [x] Ignore other headsets
	- [x] Code to generate the distortion mesh
	- [X] Darken the display when no motion is detected
      * ⚠️Be careful with high brightnesses to avoid burn in on the MeganeX. This driver does not automatically dim the headset in bright scenes which may permanently dim the panel if viewed for many hours.
	- [X] Support for non square outputs to light edges of the display
	- [X] A good default distortion profile
	- [X] A near perfect default distortion profile
	- [X] Multiple distortion profiles
	- [X] Hidden area mesh
- Custom Shader
	- [X] Support all native headsets (MeganeX, Dream Air, BSB 1&2, Index, Vive, etc)
	- [X] Replace SteamVR compositor shaders at runtime
	- [X] Configure contrast, chroma, and gamma
	- [X] Subpixel shifts for the MeganeX and Vive
	- [X] sRGB color correction for the MeganeX
	- [X] Remove mura correction or black clamp for any native headset
- Config
	- [X] Define the structure of the config file
	- [X] Create MeganeX section
	- [X] Read config file
	- [X] Hot reload when the config is changed
	- [X] Load custom distortion profiles
	- [ ] Define the structure for the overrides section
- GUI
	- [X] Create a Tauri project
	- [X] Create a Angular project within it
	- [X] Configure MeganeX settings
	- [ ] Edit distortion profiles my dragging points on a curve
	- [ ] Create overrides for any headset
	- [X] Install the driver
	- [ ] Change physical MeganeX settings
- Driver
	- [X] Linux build support
	- [ ] Override any property of any SteamVR device based on the config
	- [ ] Change device types
	- [ ] Apply presets to devices ex: Vive tracker or generic headset
	- [X] Output json file with information about SteamVR and devices for configuration utilities to use.


## License
This project is licensed under the GPLv2 license. See the LICENSE file for more details.
I may relicense this project for to allow for use that GPLv2 does not allow for so be aware of that when you contribute

## Contributing
By committing to this repository you give me the right to relicense your code for others to use. If you want to make a significant contribution but do not want to comply with this, mark your code as such and let me know so I will be able to remove it if I need to.

Try to keep the coding style consistent with existing files. But other than keeping tabbing consistent, I will not enforce strict guidelines.


## Supporters
Thanks to everyone who has shown appreciation to me for this project. It has been an amazing experience having so many people thank me for my work. I really appreciate the kind words you all had for me and I have a special appreciation to those who went a step further and donated.

<picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="80" height="80"><img/></picture> [<picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/df8e8bbdb98cda795b9602697ea50542.jpg" width="64" height="64"><img/></picture>]( https://t.me/realtavirtualevr "") [<picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default.png" width="64" height="64"><img/></picture>](# "GoodyearBiff") [<picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/773d6805d3aa80d1316a26ec1b364379.jpg" width="64" height="64"><img/></picture>](https://github.com/gregtakacs "Greg Takacs") <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="64" height="64"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="64" height="64"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="64" height="64"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="64" height="64"><img/></picture> [<picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/7ac08ea782441bede3916424fc32bc0b.jpg" width="64" height="64"><img/></picture>](https://www.youtube.com/@Essentia-Channel "Essentia") [<picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/3e9ac621e70bbe27c69d79f346c30491.jpg" width="64" height="64"><img/></picture>](https://www.youtube.com/c/VRFlightSimGuy "VR Flight Sim Guy") <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="64" height="64"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="64" height="64"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="64" height="64"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="64" height="64"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> [<picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/f2abff6a17e0ff18d125e7ba7b003fb2.png" width="48" height="48"><img/></picture>](https://hookmanuk.itch.io/ "hookman") <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> [<picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/7ddc5c7cc7adf41aff1285ac1cd10ba0.jpg" width="48" height="48"><img/></picture>](# "Richard Åsberg (WebMaximus)") <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> [<picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/04c6935fd569b309d57c3c0a51bea463.jpg" width="32" height="32"><img/></picture>](https://www.artstation.com/cless "") <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> [<picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/cbcb907081b8d18f7bcc2755ef8f72ef.jpg" width="32" height="32"><img/></picture>](https://github.com/hsinyu-chen "hsin yu,chen") [<picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default.png" width="32" height="32"><img/></picture>](# "Nick Babalis") [<picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/faec663d3c4ea4ce4341e621eaeab573.jpg" width="32" height="32"><img/></picture>](https://www.youtube.com/@MartydudeVR "MartydudeVR") <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> [<picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default.png" width="32" height="32"><img/></picture>](# "SparkerInVR") <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> 

If you want your info displayed, send me a message on the platform you donated on. You can tell me where to get an icon, a name to display when hovered, and a link to wherever you want. You can provide any combination of the 3 and I will add it. Alternatively you can remain autonomous as a generic icon. I can always change it for you in the future, but information will stay in github history forever.
