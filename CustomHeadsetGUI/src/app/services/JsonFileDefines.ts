export type DistortionProfileConfig = {
  name: string;
  description: string;
  modifiedTime: number;
  type: string;
  distortionProfileId: string;
  distortions: number[];
  distortionsRed: number[];
  distortionsBlue: number[];
};
/**
 * See the c++ documentation for documentation on what the settings are for
 */
export type Settings = {
  meganeX8K: MeganeX8KConfig,
  dreamAir: DreamAirConfig,
  dreamAirSE: DreamAirSEConfig,
  crystalSuper50PPD: CrystalSuper50PPDConfig,
  crystalSuper57PPD: CrystalSuper57PPDConfig,
  crystalSuperUltrawide: CrystalSuperUltrawideConfig,
  crystalSuperMicroOLED: CrystalSuperMicroOLEDConfig,
  crystalLight: CrystalLightConfig,
  crystalOG: CrystalOGConfig,
  pimax5KSuper: Pimax5KSuperConfig,
  pimax5KPlus: Pimax5KPlusConfig,
  pimax8KX: Pimax8KXConfig,
  pimax8KPlus: Pimax8KPlusConfig,
  pimaxArtisan: PimaxArtisanConfig,
  generalHeadset: GeneralHeadsetConfig,
  customShader: CustomShaderConfig,
  forceTracking: boolean,
  forceTrackingHeadsetOnly: boolean,
  takeCompositorScreenshots: boolean,
  watchDistortionProfiles: boolean,
}
export type StationaryDimmingConfig = {
  enable: boolean,
  movementThreshold: number,
  movementTime: number,
  dimBrightnessPercent: number,
  dimSeconds: number,
  brightenSeconds: number
}
export type HiddenAreaMeshConfig = {
  enable: boolean;
  testMode: boolean;
  detailLevel: number;
  radiusTopOuter: number;
  radiusTopInner: number;
  radiusBottomInner: number;
  radiusBottomOuter: number;
};
export type LinearColor = {
  r: number;
  g: number;
  b: number;
};
export type CustomShaderConfig = {
  enable: boolean;
  enableForMeganeX8K: boolean,
  enableForPimax: boolean,
  enableForOther: boolean,
  contrast: number;
  contrastMidpoint: number;
  contrastLinear: boolean;
  contrastPerEye: boolean;
  contrastPerEyeLinear: boolean;
  contrastLeft: number;
  contrastMidpointLeft: number;
  contrastRight: number;
  contrastMidpointRight: number;
  chroma: number;
  saturation: number;
  gamma: number;
  subpixelShift: boolean;
  disableMuraCorrection: boolean;
  disableBlackLevels: boolean;
  srgbColorCorrection: boolean;
  srgbWhitePointCorrection: boolean;
  srgbColorCorrectionMatrix: number[]; // 3x3 matrix as a flat array of 9 elements
  lensColorCorrection: boolean;
  dither10Bit: boolean;
  enableFilterForOverlay: boolean;
  enableFilterForDashboard: boolean;
  samplingFilter: string;
  samplingFilterFXAA2SharpenStrength: number;
  samplingFilterFXAA2SharpenClamp: number;
  samplingFilterFXAA2CASStrength: number;
  samplingFilterFXAA2CASContrast: number;
  samplingFilterLumaSharpenStrength: number;
  samplingFilterLumaSharpenClamp: number;
  samplingFilterLumaSharpenPattern: number;
  samplingFilterLumaSharpenRadius: number;
  samplingFilterCASStrength: number;
  samplingFilterCASContrast: number;
  colorMultiplier: LinearColor;
}

/**
 * Base configuration type for headset devices that share common settings.
 * Mirrors the BaseHeadsetConfig class in Config.h
 */
export type BaseHeadsetConfig = {
  enable: boolean;
  forceEnable: boolean;
  ipd: number;
  ipdOffset: number;
  horizontalIPDOffset: number;
  blackLevel: number;
  colorMultiplier: LinearColor;
  distortionProfile: string;
  distortionZoom: number;
  fovZoom: number;
  flatFovZoom: number;
  subpixelShift: number;
  subpixelOffsets: number[];
  resolutionX: number;
  resolutionY: number;
  displayRotation: number;
  maxFovX: number;
  maxFovY: number;
  distortionMeshResolution: number;
  fovBurnInPrevention: boolean;
  fovClamping: boolean;
  distortionProfileDeviceType: string;
  renderResolutionMultiplierX: number;
  renderResolutionMultiplierY: number;
  superSamplingFilterPercent: number;
  secondsFromVsyncToPhotons: number;
  secondsFromPhotonsToVblank: number;
  eyeRotation: number;
  autoEyeRotation: boolean;
  hardwareIpd: boolean;
  disableEye: number;
  disableEyeDecreaseFov: number;
  edidVendorIdOverride: number;
  hiddenArea: HiddenAreaMeshConfig;
  stationaryDimming: StationaryDimmingConfig;
  parallelProjection: boolean;
  enableEyeTracking: boolean;
};

export type MeganeX8KConfig = BaseHeadsetConfig & {
  // MeganeX8K-specific fields can be added here if needed
};

export type DreamAirConfig = BaseHeadsetConfig & {
  // DreamAir-specific fields can be added here if needed
};

export type DreamAirSEConfig = BaseHeadsetConfig & {
  // DreamAirSE-specific fields can be added here if needed
};

export type CrystalSuper50PPDConfig = BaseHeadsetConfig & {
  // CrystalSuper50PPD-specific fields can be added here if needed
};

export type CrystalSuper57PPDConfig = BaseHeadsetConfig & {
  // CrystalSuper57PPD-specific fields can be added here if needed
};

export type CrystalSuperUltrawideConfig = BaseHeadsetConfig & {
  // CrystalSuperUltrawide-specific fields can be added here if needed
};

export type CrystalSuperMicroOLEDConfig = BaseHeadsetConfig & {
  // CrystalSuperMicroOLED-specific fields can be added here if needed
};

export type CrystalLightConfig = BaseHeadsetConfig & {
  // CrystalLight-specific fields can be added here if needed
};

export type CrystalOGConfig = BaseHeadsetConfig & {
  // CrystalOG-specific fields can be added here if needed
};

export type Pimax5KSuperConfig = BaseHeadsetConfig & {
  // Pimax5KSuper-specific fields can be added here if needed
};

export type Pimax5KPlusConfig = BaseHeadsetConfig & {
  // Pimax5KPlus-specific fields can be added here if needed
};

export type Pimax8KXConfig = BaseHeadsetConfig & {
  // Pimax8KX-specific fields can be added here if needed
};

export type Pimax8KPlusConfig = BaseHeadsetConfig & {
  // Pimax8KPlus-specific fields can be added here if needed
};

export type PimaxArtisanConfig = BaseHeadsetConfig & {
  // PimaxArtisan-specific fields can be added here if needed
};

export type GeneralHeadsetConfig = {
  useViveBluetooth: boolean;
}

export type AppSetting = {
  colorScheme: 'system' | 'dark' | 'light' | 'evo';
  updateMode: 'replace' | 'rewrite';
  advanceMode: boolean;
  defaultSettingsTab: 'auto' | 'General' | 'MeganeX8K' | 'DreamAir' | 'DreamAirSE' | 'CrystalSuperMicroOLED' | 'Pimax8KX';
  showIncompatibleProfiles: boolean;
  launchPimaxOnStartup: boolean;
  installMethod: 'auto' | 'register' | 'copy';
}

export const HeadsetType = {
  None: 0,
  Other: 1,
  MeganeX8K: 2,
  Vive: 3,
  DreamAir: 4,
  DreamAirSE: 5,
  CrystalSuper50PPD: 6,
  CrystalSuper57PPD: 7,
  CrystalSuperUltrawide: 8,
  CrystalSuperMicroOLED: 9,
  CrystalLight: 14,
  CrystalOG: 15,
  Pimax5KSuper: 16,
  Pimax5KPlus: 17,
  Pimax8KX: 18,
  Pimax8KPlus: 19,
  PimaxArtisan: 20,
} as const;

export type HeadsetType = typeof HeadsetType[keyof typeof HeadsetType];

export type DriverInfo = {
  about: string;
  defaultSettings: Settings;
  builtInDistortionProfiles: BuiltInDistortionProfiles;
  resolution: ResolutionInfo,
  driverVersion: string,
  connectedHeadset: number,
  nonNativeHeadsetFound: boolean
}
export type ResolutionInfo = {
  fovX: number,
  fovY: number,
  fovMaxX: number,
  fovMaxY: number,
  combinedFovX: number,
  combinedFovY: number,
  renderResolution1To1X: number,
  renderResolution1To1Y: number,
  renderResolution1To1Percent: number,
  renderResolution100PercentX: number,
  renderResolution100PercentY: number
}

export type BuiltInDistortionProfile = {
  device?: string | string[];
  distortionProfileId?: string;
  description?: string;
  author?: string;
  creationDate?: number;
  type?: string;
  distortions?: number[];
  distortionsRed?: number[];
  distortionsBlue?: number[];
  legacySmoothing?: boolean;
  smoothAmount?: number;
  offsetX?: number;
  offsetY?: number;
};

export type BuiltInDistortionProfiles = {
  [profileName: string]: BuiltInDistortionProfile;
};
