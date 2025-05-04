# CustomHeadsetOpenVR
This is a custom headset driver for SteamVR that allows the MeganeX Superlight 8K to operate as a native SteamVR headset. In the future this will provide tools to modify any SteamVR device.

## Building
1. Clone the repository wit `git clone https://github.com/sboys3/CustomHeadsetOpenVR.git`
2. Pull the submodules with `git submodule update --init --recursive`
3. Open the solution in Visual Studio.
4. Change the configuration to Release.
5. Build the solution. It will create the driver folder in `output` and copy it to the SteamVR drivers folder.

## Features
- MeganeX
	- [x] Running as a native headset
	- [x] Ignore other headsets
	- [x] Code to render generate the distortion mesh
	- [ ] Support for non square outputs to light edges of the display
	- [ ] A near perfect default distortion profile
	- [ ] Multiple distortion profiles
	- [ ] Hidden area mesh
- Config
	- [ ] Define the structure of the config file
	- [ ] Create MeganeX section
	- [ ] Read config file
	- [ ] Hot reload when the config is changed
	- [ ] Load custom distortion profiles
	- [ ] Define the structure for the overrides section
- GUI
	- [ ] Create a Tauri project
	- [ ] Create a Vue project within it
	- [ ] Configure MeganeX settings
	- [ ] Edit distortion profiles my dragging points on a curve
	- [ ] Create overrides for any headset
- Driver
	- [ ] Linux build support
	- [ ] Override any property of any SteamVR device based on the config
	- [ ] Change device types
	- [ ] Apply presets to devices ex: Vive tracker or generic headset


## Supporters
Thanks to everyone who has shown appreciation to me for this project. It has been an amazing experience having so many people thank me for my work. I really appreciate the kind words you all had for me and I have a special appreciation to those who went a step further and donated.

<picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="80" height="80"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="64" height="64"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="64" height="64"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="64" height="64"><img/></picture> [<picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/3e9ac621e70bbe27c69d79f346c30491.jpg" width="64" height="64"><img/></picture>](https://www.youtube.com/c/VRFlightSimGuy "VR Flight Sim Guy") <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> [<picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/f2abff6a17e0ff18d125e7ba7b003fb2.png" width="48" height="48"><img/></picture>](https://hookmanuk.itch.io/ "hookman") <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="48" height="48"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> [<picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/cbcb907081b8d18f7bcc2755ef8f72ef.jpg" width="32" height="32"><img/></picture>](https://github.com/hsinyu-chen "hsin yu,chen") [<picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default.png" width="32" height="32"><img/></picture>](# "Nick Babalis") [<picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/faec663d3c4ea4ce4341e621eaeab573.jpg" width="32" height="32"><img/></picture>](https://www.youtube.com/@MartydudeVR "MartydudeVR") <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture> <picture><img src="https://raw.githubusercontent.com/sboys3/supporters/main/icons/default-anon.png" width="32" height="32"><img/></picture>  

If you want your info displayed, send me a message on the platform you donated on. You can tell me where to get an icon, a name to display when hovered, and a link to wherever you want. You can provide any combination of the 3 and I will add it. Alternatively you can remain autonomous as a generic icon. I can always change it for you in the future, but it will stay in github history.
