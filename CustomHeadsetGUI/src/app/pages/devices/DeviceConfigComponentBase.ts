import { Component, computed, effect, HostBinding, inject, input, model, OnDestroy, signal } from "@angular/core";
import { AppSettingService } from "../../services/app-setting.service";
import { DriverInfoService } from "../../services/driver-info.service";
import { DistortionProfileEntry, DriverSettingService } from "../../services/driver-setting.service";
import { SystemDiagnosticService } from "../../services/system-diagnostic.service";
import { PullingService } from "../../services/PullingService";
import { is_vrmonitor_running } from "../../tauri_wrapper";
import { DriverInfo, Settings } from "../../services/JsonFileDefines";
@Component({
    selector: 'device-config-componentBase',
    template: ``
})
export abstract class DeviceConfigComponentBase<T extends { enable: boolean }> implements OnDestroy {
    public settingField = input.required<string>()
    public driverName = input.required<string>()
    public defaults?: T
    public settings?: T
    public rootSetting?: Settings;
    public dss = inject(DriverSettingService)
    public dis = inject(DriverInfoService)
    public ass = inject(AppSettingService)
    public sds = inject(SystemDiagnosticService)
    resolutionInfoDisplay = signal(false)
    profiles: DistortionProfileEntry[] = []
    advanceMode = computed(() => this.ass.values()?.advanceMode ?? false)
    driverWarning = signal(false)
    driverEnablePrompt = signal(false)
    steamVRRunning = signal({ updated: false, running: false }, { equal: (a, b) => a.running === b.running && a.updated === b.updated })
    steamVRStatePulling = new PullingService(async () => {
        this.steamVRRunning.set({ updated: true, running: await is_vrmonitor_running() })
    }, 'steamVRStatePulling');
    @HostBinding('class.page-disabled') get pageDisabled() { return !this.settings?.enable }
    constructor() {
        effect(() => {
            this.rootSetting = this.dss.values()
            this.settings = (this.rootSetting as any)?.[this.settingField()];
        });
        effect(() => {
            const info = (this.dis.values() ?? {}) as DriverInfo;
            const defaultProfiles = info?.builtInDistortionProfiles ?? {};
            this.defaults = (info?.defaultSettings as any)?.[this.settingField()];
            this.profiles = [
                ...Object.keys(defaultProfiles).map(name => ({ name, isDefault: true })),
                ...this.dss.distortionProfileList().map(f => ({
                    name: f.name.split('.').slice(0, -1).join('.'),
                    isDefault: false,
                    file: f
                }))]
        });

        effect(() => {
            const steamVrConfig = this.sds.steamVrConfig();
            const config = this.dss.values();
            const driverName = this.driverName();
            if (steamVrConfig) {
                let shiftallEnabled = this.sds.getSteamVRDriverEnableState(steamVrConfig, driverName)
                let customEnabled = this.sds.getSteamVRDriverEnableState(steamVrConfig, 'CustomHeadsetOpenVR')
                this.driverWarning.set((shiftallEnabled || !customEnabled) && (config?.meganeX8K?.enable ?? false));
                this.driverEnablePrompt.set(!shiftallEnabled && !(config?.meganeX8K?.enable ?? true));
            }
        })
        effect(() => {
            this.steamVRRunning.set({ running: false, updated: false })
            if (this.resolutionInfoDisplay()) {
                this.steamVRStatePulling.start()
            } else {
                this.steamVRStatePulling.stop()
            }
        })
    }
    resetOption(key: keyof T) {
        if (this.settings && this.defaults) {
            (this.settings as any)[key] = this.defaults?.[key];
            this.saveConfigSettings()
        }
    }
    toggleResolutionDisplay() {
        this.resolutionInfoDisplay.update(x => !x)
    }
    async disableDriver() {
        await this.sds.disableSteamVRDriver(this.driverName());
        await this.sds.enableSteamVRDriver('CustomHeadsetOpenVR');
    }
    async enableDriver() {
        await this.sds.enableSteamVRDriver(this.driverName());
    }


    saveConfigSettings() {
        if (this.rootSetting) {
            this.dss.save(this.rootSetting);
        }
    }
    checkShiftallDriverState() {

    }
    loading = false;
    ngOnDestroy(): void {
        this.steamVRStatePulling.stop()
    }
}