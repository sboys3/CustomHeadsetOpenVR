import { Component, computed, effect } from '@angular/core';
import { MatSliderModule } from '@angular/material/slider';
import { MatSelectModule } from '@angular/material/select';
import { MatDividerModule } from '@angular/material/divider';
import { MatButtonModule } from '@angular/material/button';
import { MatIconModule } from '@angular/material/icon';
import { MatInputModule } from '@angular/material/input';
import { MatSlideToggleModule } from '@angular/material/slide-toggle'
import { MatTooltipModule } from '@angular/material/tooltip';
import { DistortionProfileEntry, DriverSettingService } from '../../services/driver-setting.service';
import { CommonModule } from '@angular/common';
import { Settings, MeganeX8KConfig, DriverInfo } from '../../services/JsonFileDefines';
import { FormsModule } from '@angular/forms';
import { AppSettingService } from '../../services/app-setting.service';



@Component({
    selector: 'app-basic-settings',
    imports: [MatSliderModule,
        MatSelectModule,
        FormsModule,
        MatDividerModule,
        MatButtonModule,
        MatIconModule,
        MatInputModule,
        MatSlideToggleModule,
        MatTooltipModule,
        CommonModule],
    templateUrl: './basic-settings.component.html',
    styleUrl: './basic-settings.component.scss'
})
export class BasicSettingsComponent {
    profiles: DistortionProfileEntry[] = []
    settings?: MeganeX8KConfig
    config?: Settings;
    defaults?: MeganeX8KConfig;
    subpixelShiftOptions = [0, 0.33];
    resolutionModel?: number;
    resolutionOptions = [
        { name: '4K', x: 3840, y: 3552 }, { name: '2K', x: 2880, y: 2664 }
    ]
    distortionMeshResolutionOptions = [64, 128, 256];
    distortionMeshResolutionOptionsText: { [key: number]: string } = {
        64: $localize`Low (${64})`,
        128: $localize`Normal (${128})`,
        256: $localize`High (${256})`
    }
    advanceMode = computed(() => this.ass.values()?.advanceMode ?? false)

    constructor(private dss: DriverSettingService, private ass: AppSettingService) {
        effect(() => {
            const config = dss.values();
            this.config = config;
            this.settings = config?.meganeX8K;
            this.resolutionModel = this.settings?.resolutionX;
        });

        effect(() => {
            const info = (dss.driverInfo() ?? {}) as DriverInfo;
            const defaultProfiles = info?.builtinDistortionProfiles ?? {};
            this.defaults = info?.defaultSettings?.meganeX8K;
            this.profiles = [
                ...Object.keys(defaultProfiles).map(name => ({ name, isDefault: true })),
                ...this.dss.distortionProfileList().map(f => ({
                    name: f.name.split('.').slice(0, -1).join('.'),
                    isDefault: false,
                    file: f
                }))]
        });
    }
    resetOption(key: keyof MeganeX8KConfig) {
        if (this.settings && this.defaults) {
            (this.settings as any)[key] = this.defaults[key];
            this.saveConfigSettings()
        }
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
    saveConfigSettings() {
        if (this.config) {
            this.dss.save(this.config);
        }
    }
    loading = false;
}
