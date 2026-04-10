import { Component, effect, input, output } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
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
import { BaseHeadsetConfig } from '../../../services/JsonFileDefines';

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
  showDriverWarning: boolean;
  ipdInfo: string;
  defaultMaxFovX: number;
  defaultMaxFovY: number;
}

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
  
  resolutionModel?: number;
  subpixelShiftOptions = [0, 0.33];
  disableEyeOptions: DisableEyeOption[] = [
    { name: $localize`None`, value: 0 },
    { name: $localize`Left`, value: 1 },
    { name: $localize`Right`, value: 2 },
    { name: $localize`Both`, value: 3 }
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
        this.resolutionModel = this.settings.resolutionX;
      }
    });
  }

  resetResolution() {
    this.resolutionModel = this.defaults?.resolutionX;
    this.setResolution();
  }

  setResolution() {
    if (this.settings && this.resolutionModel) {
      const res = this.config().resolutionOptions.find(x => x.x === this.resolutionModel);
      if (res) {
        this.settings.resolutionX = res.x;
        this.settings.resolutionY = res.y;
        this.saveConfigSettings();
      }
    }
  }

  scaledMaxFOV(originalFOV: number) {
    return Math.ceil(originalFOV / (this.settings?.fovZoom || 1));
  }

  combinedHfov(horizontalFov: number) {
    return Math.max(0, horizontalFov + (this.settings?.eyeRotation || 0) * 2);
  }

  binocularOverlap(horizontalFov: number) {
    return Math.max(0, horizontalFov - (this.settings?.eyeRotation || 0) * 2);
  }

  showBurnInDeviationNote(maxPossibleFovX: number, maxPossibleFovY: number) {
    if (!this.settings?.fovBurnInPrevention) {
      return false;
    }
    return this.settings.maxFovX < maxPossibleFovX || this.settings.maxFovY < maxPossibleFovY;
  }
}
