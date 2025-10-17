import { Component, computed, effect, HostBinding, inject, input, model, OnDestroy, output, signal } from "@angular/core";
import { AppSettingService } from "../../services/app-setting.service";
import { DriverInfoService } from "../../services/driver-info.service";
import { DistortionProfileEntry, DriverSettingService } from "../../services/driver-setting.service";
import { SystemDiagnosticService } from "../../services/system-diagnostic.service";
import { PullingService } from "../../services/PullingService";
import { is_vrmonitor_running } from "../../tauri_wrapper";
import { DriverInfo, Settings } from "../../services/JsonFileDefines";
import { deepCopy } from "../../helpers";

type KeysMatching<T, V> = { [K in keyof T]-?: T[K] extends V ? K : never }[keyof T];
type ObjectKeys<T> = KeysMatching<T, Record<PropertyKey, any>>;
@Component({
    selector: 'device-config-componentBase',
    template: ``
})
export abstract class DeviceConfigComponentBase<T extends { enable: boolean }> implements OnDestroy {
    public settingField = input.required<string>()
    public driverName = input.required<string | string[]>()
    protected driverNames = computed(() => {
        const names = this.driverName();
        if (typeof names === 'string') {
            return [names];
        }
        return names;
    })
    public enabled = output<boolean>()
    public defaults?: T
    public settings?: T
    public rootSetting?: Settings;
    public dss = inject(DriverSettingService)
    public dis = inject(DriverInfoService)
    public ass = inject(AppSettingService)
    public sds = inject(SystemDiagnosticService)
    public Math = Math
    resolutionInfoDisplay = signal(false)
    profiles: DistortionProfileEntry[] = []
    advanceMode = computed(() => this.ass.values()?.advanceMode ?? false)
    driverWarning = signal(false)
    driverEnablePrompt = signal(false)
    steamVRRunning = signal({ updated: false, running: false }, { equal: (a, b) => a.running === b.running && a.updated === b.updated })
    static steamVRStatePulling = new PullingService(async () => await is_vrmonitor_running(), 'steamVRStatePulling').shared();
    pullingRef = DeviceConfigComponentBase.steamVRStatePulling.createRef(value => {
        this.steamVRRunning.set({ updated: true, running: value })
    })
    @HostBinding('class.page-disabled') get pageDisabled() { return this.settingField() && !this.settings?.enable }
    constructor() {
        effect(() => {
            this.rootSetting = this.dss.values()
            const field = this.settingField();
            if (field) {
                this.settings = (this.rootSetting as any)?.[field];
            } else {
                //special case : root settings
                this.settings = (this.rootSetting as any)
            }
            this.enabled.emit(this.settings?.enable ?? false)
        });
        effect(() => {
            const info = (this.dis.values() ?? {}) as DriverInfo;
            const defaultProfiles = info?.builtInDistortionProfiles ?? {};
            const field = this.settingField();
            if (field) {
                this.defaults = (info?.defaultSettings as any)?.[this.settingField()];
            } else {
                //special case : root settings
                this.defaults = (info?.defaultSettings as any)
            }
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
            const driverNames = this.driverNames();
            if (steamVrConfig) {
                let shiftallEnabled = driverNames.map(name => this.sds.getSteamVRDriverEnableState(steamVrConfig, name));
                let customEnabled = this.sds.getSteamVRDriverEnableState(steamVrConfig, 'CustomHeadsetOpenVR')
                this.driverWarning.set((shiftallEnabled.some(x => x) || !customEnabled) && (config?.meganeX8K?.enable ?? false));
                this.driverEnablePrompt.set(shiftallEnabled.every(x => !x) && !(config?.meganeX8K?.enable ?? true));
            } else if (steamVrConfig) {
                let customEnabled = this.sds.getSteamVRDriverEnableState(steamVrConfig, 'CustomHeadsetOpenVR');
                this.driverWarning.set(!customEnabled);
                this.driverEnablePrompt.set(false);
            }
        })
        effect(() => {
            this.steamVRRunning.set({ running: false, updated: false })
            if (this.resolutionInfoDisplay()) {
                this.pullingRef.start()
            } else {
                this.pullingRef.stop()
            }
        })
    }
    resetOption(key: keyof T) {
        if (this.settings && this.defaults) {
            if (this.defaults?.[key] && typeof this.defaults[key] === 'object') {
                (this.settings as any)[key] = deepCopy(this.defaults[key]);
            } else {
                (this.settings as any)[key] = this.defaults?.[key];
            }
            this.saveConfigSettings()
        }
    }
    nested<U extends ObjectKeys<T>>(key: U) {
        return this._nested(key, this.settings, this.defaults)
    }
    private _nested<K extends Record<PropertyKey, any>, U extends ObjectKeys<K>>(key: U, obj?: K, defaults?: K) {
        const setting = obj ?? ({} as K);
        const defaultsObj = defaults ?? ({} as K);
        const scope = setting[key];
        const scopeDefault = defaultsObj[key];
        return {
            nested: (nextKey: keyof K[U]) => {
                // @ts-ignore
                return this._nested(nextKey, scope, scopeDefault)
            },
            resetOption: (resetFieldKey: keyof K[U]) => {
                if (scope && scopeDefault) {
                    scope[resetFieldKey] = scopeDefault[resetFieldKey];
                    this.saveConfigSettings();
                }
            }
        }
    }
    toggleResolutionDisplay() {
        this.resolutionInfoDisplay.update(x => !x)
    }
    async disableDriver() {
        for (const name of this.driverNames()) {
            await this.sds.disableSteamVRDriver(name);
        }
        await this.sds.enableSteamVRDriver('CustomHeadsetOpenVR');
    }
    async enableDriver() {
        const driverNames = this.driverNames();
        if (driverNames.length) {
            await this.sds.enableSteamVRDriver(driverNames[0]);
        }
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
        this.pullingRef.stop()
    }
}