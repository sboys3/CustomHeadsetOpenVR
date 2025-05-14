import { Component, effect } from '@angular/core';
import { AppSettingService } from '../services/app-setting.service';
import { AppSetting } from '../services/JsonFileDefines';
import { CommonModule } from '@angular/common';
import { MatSelectModule } from '@angular/material/select';
import { FormsModule } from '@angular/forms';
import { MatDividerModule } from '@angular/material/divider';
import { CdkListboxModule } from '@angular/cdk/listbox';
@Component({
  selector: 'app-app-settings',
  imports: [CommonModule, MatSelectModule, FormsModule, MatDividerModule, CdkListboxModule],
  templateUrl: './app-settings.component.html',
  styleUrl: './app-settings.component.scss'
})
export class AppSettingsComponent {
  settings?: AppSetting;
  constructor(private appSettingService: AppSettingService) {
    effect(() => {
      this.settings = appSettingService.values();
    })
  }
  saveConfigSettings() {
    if (this.settings) {
      this.appSettingService.save(this.settings);
    }
  }
}
