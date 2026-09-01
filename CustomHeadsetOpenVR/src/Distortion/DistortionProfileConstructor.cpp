#include "DistortionProfileConstructor.h"
#include "RadialBezierDistortionProfile.h"
#ifdef PVR_EXISTS
#include "PimaxDistortionProfile.h"
#endif
#include <cmath>
#include <map>

std::unordered_map<std::string, DistortionProfileConfig> builtInDistortionProfiles = {};
// map old names to the new name of distortion profiles for comatability with old configs
std::unordered_map<std::string, std::string> builtInDistortionProfileAliases = {
	{"MeganeX8K Original", "MeganeX8K v0.3.0"}
};


bool PopulateBuiltInDistortionProfiles(){
	DistortionProfileConfig defaultProfile = {};
	defaultProfile.name = "MeganeX8K Default";
	defaultProfile.device = {"MeganeX8K"};
	defaultProfile.modifiedTime = 0;
	defaultProfile.description = "Default distortion profile for MeganeX8K. This is the profile made by Essentia that was called the Essentia Zoom Out profile. Previous profiles are too zoomed in when the diopters are set to zero, so this one provides more FOV. It works best if you can get close enough to the lenses.";
	defaultProfile.author = "Essentia, SBoys3, Shiftall";
	defaultProfile.creationDate = 1755911419.135;
	defaultProfile.type = "RadialBezier";
	defaultProfile.distortions = {
		 0.0000, 0.0,
		 5.0000, 12.024,
		10.0000, 23.904,
		15.0000, 35.429,
		20.0000, 46.511,
		25.0000, 57.162,
		30.0000, 67.498,
		35.0000, 77.596,
		40.0000, 86.882,
		45.0000, 94.532,
		48.3073, 98.599,
		50.0000, 100.265
	};
	builtInDistortionProfiles[defaultProfile.name] = defaultProfile;
	
	
	DistortionProfileConfig meganeX8Kv1_0_0 = {};
	meganeX8Kv1_0_0.name = "MeganeX8K v1.0.0";
	meganeX8Kv1_0_0.device = {"MeganeX8K"};
	meganeX8Kv1_0_0.modifiedTime = 0;
	meganeX8Kv1_0_0.description = "This is the profile made by Essentia that was called the Western B CA profile. It works pretty well if you can get close enough to be able to nearly see the 110 degree field of view.";
	meganeX8Kv1_0_0.author = "Essentia, SBoys3, Shiftall";
	meganeX8Kv1_0_0.creationDate = 1748554999.135;
	meganeX8Kv1_0_0.type = "RadialBezier";
	meganeX8Kv1_0_0.distortions = {
		0.00000, 0.0,
		5.00000, 12.463,
		10.0000, 24.750,
		15.00000, 36.665,
		20.00000, 48.103,
		25.00000, 59.093,
		30.00000, 69.749,
		35.00000, 79.994,
		40.00000, 89.147,
		45.00000, 96.357,
		48.30730, 100.0,
	};
	builtInDistortionProfiles[meganeX8Kv1_0_0.name] = meganeX8Kv1_0_0;
	
	
	DistortionProfileConfig meganeX8Kv0_4_0 = {};
	meganeX8Kv0_4_0.name = "MeganeX8K v0.4.0";
	meganeX8Kv0_4_0.device = {"MeganeX8K"};
	meganeX8Kv0_4_0.modifiedTime = 0;
	meganeX8Kv0_4_0.description = "MeganeX8K distortion profile from CustomHeadset v0.4.0";
	meganeX8Kv0_4_0.author = "SBoys3, Shiftall";
	meganeX8Kv0_4_0.type = "RadialBezier";
	meganeX8Kv0_4_0.distortions = {
		0.00000, 0.0,
		10.0000, 24.7,
		20.0000, 48.0,
		30.0000, 69.6,
		35.0000, 79.9,
		40.0000, 89.06,
		45.0000, 96.30,
		48.3073, 100.0,
	};
	builtInDistortionProfiles[meganeX8Kv0_4_0.name] = meganeX8Kv0_4_0;
	
	
	DistortionProfileConfig meganeX8Kv0_3_0 = {};
	meganeX8Kv0_3_0.name = "MeganeX8K v0.3.0";
	meganeX8Kv0_3_0.device = {"MeganeX8K"};
	meganeX8Kv0_3_0.modifiedTime = 0;
	meganeX8Kv0_3_0.description = "MeganeX8K original distortion profile which is the same as simplehmd.";
	meganeX8Kv0_3_0.author = "Shiftall";
	meganeX8Kv0_3_0.type = "RadialBezier";
	meganeX8Kv0_3_0.distortions = {
		00.0000, 0.0,
		10.0000, 24.77952472,
		20.0000, 48.32328161,
		30.0000, 69.9136628,
		35.0000, 79.99462488,
		40.0000, 89.06057112,
		45.0000, 96.29634484,
		48.3073, 100.0,
	};
	meganeX8Kv0_3_0.distortionsRed = {
		0., 0.5,
		47.5, 0.5,
	};
	meganeX8Kv0_3_0.distortionsBlue = {
		0., -0.42,
		47.5, -0.42,
	};
	builtInDistortionProfiles[meganeX8Kv0_3_0.name] = meganeX8Kv0_3_0;
	
	
	DistortionProfileConfig dreamAir = {};
	dreamAir.name = "Dream Air Default";
	dreamAir.device = {"DreamAir", "CrystalSuperMicroOLED"};
	dreamAir.modifiedTime = 0;
	dreamAir.description = "Default distortion profile for the Dream Air in this driver. It is a custom distortion profile created for for the Dream Air by using a calibrated camera.";
	dreamAir.author = "SBoys3";
	dreamAir.creationDate = 1771922353.519;
	dreamAir.type = "RadialBezier";
	dreamAir.distortions = {
		0.00, 0.000,
		5.00, 12.80,
		10.0, 25.50,
		15.0, 38.00,
		20.0, 50.23,
		25.0, 62.05,
		30.0, 73.35,
		35.0, 84.10,
		40.0, 94.20,
		45.0, 103.25,
		50.0, 111.1,
		52.5, 114.3,
		55.0, 117.3,
	};
	dreamAir.distortionsRed = {
		0.00, 0.30,
		25.0, 0.45,
		40.0, 0.50,
		50.0, 0.55,
	};
	dreamAir.distortionsBlue = {
		0.00, -0.2,
		25.0, -0.4,
		30.0, -0.5,
		40.0, -0.7,
		50.0, -0.7,
	};
	builtInDistortionProfiles[dreamAir.name] = dreamAir;
	
	DistortionProfileConfig dreamAirLikePimax = {};
	dreamAirLikePimax.name = "Dream Air like Pimax";
	dreamAirLikePimax.device = {"DreamAir"};
	dreamAirLikePimax.modifiedTime = 0;
	dreamAirLikePimax.description = "Distortion profile for the Dream Air that combines the lens geometry of the Pimax native profile with the chromatic aberration calibration of the camera calibrated Dream Air Default. The geometry was fitted from a capture of the distortion mesh rendered by the Pimax compositor, including the optical center sitting 2.7% towards the nose and 0.9% above the panel center, and follows the capture within 0.1 degrees. The chromatic aberration correction reproduces the Dream Air Default at every panel radius. Set Max FOV to 98.0x87.0 to render exactly the field the native profile shows.";
	dreamAirLikePimax.author = "Pimax, Yar, SBoys3";
	dreamAirLikePimax.creationDate = 1786665600.0;
	dreamAirLikePimax.type = "RadialBezier";
	dreamAirLikePimax.offsetX = -2.71f;
	dreamAirLikePimax.offsetY = 0.9f;
	// dreamAirLikePimax.recommendedMaxFovX = 98.0;
	// dreamAirLikePimax.recommendedMaxFovY = 87.0;
	dreamAirLikePimax.distortions = {
		0.00, 0.000,
		2.50, 6.420,
		5.00, 12.869,
		10.0, 25.641,
		15.0, 38.185,
		20.0, 50.377,
		25.0, 62.062,
		30.0, 73.145,
		35.0, 83.496,
		40.0, 93.232,
		45.0, 102.044,
		47.5, 105.735,
		50.0, 108.985,
		52.5, 111.722,
		55.0, 114.836
	};
	dreamAirLikePimax.distortionsRed = {
		0.00, 0.30,
		5.00, 0.33,
		10.0, 0.37,
		15.0, 0.40,
		20.0, 0.42,
		25.0, 0.44,
		30.0, 0.46,
		35.0, 0.46,
		40.0, 0.48,
		45.0, 0.48,
		50.0, 0.48,
		55.0, 0.60
	};
	dreamAirLikePimax.distortionsBlue = {
		0.00, -0.20,
		5.00, -0.24,
		10.0, -0.28,
		15.0, -0.32,
		20.0, -0.36,
		25.0, -0.39,
		30.0, -0.48,
		35.0, -0.59,
		40.0, -0.67,
		45.0, -0.68,
		50.0, -0.68,
		55.0, -0.71
	};
	builtInDistortionProfiles[dreamAirLikePimax.name] = dreamAirLikePimax;
	
	DistortionProfileConfig dreamAirSE = {};
	dreamAirSE.name = "Dream Air SE Default";
	dreamAirSE.device = {"DreamAirSE"};
	dreamAirSE.modifiedTime = 0;
	dreamAirSE.description = "Default distortion profile for the Dream Air SE in this driver. It is a custom distortion profile created for for the Dream Air SE by using a calibrated camera.";
	dreamAirSE.author = "SBoys3";
	dreamAirSE.creationDate = 1783740598.037;
	dreamAirSE.type = "RadialBezier";
	dreamAirSE.offsetX = -3.0;
	dreamAirSE.eyeRotationOffset = -1.0;
	dreamAirSE.distortions = {
		0.00, 0.000,
		5.00, 12.40,
		10.0, 24.70,
		15.0, 36.85,
		20.0, 48.70,
		25.0, 60.05,
		30.0, 70.90,
		35.0, 81.20,
		40.0, 90.80,
		45.0, 99.60,
		50.0, 107.7,
		55.0, 115.5
	};
	dreamAirSE.distortionsRed = {
		0.00, 0.30,
		25.0, 0.45,
		40.0, 0.50,
		50.0, 0.55,
	};
	dreamAirSE.distortionsBlue = {
		0.00, -0.2,
		25.0, -0.4,
		30.0, -0.4,
		40.0, -0.5,
		50.0, -0.6
	};
	builtInDistortionProfiles[dreamAirSE.name] = dreamAirSE;
	
	DistortionProfileConfig dreamAirSELikePimax = {};
	dreamAirSELikePimax.name = "Dream Air SE like Pimax";
	dreamAirSELikePimax.device = {"DreamAirSE"};
	dreamAirSELikePimax.modifiedTime = 0;
	dreamAirSELikePimax.description = "Distortion profile for the Dream Air SE that combines the lens geometry of the Pimax native profile with the chromatic aberration calibration of the camera calibrated Dream Air SE Default. The geometry aims to closely reproduce the shape of the native Pimax rendering, including the optical center sitting 5.5% towards the nose and 2.2% above the panel center, and follows it within 0.05 degrees. The chromatic aberration correction reproduces the Dream Air SE Default at every panel radius. Set Max FOV to 89.4x89.5 to render exactly the field the native profile shows.";
	dreamAirSELikePimax.author = "Pimax, Yar, SBoys3";
	dreamAirSELikePimax.creationDate = 1787961600.0;
	dreamAirSELikePimax.type = "RadialBezier";
	dreamAirSELikePimax.offsetX = -5.48f;
	dreamAirSELikePimax.offsetY = 2.19f;
	dreamAirSELikePimax.eyeRotationOffset = -2.0;
	// dreamAirSELikePimax.recommendedMaxFovX = 89.4;
	// dreamAirSELikePimax.recommendedMaxFovY = 89.5;
	dreamAirSELikePimax.distortions = {
		0.00, 0.000,
		2.50, 6.178,
		5.00, 12.363,
		10.0, 24.677,
		15.0, 36.788,
		20.0, 48.618,
		25.0, 60.034,
		30.0, 70.936,
		35.0, 81.225,
		40.0, 90.919,
		45.0, 99.923,
		47.5, 104.386,
		50.0, 109.073,
		52.5, 113.880,
		55.0, 118.993
	};
	dreamAirSELikePimax.distortionsRed = {
		0.00, 0.30,
		25.0, 0.45,
		40.0, 0.50,
		50.0, 0.55,
	};
	dreamAirSELikePimax.distortionsBlue = {
		0.00, -0.2,
		25.0, -0.4,
		30.0, -0.4,
		40.0, -0.5,
		50.0, -0.6
	};
	builtInDistortionProfiles[dreamAirSELikePimax.name] = dreamAirSELikePimax;
	
	DistortionProfileConfig crystalSuperOled = {};
	crystalSuperOled.name = "Crystal Super Micro-OLED Default";
	crystalSuperOled.device = {"DreamAir", "CrystalSuperMicroOLED"};
	crystalSuperOled.modifiedTime = 0;
	crystalSuperOled.description = "Default distortion profile for the Crystal Super OLED in this driver. It is a custom distortion profile created for the Crystal Super OLED by using a calibrated camera.";
	crystalSuperOled.author = "SBoys3";
	crystalSuperOled.creationDate = 1783921954.447;
	crystalSuperOled.type = "RadialBezier";
	crystalSuperOled.offsetX = -3.0;
	crystalSuperOled.eyeRotationOffset = -1.0;
	crystalSuperOled.distortions = {
		0.00, 0.000,
		5.00, 12.80,
		10.0, 25.50,
		15.0, 38.00,
		20.0, 50.23,
		25.0, 62.05,
		30.0, 73.35,
		35.0, 84.10,
		40.0, 94.20,
		45.0, 103.25,
		50.0, 111.1,
		52.5, 114.3,
		55.0, 117.3
	};
	crystalSuperOled.distortionsRed = {
		0.00, 0.30,
		25.0, 0.45,
		40.0, 0.50,
		50.0, 0.55,
	};
	crystalSuperOled.distortionsBlue = {
		0.00, -0.2,
		25.0, -0.4,
		30.0, -0.5,
		40.0, -0.7,
		50.0, -0.7
	};
	builtInDistortionProfiles[crystalSuperOled.name] = crystalSuperOled;
	
	DistortionProfileConfig crystalSuperOledLikePimax = {};
	crystalSuperOledLikePimax.name = "Crystal Super micro-OLED like Pimax";
	crystalSuperOledLikePimax.device = {"CrystalSuperMicroOLED"};
	crystalSuperOledLikePimax.modifiedTime = 0;
	crystalSuperOledLikePimax.description = "Distortion profile for the Crystal Super micro-OLED that combines the lens geometry of the Pimax native profile with the chromatic aberration calibration of the camera calibrated Crystal Super Micro-OLED Default. The geometry was fitted from a capture of the distortion mesh rendered by the Pimax compositor, including the optical center sitting 3.6% towards the nose and 2.7% above the panel center, and follows the capture within 0.1 degrees. The chromatic aberration correction reproduces the Micro-OLED Default at every panel radius. Set Max FOV to 98.2x87.1 to render exactly the field the native profile shows.";
	crystalSuperOledLikePimax.author = "Pimax, Yar, SBoys3";
	crystalSuperOledLikePimax.creationDate = 1786233600.0;
	crystalSuperOledLikePimax.type = "RadialBezier";
	crystalSuperOledLikePimax.offsetX = -3.59f;
	crystalSuperOledLikePimax.offsetY = 2.69f;
	crystalSuperOledLikePimax.eyeRotationOffset = -1.0;
	// crystalSuperOledLikePimax.recommendedMaxFovX = 98.2;
	// crystalSuperOledLikePimax.recommendedMaxFovY = 87.1;
	crystalSuperOledLikePimax.distortions = {
		0.00, 0.000,
		2.50, 6.430,
		5.00, 12.871,
		10.0, 25.638,
		15.0, 38.183,
		20.0, 50.374,
		25.0, 62.063,
		30.0, 73.141,
		35.0, 83.498,
		40.0, 93.230,
		45.0, 102.040,
		47.5, 105.733,
		50.0, 108.973,
		52.5, 111.730,
		55.0, 114.792,
		57.5, 118.138
	};
	crystalSuperOledLikePimax.distortionsRed = {
		0.00, 0.30,
		5.00, 0.33,
		10.0, 0.37,
		15.0, 0.40,
		20.0, 0.42,
		25.0, 0.44,
		30.0, 0.46,
		35.0, 0.46,
		40.0, 0.48,
		45.0, 0.48,
		50.0, 0.48,
		55.0, 0.59
	};
	crystalSuperOledLikePimax.distortionsBlue = {
		0.00, -0.20,
		5.00, -0.24,
		10.0, -0.28,
		15.0, -0.32,
		20.0, -0.35,
		25.0, -0.39,
		30.0, -0.48,
		35.0, -0.59,
		40.0, -0.67,
		45.0, -0.68,
		50.0, -0.68,
		55.0, -0.69
	};
	builtInDistortionProfiles[crystalSuperOledLikePimax.name] = crystalSuperOledLikePimax;
	
	DistortionProfileConfig crystalSuperBad = {};
	crystalSuperBad.name = "Crystal Super Bad";
	crystalSuperBad.device = {"CrystalSuper50PPD", "CrystalSuper57PPD", "CrystalSuperUltrawide"};
	crystalSuperBad.modifiedTime = 0;
	crystalSuperBad.description = "This a crude distortion profile for the Crystal Super 50PPD and Ultrawide. The distortion is not good, however the chromatic aberration is fairly well calibrated.";
	crystalSuperBad.author = "SBoys3";
	crystalSuperBad.creationDate = 1783126561.551;
	crystalSuperBad.type = "RadialBezier";
	crystalSuperBad.distortions = {
		0.00, 0.000,
		5.00, 11.80,
		10.0, 23.50,
		15.0, 34.90,
		20.0, 46.00,
		25.0, 56.70,
		30.0, 67.10,
		35.0, 77.30,
		40.0, 87.00,
		45.0, 95.50,
		50.0, 102.70,
		52.5, 105.40,
		55.0, 109.00
	};
	crystalSuperBad.distortionsRed = {
		0.00, 0.50,
		50.0, 0.80
	};
	crystalSuperBad.distortionsBlue = {
		0.00, -0.7,
		50.0, -1.1
	};
	builtInDistortionProfiles[crystalSuperBad.name] = crystalSuperBad;
	
	DistortionProfileConfig crystalSuper50PPDDefault = {};
	crystalSuper50PPDDefault.name = "Crystal Super 50ppd Default";
	crystalSuper50PPDDefault.device = {"CrystalSuper50PPD", "CrystalSuperUltrawide"};
	crystalSuper50PPDDefault.modifiedTime = 0;
	crystalSuper50PPDDefault.description = "Distortion profile for the Crystal Super 50PPD that combines the lens geometry of the Pimax native profile with the chromatic aberration calibration of the old test profile. The geometry, including the optical center sitting 3.8% above the panel center, was fitted from a capture of the distortion mesh rendered by the Pimax compositor, where it stays within 0.05 degrees. The chromatic aberration correction follows old test profile's calibration near the center and grows along a smooth quartic towards the edges, and was tuned by eye in the outer band.";
	crystalSuper50PPDDefault.author = "Pimax, Yar, SBoys3";
	crystalSuper50PPDDefault.creationDate = 1786060800.0;
	crystalSuper50PPDDefault.type = "RadialBezier";
	crystalSuper50PPDDefault.offsetY = 3.8f;
	crystalSuper50PPDDefault.distortions = {
		0.00, 0.000,
		2.50, 5.499,
		5.00, 11.014,
		10.0, 21.965,
		15.0, 32.786,
		20.0, 43.369,
		25.0, 53.609,
		30.0, 63.416,
		35.0, 72.617,
		40.0, 81.122,
		45.0, 88.710,
		47.5, 92.107,
		50.0, 95.252,
		52.5, 98.070,
		55.0, 100.366,
		57.5, 102.950,
		60.0, 105.730,
		62.5, 108.833
	};
	crystalSuper50PPDDefault.distortionsRed = {
		0.00, 0.51,
		5.00, 0.51,
		10.0, 0.52,
		15.0, 0.54,
		20.0, 0.57,
		25.0, 0.60,
		30.0, 0.63,
		35.0, 0.67,
		40.0, 0.71,
		45.0, 0.75,
		50.0, 0.79,
		55.0, 0.82,
		60.0, 0.84,
		62.5, 0.85
	};
	crystalSuper50PPDDefault.distortionsBlue = {
		0.00, -0.71,
		5.00, -0.71,
		10.0, -0.73,
		15.0, -0.76,
		20.0, -0.79,
		25.0, -0.83,
		30.0, -0.88,
		35.0, -0.94,
		40.0, -1.00,
		45.0, -1.06,
		50.0, -1.11,
		55.0, -1.16,
		60.0, -1.20,
		62.5, -1.22
	};
	builtInDistortionProfiles[crystalSuper50PPDDefault.name] = crystalSuper50PPDDefault;
	
	DistortionProfileConfig crystalSuper57LikePimax = {};
	crystalSuper57LikePimax.name = "Crystal Super 57ppd Default";
	crystalSuper57LikePimax.device = {"CrystalSuper57PPD"};
	crystalSuper57LikePimax.modifiedTime = 0;
	crystalSuper57LikePimax.description = "Distortion profile for the Crystal Super 57PPD aiming to closely reproduce the geometric shape of the native Pimax rendering. The chromatic aberration was used from the old Crystal Super test profile. The optical center sits 6.4% above the panel center and the profile follows the native rendering within 0.08 degrees. The eye rotation offset brings the configured 7 degrees to the 6 degree cant of the captured eye pose, the same cant as the other Crystal Super QLED engines. Set Max FOV to 88.6x86.1 to render exactly the field the native profile shows.";
	crystalSuper57LikePimax.author = "Pimax, Yar, SBoys3";
	crystalSuper57LikePimax.creationDate = 1787961600.0;
	crystalSuper57LikePimax.type = "RadialBezier";
	crystalSuper57LikePimax.offsetX = 0.0f;
	crystalSuper57LikePimax.offsetY = 6.38f;
	crystalSuper57LikePimax.eyeRotationOffset = 0.0;
	// crystalSuper57LikePimax.recommendedMaxFovX = 88.6;
	// crystalSuper57LikePimax.recommendedMaxFovY = 86.1;
	crystalSuper57LikePimax.distortions = {
		0.00, 0.000,
		2.50, 6.178,
		5.00, 12.356,
		10.0, 24.731,
		15.0, 37.043,
		20.0, 49.212,
		25.0, 61.070,
		30.0, 72.513,
		35.0, 83.307,
		40.0, 93.783,
		45.0, 103.411,
		47.5, 107.698,
		50.0, 112.366,
		52.5, 117.083,
		54.0, 120.085
	};
	crystalSuper57LikePimax.distortionsRed = {
		0.00, 0.50,
		50.0, 0.80
	};
	crystalSuper57LikePimax.distortionsBlue = {
		0.00, -0.7,
		50.0, -1.1
	};
	builtInDistortionProfiles[crystalSuper57LikePimax.name] = crystalSuper57LikePimax;
	
	
	DistortionProfileConfig crystalSuperUltrawideLikePimax = {};
	crystalSuperUltrawideLikePimax.name = "Crystal Super Ultrawide Default";
	crystalSuperUltrawideLikePimax.device = {"CrystalSuperUltrawide"};
	crystalSuperUltrawideLikePimax.modifiedTime = 0;
	crystalSuperUltrawideLikePimax.description = "Distortion profile for the Crystal Super Ultrawide aiming to closely reproduce the geometric shape of the native Pimax rendering. The chromatic aberration was used from the old Crystal Super test profile. The optical center sits 6.4% towards the nose and 6.4% above the panel center, and the profile follows the native rendering within 0.06 degrees. The eye rotation offset brings the configured 9 degrees to the 6 degree cant of the captured eye pose, the same cant as the other Crystal Super QLED engines. Set Max FOV to 111.3x107.3 to render exactly the field the native profile shows.";
	crystalSuperUltrawideLikePimax.author = "Pimax, Yar, SBoys3";
	crystalSuperUltrawideLikePimax.creationDate = 1787961600.0;
	crystalSuperUltrawideLikePimax.type = "RadialBezier";
	crystalSuperUltrawideLikePimax.offsetX = -6.38f;
	crystalSuperUltrawideLikePimax.offsetY = 6.38f;
	crystalSuperUltrawideLikePimax.eyeRotationOffset = -3.0;
	// crystalSuperUltrawideLikePimax.recommendedMaxFovX = 111.3;
	// crystalSuperUltrawideLikePimax.recommendedMaxFovY = 107.3;
	crystalSuperUltrawideLikePimax.distortions = {
		0.00, 0.000,
		2.50, 5.401,
		5.00, 10.782,
		10.0, 21.558,
		15.0, 32.241,
		20.0, 42.764,
		25.0, 53.018,
		30.0, 62.918,
		35.0, 72.286,
		40.0, 81.020,
		45.0, 88.966,
		47.5, 92.620,
		50.0, 96.090,
		52.5, 99.265,
		55.0, 101.656,
		57.5, 104.180,
		60.0, 106.978,
		62.5, 110.070,
		65.0, 113.448,
		67.5, 117.313
	};
	crystalSuperUltrawideLikePimax.distortionsRed = crystalSuper50PPDDefault.distortionsRed;
	crystalSuperUltrawideLikePimax.distortionsBlue = crystalSuper50PPDDefault.distortionsBlue;
	builtInDistortionProfiles[crystalSuperUltrawideLikePimax.name] = crystalSuperUltrawideLikePimax;
	

	DistortionProfileConfig crystalOGLikePimax = {};
	// crystalOGLikePimax.name = "Crystal OG like Pimax";
	crystalOGLikePimax.name = "Crystal OG Default";
	crystalOGLikePimax.device = {"CrystalOG", "CrystalLight"};
	crystalOGLikePimax.modifiedTime = 0;
	crystalOGLikePimax.description = "Distortion profile for the Pimax Crystal fitted from a capture of the distortion mesh rendered by the Pimax compositor. The chromatic aberration was used from the old Crystal Super test profile. The optical center sits 7.0% above the panel center, so the field reaches noticeably further down than up, and the profile follows the capture within 0.1 degrees. Set Max FOV to 92.3x93.1 to render exactly the field the native profile shows.";
	crystalOGLikePimax.author = "Pimax, Yar, SBoys3";
	crystalOGLikePimax.creationDate = 1786838400.0;
	crystalOGLikePimax.type = "RadialBezier";
	crystalOGLikePimax.offsetX = 0.0f;
	crystalOGLikePimax.offsetY = 7.04f;
	crystalOGLikePimax.eyeRotationOffset = 0.0;
	// crystalOGLikePimax.recommendedMaxFovX = 92.3;
	// crystalOGLikePimax.recommendedMaxFovY = 93.1;
	crystalOGLikePimax.distortions = {
		0.00, 0.000,
		2.50, 6.066,
		5.00, 12.177,
		10.0, 24.230,
		15.0, 36.183,
		20.0, 47.859,
		25.0, 59.140,
		30.0, 69.980,
		35.0, 80.123,
		40.0, 89.485,
		45.0, 97.878,
		47.5, 101.621,
		50.0, 105.079,
		52.5, 108.201,
		55.0, 110.749,
		57.5, 113.563
	};
	crystalOGLikePimax.distortionsRed = {
		0.00, 0.50,
		50.0, 0.80
	};
	crystalOGLikePimax.distortionsBlue = {
		0.00, -0.7,
		50.0, -1.1
	};
	builtInDistortionProfiles[crystalOGLikePimax.name] = crystalOGLikePimax;
	
	
	DistortionProfileConfig pimaxBuiltin = {};
	pimaxBuiltin.name = "Pimax Builtin";
	pimaxBuiltin.device = {/*"DreamAir", "DreamAirSE", "CrystalSuper50PPD", "CrystalSuper57PPD", "CrystalSuperUltrawide", "CrystalSuperMicroOLED", "CrystalLight", "CrystalOG",*/ "Pimax5KSuper", "Pimax5KPlus", "Pimax8KX", "Pimax8KPlus", "PimaxArtisan"};
	pimaxBuiltin.modifiedTime = 0;
	pimaxBuiltin.description = "Distortion Profile retrieved from Pimax API.";
	pimaxBuiltin.author = "Pimax";
	pimaxBuiltin.creationDate = 0.0;
	pimaxBuiltin.type = "Pimax";
	builtInDistortionProfiles[pimaxBuiltin.name] = pimaxBuiltin;

	return true;
};
// this is pretty much just an initializer so immediately call
bool voidBool = PopulateBuiltInDistortionProfiles();


bool DistortionProfileConstructor::LoadDistortionProfile(std::string name){
	
	DistortionProfileConfig config = {};
	
	std::string aliasedName = name;
	if(builtInDistortionProfileAliases.find(name) != builtInDistortionProfileAliases.end()){
		aliasedName = builtInDistortionProfileAliases[name];
	}
	
	if(builtInDistortionProfiles.find(aliasedName) != builtInDistortionProfiles.end()){
		config = builtInDistortionProfiles[aliasedName];
	}
	
	if(config.name == "None"){
		DistortionProfileConfig configFromDisk = driverConfigLoader.ParseDistortionConfig(name);
		if(configFromDisk.name != "None"){
			config = configFromDisk;
		}
	}
	
	
	// check if the profile has not changed to avoid recreating it
	if(profile != nullptr && config.name == profileName && config.modifiedTime == profileModifiedTime){
		return false;
	}
	
	DistortionProfile* newProfile = nullptr;
		
	// construct RadialBezierDistortionProfile object from config
	if(config.type == "RadialBezier"){
		RadialBezierDistortionProfile* radialBezierProfile = new RadialBezierDistortionProfile();
		if(config.distortions.size() >= 2){
			radialBezierProfile->distortions.clear();
			for(int i = 0; i < config.distortions.size() / 2; i++){
				radialBezierProfile->distortions.push_back({(float)config.distortions[i * 2], (float)config.distortions[i * 2 + 1]});
			}
		}
		if(config.distortionsRed.size() >= 2){
			radialBezierProfile->distortionsRed.clear();
			for(int i = 0; i < config.distortionsRed.size() / 2; i++){
				radialBezierProfile->distortionsRed.push_back({(float)config.distortionsRed[i * 2], (float)config.distortionsRed[i * 2 + 1]});
			}
		}
		if(config.distortionsBlue.size() >= 2){
			radialBezierProfile->distortionsBlue.clear();
			for(int i = 0; i < config.distortionsBlue.size() / 2; i++){
				radialBezierProfile->distortionsBlue.push_back({(float)config.distortionsBlue[i * 2], (float)config.distortionsBlue[i * 2 + 1]});
			}
		}
		radialBezierProfile->legacySmoothing = config.legacySmoothing;
		radialBezierProfile->smoothAmount = config.smoothAmount;
		radialBezierProfile->offsetX = config.offsetX;
		radialBezierProfile->offsetY = config.offsetY;
		radialBezierProfile->eyeRotationOffset = (float)config.eyeRotationOffset;
		newProfile = radialBezierProfile;
	}
	else if (config.type == "Pimax"){
		#ifdef PVR_EXISTS
		DriverLog("Using Pimax PVR distortion function");
		PimaxDistortionProfile* pimaxProfile = new PimaxDistortionProfile();
		newProfile = pimaxProfile;
		#else
		DriverLog("Pimax PVR distortion profile is not available");
		#endif
	}

	bool changed = false;
	
	if(newProfile != nullptr){
		if(profile != nullptr && profile != &distortionSettings){
			delete profile;
		}
		profile = newProfile;
		ReInitializeProfile();
		changed = true;
	}
	
	// fallback to default profile if nothing was set
	if(newProfile == nullptr && profile != &distortionSettings){
		if(profile != nullptr){
			delete profile;
		}
		profile = &distortionSettings;
		changed = true;
	}
	
	profileName = config.name;
	profileModifiedTime = config.modifiedTime;
	return changed;
}

void DistortionProfileConstructor::ReInitializeProfile(){
	// copy settings to new distortion profile
	profile->resolution = distortionSettings.resolution;
	profile->resolutionX = distortionSettings.resolutionX;
	profile->resolutionY = distortionSettings.resolutionY;
	profile->maxFovX = distortionSettings.maxFovX;
	profile->maxFovY = distortionSettings.maxFovY;
	profile->fovZoom = distortionSettings.fovZoom;
	profile->flatFovZoom = distortionSettings.flatFovZoom;
	profile->fovClamping = distortionSettings.fovClamping;
	if(profile->fovZoom == 0.0f){
		// avoid division by zero in calculations because invalid distortion data can prevent the compositor from starting
		profile->fovZoom = 1.0f; 
	}
	// initialize new profile and replace old one
	profile->Initialize();
}

void DistortionProfileConstructor::GetRecommendedRenderTargetSize(uint32_t* pnWidth, uint32_t* pnHeight){
	uint32_t originalWidth = (uint32_t)distortionSettings.resolutionX;
	uint32_t originalHeight = (uint32_t)distortionSettings.resolutionY;
	uint32_t renderWidth = originalWidth;
	uint32_t renderHeight = originalHeight;
	if(profile != nullptr){
		profile->GetRecommendedRenderTargetSize(&renderWidth, &renderHeight);
	}
	// keep total number of pixels the same but change aspect ratio
	double targetPixels = originalWidth * originalHeight;
	double renderPixels = renderWidth * renderHeight;
	renderWidth = (uint32_t)(renderWidth * std::sqrt(targetPixels / renderPixels));
	renderHeight = (uint32_t)(renderHeight * std::sqrt(targetPixels / renderPixels));
	renderWidth = std::min(renderWidth, 16384u);
	renderHeight = std::min(renderHeight, 16384u);
	DriverLog("100% render target size: %d x %d", renderWidth, renderHeight);
	*pnWidth = renderWidth;
	*pnHeight = renderHeight;
}

DistortionProfileConstructor::~DistortionProfileConstructor(){
	if(profile != nullptr && profile != &distortionSettings){
		delete profile;
	}
}
