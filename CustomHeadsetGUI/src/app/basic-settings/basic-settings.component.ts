import { Component, effect } from '@angular/core';
import { MatSliderModule } from '@angular/material/slider';
import { MatSelectModule } from '@angular/material/select';
import { MatDividerModule } from '@angular/material/divider';
import { MatButtonModule } from '@angular/material/button';
import { MatIconModule } from '@angular/material/icon';
import { MatInputModule } from '@angular/material/input';
import { DistortionProfileEntry, DriverSettingService } from '../services/driver-setting.service';
import { DirEntry } from '@tauri-apps/plugin-fs';
import { CommonModule } from '@angular/common';
import { Config, MeganeX8KConfig } from '../services/JsonFileDefines';
import { FormsModule } from '@angular/forms';



@Component({
    selector: 'app-basic-settings',
    imports: [MatSliderModule, MatSelectModule, FormsModule, MatDividerModule, MatButtonModule, MatIconModule, MatInputModule, CommonModule],
    templateUrl: './basic-settings.component.html',
    styleUrl: './basic-settings.component.scss'
})
export class BasicSettingsComponent {
    profiles: DistortionProfileEntry[] = []
    settings?: MeganeX8KConfig
    config?: Config;
    subpixelShiftOptions = [-0.33, 0, 0.33];
    defaultProfiles = [
        {
            name: 'MeganeX8K Default',
            isDefault: true
        },
        {
            name: 'MeganeX8K Original',
            isDefault: true
        }
    ]
    constructor(private fs: DriverSettingService) {
        effect(() => {
            const config = fs.settings();
            this.config = config;
            this.settings = config?.meganeX8K;
        });
        effect(() => {
            this.profiles = [
                ...this.defaultProfiles,
                ...this.fs.distortionProfileList().map(f => ({
                    name: f.name.split('.').slice(0, -1).join('.'),
                    isDefault: false,
                    file: f
                }))]
        });
    }
    saveConfigSettings() {
        if (this.config) {
            this.fs.saveSetting(this.config);
        }
    }
    loading = false;
}
