import { Component, effect } from '@angular/core';
import { AppSettingService } from '../../services/app-setting.service';
import { AppSetting } from '../../services/JsonFileDefines';
import { CommonModule } from '@angular/common';
import { MatSelectModule } from '@angular/material/select';
import { FormsModule } from '@angular/forms';
import { MatDividerModule } from '@angular/material/divider';
import { MatSlideToggleModule } from '@angular/material/slide-toggle';
import { ResetButtonComponent } from '../../utilities/reset-button/reset-button.component';
@Component({
  selector: 'app-app-settings',
  imports: [CommonModule, MatSelectModule, FormsModule, MatDividerModule, MatSlideToggleModule, ResetButtonComponent],
  templateUrl: './app-settings.component.html',
  styleUrl: './app-settings.component.scss'
})
export class AppSettingsComponent {
  settings?: AppSetting;

  constructor(public appSettingService: AppSettingService) {
    effect(() => {
      this.settings = appSettingService.values();
    })
  }
  saveConfigSettings() {
    if (this.settings) {
      this.appSettingService.save(this.settings);
    }
  }
  resetOption(key: keyof AppSetting) {
    if (this.settings) {
      (this.settings as any)[key] = this.appSettingService.defaults?.[key];
      this.saveConfigSettings();
    }
  }
}
