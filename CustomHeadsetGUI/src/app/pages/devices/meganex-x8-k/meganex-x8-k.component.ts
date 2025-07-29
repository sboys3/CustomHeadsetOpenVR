import { Component, effect, model, signal } from '@angular/core';
import { HiddenAreaMeshConfig, MeganeX8KConfig, StationaryDimmingConfig } from '../../../services/JsonFileDefines';
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
import { DeviceConfigComponentBase } from '../DeviceConfigComponentBase';
import {ColorPickerComponent} from '../../../utilities/color-picker/color-picker.component'

@Component({
  selector: 'app-meganex-x8-k',
  imports: [MatSliderModule,
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
    CommonModule],
  templateUrl: './meganex-x8-k.component.html',
  styleUrl: './meganex-x8-k.component.scss'
})
export class MeganexX8KComponent extends DeviceConfigComponentBase<MeganeX8KConfig> {
  subpixelShiftOptions = [0, 0.33];

  resolutionModel?: number;
  resolutionOptions = [
    { name: '4K', x: 3840, y: 3552 }, { name: '2K', x: 2880, y: 2664 }
  ];
  disableEyeOptions = [
    { name: $localize`None`, value: 0 }, { name: $localize`Left`, value: 1 }, 
    { name: $localize`Right`, value: 2 }, { name: $localize`Both`, value: 3 }
  ];
  distortionMeshResolutionOptions = [63, 127, 255];
  distortionMeshResolutionOptionsText: { [key: number]: string } = {
    63: $localize`Low (${63})`,
    127: $localize`Normal (${127})`,
    255: $localize`High (${255})`
  }
  constructor() {
    super()
    effect(() => {
      this.dss.values()
      this.resolutionModel = this.settings?.resolutionX
    })
  }
 
  resetResolution() {
    this.resolutionModel = this.defaults?.resolutionX;
    this.setResolution();
  }
  setResolution() {
    if (this.settings && this.resolutionModel) {
      const res = this.resolutionOptions.find(x => x.x == this.resolutionModel)
      if (res) {
        this.settings.resolutionX = res.x;
        this.settings.resolutionY = res.y;
        this.saveConfigSettings();
      }
    }
  }
  scaledMaxFOV(originalFOV: number){
    return Math.ceil(originalFOV / (this.settings?.fovZoom || 1))
  }
}
