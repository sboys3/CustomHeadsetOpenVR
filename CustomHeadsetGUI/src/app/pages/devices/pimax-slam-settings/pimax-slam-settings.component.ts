import { Component, input } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { MatSlideToggleModule } from '@angular/material/slide-toggle';
import { MatTooltipModule } from '@angular/material/tooltip';
import { BaseHeadsetConfig } from '../../../services/JsonFileDefines';
import { DeviceConfigComponentBase } from '../DeviceConfigComponentBase';
import { FieldTipComponent } from '../../../utilities/field-tip/field-tip.component';
import { FieldNoteComponent } from '../../../utilities/field-note/field-note.component';
import { ResetButtonComponent } from '../../../utilities/reset-button/reset-button.component';

@Component({
  selector: 'app-pimax-slam-settings',
  imports: [FormsModule, MatSlideToggleModule, MatTooltipModule, FieldTipComponent, FieldNoteComponent, ResetButtonComponent],
  templateUrl: './pimax-slam-settings.component.html',
  styleUrl: './pimax-slam-settings.component.scss'
})
export class PimaxSlamSettingsComponent extends DeviceConfigComponentBase<BaseHeadsetConfig> {
  override settingField = input.required<string>();
  override driverName = input<string | string[]>([]);
}