import { Component, computed, effect, signal } from '@angular/core';
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
import { Settings, MeganeX8KConfig, DriverInfo, HiddenAreaMeshConfig } from '../../services/JsonFileDefines';
import { FormsModule } from '@angular/forms';
import { AppSettingService } from '../../services/app-setting.service';
import { DriverInfoService } from '../../services/driver-info.service';
import { ResetButtonComponent } from '../../utilities/reset-button/reset-button.component';
import { SystemReadyComponent } from '../../utilities/system-ready/system-ready.component';
import { SystemDiagnosticService } from '../../services/system-diagnostic.service';



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
        ResetButtonComponent,
        SystemReadyComponent,
        CommonModule],
    providers: [],
    templateUrl: './basic-settings.component.html',
    styleUrl: './basic-settings.component.scss'
})
export class BasicSettingsComponent {
    profiles: DistortionProfileEntry[] = []
    settings?: MeganeX8KConfig
    hiddenArea?: HiddenAreaMeshConfig
    config?: Settings;
    defaults?: MeganeX8KConfig;
    subpixelShiftOptions = [0, 0.33];
    resolutionInfoDisplay = signal(false)
    resolutionModel?: number;
    resolutionOptions = [
        { name: '4K', x: 3840, y: 3552 }, { name: '2K', x: 2880, y: 2664 }
    ]
    distortionMeshResolutionOptions = [63, 127, 255];
    distortionMeshResolutionOptionsText: { [key: number]: string } = {
        63: $localize`Low (${63})`,
        127: $localize`Normal (${127})`,
        255: $localize`High (${255})`
    }
    advanceMode = computed(() => this.ass.values()?.advanceMode ?? false)
    driverWarning = signal(false)
    driverEnablePrompt = signal(false)
    constructor(public dss: DriverSettingService, public dis: DriverInfoService, private ass: AppSettingService, public sds: SystemDiagnosticService) {
        effect(() => {
            const config = dss.values();
            this.config = config;
            this.settings = config?.meganeX8K;
            this.hiddenArea = config?.meganeX8K?.hiddenArea
            this.resolutionModel = this.settings?.resolutionX;
        });

        effect(() => {
            const info = (dis.values() ?? {}) as DriverInfo;
            const defaultProfiles = info?.builtInDistortionProfiles ?? {};
            this.defaults = info?.defaultSettings?.meganeX8K;
            this.profiles = [
                ...Object.keys(defaultProfiles).map(name => ({ name, isDefault: true })),
                ...this.dss.distortionProfileList().map(f => ({
                    name: f.name.split('.').slice(0, -1).join('.'),
                    isDefault: false,
                    file: f
                }))]
        });

        effect(() => {
            const steamVrConfig = sds.steamVrConfig();
            const config = dss.values();
            if (steamVrConfig) {
                let shiftallEnabled = sds.getSteamVRDriverEnableState(steamVrConfig, 'MeganeXSuperlight')
                let customEnabled = sds.getSteamVRDriverEnableState(steamVrConfig, 'CustomHeadsetOpenVR')
                this.driverWarning.set((shiftallEnabled || !customEnabled) && (config?.meganeX8K?.enable ?? false));
                this.driverEnablePrompt.set(!shiftallEnabled && !(config?.meganeX8K?.enable ?? true));
            }
        })
    }
    toggleResolutionDisplay() {
        this.resolutionInfoDisplay.update(x => !x)
    }
    async disableDriver() {
        await this.sds.disableSteamVRDriver('MeganeXSuperlight');
        await this.sds.enableSteamVRDriver('CustomHeadsetOpenVR');
    }
    async enableDriver() {
        await this.sds.enableSteamVRDriver('MeganeXSuperlight');
    }
    resetOption(key: keyof MeganeX8KConfig) {
        if (this.settings && this.defaults) {
            (this.settings as any)[key] = this.defaults[key];
            this.saveConfigSettings()
        }
    }
    resetHiddenAreaOption(key: keyof HiddenAreaMeshConfig) {
        if (this.hiddenArea && this.defaults && this.defaults.hiddenArea) {
            (this.hiddenArea as any)[key] = this.defaults.hiddenArea[key];
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
    checkShiftallDriverState() {

    }
    loading = false;
}
