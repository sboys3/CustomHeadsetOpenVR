export type DistortionProfileConfig = {
  name: string;
  description: string;
  modifiedTime: number;
  type: string;
  distortions: number[];
  distortionsRed: number[];
  distortionsBlue: number[];
};
export type Settings = {
  watchDistortionProfiles: boolean,
  meganeX8K: MeganeX8KConfig
}
export type MeganeX8KConfig = {
  /**
   * if the MeganeX superlight 8K should be shimmed by this driver
   */
  enable: boolean;

  /**
   * ipd in mm
   */
  ipd: number;

  /**
   * ipd offset from the ipd value in mm
   */
  ipdOffset: number;

  /**
   * minimum black levels from 0 to 1
   */
  blackLevel: number;

  /**
   * distortion profile to use
   */
  distortionProfile: string;

  /**
   * amount to zoom in the distortion profile
   */
  distortionZoom: number;

  /**
   * amount to shift the subpixels to account for their different rows
   */
  subpixelShift: number;

  resolutionX: number;

  resolutionY: number;

  maxFovX: number;

  maxFovY: number;

  distortionMeshResolution: number;

  fovBurnInPrevention: boolean;

  renderResolutionMultiplierX: number;

  renderResolutionMultiplierY: number;
};

export type AppSetting = {
  colorScheme: 'system' | 'dark' | 'light';
  updateMode: 'replace' | 'rewrite';
  advanceMode: boolean
}

export type DriverInfo = {
  about: string;
  defaultSettings: Settings;
  builtInDistortionProfiles: BuiltInDistortionProfiles;
  resolution: ResolutionInfo,
  driverVersion: string
}
export type ResolutionInfo = {
  renderResolution1To1X: number,
  renderResolution1To1Y: number,
  renderResolution1To1Percent: number,
  renderResolution100PercentX: number,
  renderResolution100PercentY: number
}

export type BuiltInDistortionProfiles = {
  [profileName: string]: {};
}
