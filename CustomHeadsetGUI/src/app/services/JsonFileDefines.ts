export type DistortionProfileConfig = {
  name: string;
  description: string;
  modifiedTime: number;
  type: string;
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
  generalHeadset: GeneralHeadsetConfig,
  customShader: CustomShaderConfig,
  forceTracking: boolean,
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
  srgbColorCorrectionMatrix: number[]; // 3x3 matrix as a flat array of 9 elements
  lensColorCorrection: boolean;
  dither10Bit: boolean;
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
  ipd: number;
  ipdOffset: number;
  blackLevel: number;
  colorMultiplier: LinearColor;
  distortionProfile: string;
  distortionZoom: number;
  fovZoom: number;
  subpixelShift: number;
  resolutionX: number;
  resolutionY: number;
  maxFovX: number;
  maxFovY: number;
  distortionMeshResolution: number;
  fovBurnInPrevention: boolean;
  renderResolutionMultiplierX: number;
  renderResolutionMultiplierY: number;
  superSamplingFilterPercent: number;
  secondsFromVsyncToPhotons: number;
  secondsFromPhotonsToVblank: number;
  eyeRotation: number;
  disableEye: number;
  disableEyeDecreaseFov: number;
  edidVendorIdOverride: number;
  hiddenArea: HiddenAreaMeshConfig;
  stationaryDimming: StationaryDimmingConfig;
};

export type MeganeX8KConfig = BaseHeadsetConfig & {
  // MeganeX8K-specific fields can be added here if needed
};

export type DreamAirConfig = BaseHeadsetConfig & {
  // DreamAir-specific fields can be added here if needed
};

export type GeneralHeadsetConfig = {
  useViveBluetooth: boolean;
}

export type AppSetting = {
  colorScheme: 'system' | 'dark' | 'light';
  updateMode: 'replace' | 'rewrite';
  advanceMode: boolean
}

export const HeadsetType = {
  None: 0,
  Other: 1,
  MeganeX8K: 2,
  Vive: 3,
  DreamAir: 4,
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
  renderResolution1To1X: number,
  renderResolution1To1Y: number,
  renderResolution1To1Percent: number,
  renderResolution100PercentX: number,
  renderResolution100PercentY: number
}

export type BuiltInDistortionProfiles = {
  [profileName: string]: {};
}
