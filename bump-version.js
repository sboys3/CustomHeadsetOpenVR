let fs = require("fs")
let path = require("path")

let version = process.argv[2]
if(!version || /^\d+\.\d+\.\d+(\-[a-z0-9\.]+)?$/.test(version) === false){
	console.error("Invalid version format. Please provide a version in the format x.x.x or x.x.x-tag.x")
	process.exit(1)
}else{
	console.log(`Updating version to ${version}`)
}


let tauriConfig = path.join(__dirname, "CustomHeadsetGUI/src-tauri/tauri.conf.json")
let tauriConfigCargo = path.join(__dirname, "CustomHeadsetGUI/src-tauri/Cargo.toml")
let driverConfigCpp = path.join(__dirname, "CustomHeadsetOpenVR/src/Config/Config.cpp")
let driverManifestJson = path.join(__dirname, "CustomHeadsetOpenVR/DriverFiles/driver.vrdrivermanifest")

let tauriConfigData = JSON.parse(fs.readFileSync(tauriConfig, "utf8"))
tauriConfigData.version = version
fs.writeFileSync(tauriConfig, JSON.stringify(tauriConfigData, null, 2))

let tauriConfigCargoData = fs.readFileSync(tauriConfigCargo, "utf8")
let tauriConfigCargoVersionRegex = /^version\s*=\s*"(\d+\.\d+\.\d+(\-[a-z0-9\.]+)?)"$/m
let tauriConfigCargoVersion = tauriConfigCargoData.match(tauriConfigCargoVersionRegex)
if(tauriConfigCargoVersion){
	tauriConfigCargoData = tauriConfigCargoData.replace(tauriConfigCargoVersionRegex, `version = "${version}"`)
	fs.writeFileSync(tauriConfigCargo, tauriConfigCargoData)
}else{
	console.error("Could not find version in Cargo.toml")
}

let driverConfigCppData = fs.readFileSync(driverConfigCpp, "utf8")
let driverConfigCppVersionRegex = /std::string\s*driverVersion\s*=\s*"(\d+\.\d+\.\d+(\-[a-z0-9\.]+)?)"/g
let driverConfigCppVersion = driverConfigCppData.match(driverConfigCppVersionRegex)
if(driverConfigCppVersion){
	driverConfigCppData = driverConfigCppData.replace(driverConfigCppVersionRegex, `std::string driverVersion = "${version}"`)
	fs.writeFileSync(driverConfigCpp, driverConfigCppData)
}else{
	console.error("Could not find version in Config.cpp")
}

let driverManifestJsonData = JSON.parse(fs.readFileSync(driverManifestJson, "utf8"))
driverManifestJsonData.version = version
fs.writeFileSync(driverManifestJson, JSON.stringify(driverManifestJsonData, null, 2).replaceAll("  ", "\t"))