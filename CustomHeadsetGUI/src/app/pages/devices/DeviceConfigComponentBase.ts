import { ChangeDetectorRef, Component, computed, effect, HostBinding, inject, input, model, OnDestroy, output, signal } from "@angular/core";
import { AppSettingService } from "../../services/app-setting.service";
import { DriverInfoService } from "../../services/driver-info.service";
import { DistortionProfileEntry, DriverSettingService } from "../../services/driver-setting.service";
import { SystemDiagnosticService } from "../../services/system-diagnostic.service";
import { PullingService } from "../../services/PullingService";
import { PathsService } from "../../services/paths.service";
import { stat } from "@tauri-apps/plugin-fs";
import { DriverInfo, HeadsetType, Settings } from "../../services/JsonFileDefines";
import { deepCopy } from "../../helpers";
import { customHeadsetDriverName } from '../../../environment';

export const DistortionProfileDisplayNames: Map<string, string> = new Map([
    ["MeganeX8K Default", "MeganeX8K Custom Default"],
    ["Dream Air Default", "Dream Air Custom Default"],
    ["Dream Air SE Default", "Dream Air SE Custom Default"],
    ["Crystal Super Micro-OLED Default", "Crystal Super Micro-OLED Custom Default"],
    ["Pimax 8KX Default", "Pimax 8KX Custom Default"],
]);

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
    public ps = inject(PathsService)
    // private cdr = inject(ChangeDetectorRef)
    public Math = Math
    resolutionInfoDisplay = signal(false)
    profiles: DistortionProfileEntry[] = []
    displayNames = DistortionProfileDisplayNames;
    advanceMode = computed(() => this.ass.values()?.advanceMode ?? false)
    customShaderOtherWarning = computed(() => {
        const info = this.dis.values();
        const settings = this.dss.values();
        // Not a headset with explicit toggles
        const isOtherHeadset = info?.connectedHeadset == HeadsetType.Other || info?.connectedHeadset == HeadsetType.Vive;
        const noDirectMode = !info?.nonNativeHeadsetFound;
        const customShaderEnabled = settings?.customShader?.enableForOther ?? false;
        return isOtherHeadset && noDirectMode && !customShaderEnabled;
    })
    driverWarning = signal(false)
    driverEnablePrompt = signal(false)
    steamVRRunning = signal({ updated: false, running: false }, { equal: (a, b) => a.running === b.running && a.updated === b.updated })
    private static _diagnosticPath: string | null = null;
    static steamVRStatePulling = new PullingService(async () => {
        try {
            const diagnosticPath = DeviceConfigComponentBase._diagnosticPath;
            if(!diagnosticPath){
                return false
            }
            const stats = await stat(diagnosticPath);
            const modifiedTime = stats.mtime;
            if (!modifiedTime) {
                return false;
            }
            const fourSecondsAgo = new Date(Date.now() - 4000);
            return modifiedTime >= fourSecondsAgo;
        } catch {
            return false;
        }
    }, 'steamVRStatePulling').shared();
    pullingRef = DeviceConfigComponentBase.steamVRStatePulling.createRef(value => {
        this.steamVRRunning.set({ updated: true, running: value })
    })
    @HostBinding('class.page-disabled') get pageDisabled() { return this.settingField() && !this.settings?.enable }
    constructor() {
        if (!DeviceConfigComponentBase._diagnosticPath) {
            DeviceConfigComponentBase._diagnosticPath = this.ps.diagnosticPath;
        }
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
            // Force re-render when settings change
            // this.cdr.markForCheck()
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
            const headsetDeviceType = (this.settings as any)?.distortionProfileDeviceType as string | undefined;
            const showIncompatible = this.ass.values()?.showIncompatibleProfiles ?? false;
            
            const defaultProfileEntries = Object.keys(defaultProfiles).map(name => ({
                name,
                displayName: "",
                isDefault: true,
                device: (defaultProfiles as any)[name]?.device as string | string[] | undefined
            }));
            const fileProfileEntries = this.dss.distortionProfileList().map(f => ({
                name: f.name.split('.').slice(0, -1).join('.'),
                displayName: "",
                isDefault: false,
                file: f.entry,
                device: f.device as string | string[] | undefined
            }));
            
            let allProfiles = [...defaultProfileEntries, ...fileProfileEntries];
            
            // Helper to check if a profile matches a device type (handles both string and string[])
            const deviceMatches = (device: string | string[] | undefined, headsetType: string): boolean => {
                if (!device) return false;
                if (Array.isArray(device)) return device.includes(headsetType);
                return device === headsetType;
            };
            
            // Get the default and currently selected profile names (always show these)
            const defaultProfileName = (this.defaults as any)?.distortionProfile as string | undefined;
            const selectedProfileName = (this.settings as any)?.distortionProfile as string | undefined;
            const alwaysShowNames = new Set<string>();
            if (defaultProfileName) alwaysShowNames.add(defaultProfileName);
            if (selectedProfileName) alwaysShowNames.add(selectedProfileName);
            
            // Filter/sort profiles based on device compatibility
            if (headsetDeviceType) {
                const matching = allProfiles.filter(p => deviceMatches(p.device, headsetDeviceType));
                const noDevice = allProfiles.filter(p => !p.device);
                const alwaysShownIncompatible = allProfiles.filter(p => p.device && !deviceMatches(p.device, headsetDeviceType) && alwaysShowNames.has(p.name));
                const incompatible = allProfiles.filter(p => p.device && !deviceMatches(p.device, headsetDeviceType) && !alwaysShowNames.has(p.name));
                allProfiles = showIncompatible ? [...matching, ...noDevice, ...alwaysShownIncompatible, ...incompatible] : [...matching, ...noDevice, ...alwaysShownIncompatible];
            }
            
            this.profiles = allProfiles;
            
            this.profiles = this.profiles.map((profile) => ({...profile, displayName: this.displayNames.get(profile.name) || profile.name}))
            
            // Always show the default distortion profile at the top of the drop-down
            if (defaultProfileName) {
                const defaultIdx = this.profiles.findIndex(p => p.name === defaultProfileName);
                if (defaultIdx > 0) {
                    const [defaultProfile] = this.profiles.splice(defaultIdx, 1);
                    this.profiles.unshift(defaultProfile);
                }
            }
            
            if(info?.defaultSettings?.customShader?.saturation && this.rootSetting && this.rootSetting?.customShader?.chroma != info?.defaultSettings?.customShader?.chroma && (this?.rootSetting?.customShader?.saturation == undefined || this.rootSetting?.customShader?.saturation == info.defaultSettings?.customShader?.saturation)) {
                // migrate chroma to saturation if chroma is set and saturation is not
                this.rootSetting.customShader.saturation = this.rootSetting.customShader.chroma
                this.rootSetting.customShader.chroma = info?.defaultSettings?.customShader?.chroma
                this.saveConfigSettings()
            }
        });

        effect(() => {
            const steamVrConfig = this.sds.steamVrConfig();
            const config = this.dss.values();
            const driverNames = this.driverNames();
            if (steamVrConfig) {
                let shiftallEnabled = driverNames.map(name => this.sds.getSteamVRDriverEnableState(steamVrConfig, name));
                let customEnabled = this.sds.getSteamVRDriverEnableState(steamVrConfig, customHeadsetDriverName)
                this.driverWarning.set((shiftallEnabled.some(x => x) || !customEnabled) && (config?.meganeX8K?.enable ?? false));
                this.driverEnablePrompt.set(shiftallEnabled.every(x => !x) && !(config?.meganeX8K?.enable ?? true));
            }
        })
        this.pullingRef.start()
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
        await this.sds.enableSteamVRDriver(customHeadsetDriverName);
    }
    async enableDriver() {
        const driverNames = this.driverNames();
        if (driverNames.length >= 4) {
            await this.sds.enableSteamVRDriver(driverNames[0]);
            await this.sds.enableSteamVRDriver(driverNames[1]);
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