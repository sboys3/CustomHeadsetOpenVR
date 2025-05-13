import { Component } from '@angular/core';
import { MatSliderModule } from '@angular/material/slider';
import { MatSelectModule } from '@angular/material/select';
import { MatDividerModule } from '@angular/material/divider';
import { MatButtonModule } from '@angular/material/button';
import { MatIconModule } from '@angular/material/icon';
import { FileService } from '../services/file.service';
import { DirEntry } from '@tauri-apps/plugin-fs';
import { CommonModule } from '@angular/common';
import { Config, MeganeX8KConfig } from '../services/JsonFileDefines';
import { FormsModule } from '@angular/forms';
@Component({
    selector: 'app-basic-settings',
    imports: [MatSliderModule, MatSelectModule, CommonModule, FormsModule, MatDividerModule, MatButtonModule, MatIconModule],
    templateUrl: './basic-settings.component.html',
    styleUrl: './basic-settings.component.scss'
})
export class BasicSettingsComponent {
    profiles: {
        name: string,
        isDefault: boolean,
        file?: DirEntry
    }[] = []
    settings?: MeganeX8KConfig
    config?: Config;
    constructor(private fs: FileService) {
        this.loadSettings();
        fs.listDistortionProfiles().then(x => {
            this.profiles = [{
                name: 'MeganeX8K Default',
                isDefault: true
            },
            {
                name: 'MeganeX8K Original',
                isDefault: true
            },
            ...x.map(f => ({
                name: f.name.split('.').slice(0, -1).join('.'),
                isDefault: false,
                file: f
            }))]
        })
    }
    update() {
        if (this.config) {
            this.fs.saveSetting(this.config);
        }
    }
    loading = false;
    loadSettings() {
        if (this.loading) return;
        this.loading = true;
        this.fs.loadSetting().then(s => {
            this.config = s;
            this.settings = s.meganeX8K;
            console.log('setting.json loaded', s);
        }).finally(() => {
            this.loading = false;
        });
    }
}
