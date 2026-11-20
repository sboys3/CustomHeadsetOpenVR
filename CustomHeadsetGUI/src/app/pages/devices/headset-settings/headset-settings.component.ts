import { Component, computed, effect, input, output } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule, NgModel } from '@angular/forms';
import { MatButtonModule } from '@angular/material/button';
import { MatDividerModule } from '@angular/material/divider';
import { MatIconModule } from '@angular/material/icon';
import { MatInputModule } from '@angular/material/input';
import { MatSelectModule } from '@angular/material/select';
import { MatSlideToggleModule } from '@angular/material/slide-toggle';
import { MatSliderModule } from '@angular/material/slider';
import { MatTooltipModule } from '@angular/material/tooltip';
import { FieldNoteComponent } from '../../../utilities/field-note/field-note.component';
import { FieldTipComponent } from '../../../utilities/field-tip/field-tip.component';
import { ResetButtonComponent } from '../../../utilities/reset-button/reset-button.component';
import { ColorPickerComponent } from '../../../utilities/color-picker/color-picker.component';
import { DeviceConfigComponentBase } from '../DeviceConfigComponentBase';
import { BaseHeadsetConfig, HeadsetType, ResolutionInfo } from '../../../services/JsonFileDefines';

export interface ResolutionOption {
  name: string;
  x: number;
  y: number;
}

export interface DisableEyeOption {
  name: string;
  value: number;
}

export interface HeadsetSettingsConfig {
  driverName: string | string[];
  settingField: string;
  enableText: string;
  enableInfo: string;
  resolutionInfo: string;
  resolutionOptions: ResolutionOption[];
  showResolutionSelector: boolean;
  showDisplayRotation: boolean;
  showDriverWarning: boolean;
  ipdInfo: string;
  defaultMaxFovX: number;
  defaultMaxFovY: number;
}

type EdgeFovKey = 'maxFovOuter' | 'maxFovInner' | 'maxFovTop' | 'maxFovBottom';

@Component({
  selector: 'app-headset-settings',
  imports: [
    MatSliderModule,
    MatSelectModule,
    FormsModule,
    MatDividerModule,
    MatButtonModule,
    MatIconModule,
    MatInputModule,
    MatSlideToggleModule,
    MatTooltipModule,
    ResetButtonComponent,
    FieldTipComponent,
    FieldNoteComponent,
    ColorPickerComponent,
    CommonModule
  ],
  templateUrl: './headset-settings.component.html',
  styleUrl: './headset-settings.component.scss'
})
export class HeadsetSettingsComponent extends DeviceConfigComponentBase<BaseHeadsetConfig> {
  config = input.required<HeadsetSettingsConfig>();
  override settingField = input.required<string>();
  override driverName = input.required<string | string[]>();
  headsetType = input.required<HeadsetType>();
  navigateToGeneralTab = output<void>();
  headsetMatches = computed(() => {
    const connectedHeadset = this.dis.values()?.connectedHeadset;
    return connectedHeadset !== undefined && connectedHeadset === this.headsetType();
  });
  headsetMismatch = computed(() => {
    const current = this.dis.currentHeadsetType();
    return current !== undefined && current !== this.headsetType();
  });
  
  resolutionModel?: number; // index into resolutionOptions
  defaultResolutionIndex = computed(() => {
    if (!this.defaults) {
      return 0;
    }
    const idx = this.config().resolutionOptions.findIndex(r => r.x === this.defaults!.resolutionX && r.y === this.defaults!.resolutionY);
    return idx >= 0 ? idx : 0;
  });
  displayRotationOptions = [
    { name: $localize`0°`, value: 0 },
    { name: $localize`90°`, value: 1 },
    { name: $localize`180°`, value: 2 },
    { name: $localize`270°`, value: 3 },
  ];
  subpixelShiftOptions = [-1, 0, 1];
  disableEyeOptions: DisableEyeOption[] = [
    { name: $localize`None`, value: 0 },
    { name: $localize`Left`, value: 1 },
    { name: $localize`Right`, value: 2 },
    { name: $localize`Both`, value: 3 }
  ];
  proximitySensorTypeOptions = [
    { name: $localize`None`, value: 0 },
    { name: $localize`Hardware`, value: 1 },
    { name: $localize`Stationary Dimming`, value: 2 },
    { name: $localize`Always On`, value: 3 },
    { name: $localize`Always Off`, value: 4 },
  ];
  distortionMeshResolutionOptions = [63, 127, 255];
  distortionMeshResolutionOptionsText: { [key: number]: string } = {
    63: $localize`Low (${63})`,
    127: $localize`Normal (${127})`,
    255: $localize`High (${255})`
  };

  constructor() {
    super();
    effect(() => {
      this.dss.values();
      if (this.config().showResolutionSelector && this.settings) {
        const idx = this.config().resolutionOptions.findIndex(r => r.x === this.settings!.resolutionX && r.y === this.settings!.resolutionY);
        this.resolutionModel = idx >= 0 ? idx : 0;
      }
    });
    effect(() => {
      this.dss.values();
      if (this.config().showDisplayRotation && this.settings) {
        this.displayRotationModel = this.settings.displayRotation;
      }
    });
  }

  resetResolution() {
    const idx = this.config().resolutionOptions.findIndex(r => r.x === this.defaults!.resolutionX && r.y === this.defaults!.resolutionY);
    this.resolutionModel = idx >= 0 ? idx : 0;
    this.setResolution();
  }

  resetIpd() {
    if (this.settings && this.defaults) {
      this.settings.hardwareIpd = this.defaults.hardwareIpd;
      this.settings.ipd = this.defaults.ipd;
      this.saveConfigSettings();
    }
  }

  resetEyeRotation() {
    if (this.settings && this.defaults) {
      this.settings.autoEyeRotation = this.defaults.autoEyeRotation;
      this.settings.eyeRotation = this.defaults.eyeRotation;
      this.saveConfigSettings();
    }
  }

  setResolution() {
    if (this.settings && this.resolutionModel != null) {
      const res = this.config().resolutionOptions[this.resolutionModel];
      if (res) {
        this.settings.resolutionX = res.x;
        this.settings.resolutionY = res.y;
        this.saveConfigSettings();
      }
    }
  }

  displayRotationModel?: number;

  setDisplayRotation() {
    if (this.settings && this.displayRotationModel != null) {
      this.settings.displayRotation = this.displayRotationModel;
      this.saveConfigSettings();
    }
  }

  // Computed keys to force slider re-render when max changes
  sliderMaxFovX = computed(() => Math.ceil(this.headsetMatches() ? this.dis.values()?.resolution?.fovMaxX || this.config().defaultMaxFovX : this.config().defaultMaxFovX));
  sliderMaxFovY = computed(() => Math.ceil(this.headsetMatches() ? this.dis.values()?.resolution?.fovMaxY || this.config().defaultMaxFovY : this.config().defaultMaxFovY));
  // Capped at 89 so the slider never reaches 90, which the driver reads as no limit
  edgeSliderMax = computed<Record<EdgeFovKey, number>>(() => {
    const res = this.headsetMatches() ? this.dis.values()?.resolution : undefined;
    const max = (value?: number) => Math.min(Math.ceil(value || 90), 89);
    return {
      maxFovOuter: max(res?.fovMaxOuter),
      maxFovInner: max(res?.fovMaxInner),
      maxFovTop: max(res?.fovMaxTop),
      maxFovBottom: max(res?.fovMaxBottom),
    };
  });

  private edgeFovMaxKey: Record<EdgeFovKey, keyof ResolutionInfo> = {
    maxFovOuter: 'fovMaxOuter',
    maxFovInner: 'fovMaxInner',
    maxFovTop: 'fovMaxTop',
    maxFovBottom: 'fovMaxBottom',
  };

  edgeFovValue(key: EdgeFovKey): number {
    const value = this.settings?.[key] ?? 90;
    const sliderMax = this.edgeSliderMax()[key];
    if (value < 90) {
      return Math.min(value, sliderMax);
    }
    if (this.settings?.fovClamping === false) {
      // The driver multiplies the per-edge maxima by 1.1 when clamping is off (BaseHeadset.cpp)
      const fovMax = this.headsetMatches() ? this.dis.values()?.resolution?.[this.edgeFovMaxKey[key]] : undefined;
      if (fovMax) {
        return Math.min(Math.round(fovMax / 1.1), sliderMax);
      }
    }
    return sliderMax;
  }

  // With clamping on, the slider end stores 90 (no limit) so a wider profile stays uncropped.
  // With clamping off, the value is stored as given, the same as maxFovX/maxFovY.
  setEdgeFovValue(key: EdgeFovKey, value: number) {
    if (!this.settings || value == null) {
      return;
    }
    this.settings[key] = this.settings.fovClamping !== false && value >= this.edgeSliderMax()[key] ? 90 : value;
    this.saveConfigSettings();
  }

  // Show the stored value again when a typed value was stored as something else
  syncEdgeInput(key: EdgeFovKey, model: NgModel) {
    model.control.setValue(this.edgeFovValue(key), { emitViewToModelChange: false });
  }

  trackBySliderMax(index: number, value: number): number {
    return value;
  }

  scaledMaxFOV(originalFOV: number) {
    return Math.ceil(originalFOV / (this.settings?.fovZoom || 1));
  }

  // The combined FOV counts the outer edges twice plus the eye rotation, so twice the per eye FOV minus the combined FOV leaves twice the inner edge minus rotation.
  binocularOverlap(res: ResolutionInfo) {
    if (res.combinedFovX) {
      return Math.max(0, 2 * res.fovX - res.combinedFovX);
    }
    return Math.max(0, res.fovX - (this.settings?.eyeRotation || 0) * 2);
  }

  showBurnInDeviationNote(res: ResolutionInfo) {
    if (!this.settings?.fovBurnInPrevention) {
      return false;
    }
    if (this.settings.advancedFovLimits) {
      return this.settings.maxFovOuter < res.fovMaxOuter
        || this.settings.maxFovInner < res.fovMaxInner
        || this.settings.maxFovTop < res.fovMaxTop
        || this.settings.maxFovBottom < res.fovMaxBottom;
    }
    return this.settings.maxFovX < res.fovMaxX
      || this.settings.maxFovY < res.fovMaxY;
  }
}