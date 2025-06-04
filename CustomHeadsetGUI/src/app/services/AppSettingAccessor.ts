import { Signal, signal } from "@angular/core";
import { AppSetting } from "./JsonFileDefines";

/**
 * for read appsettings in anywhere , like in base service ,without circle dependency
 */
export abstract class AppSettingAccessor {
    abstract get settings(): AppSetting;
    abstract get settingChanged(): Signal<AppSetting>;
}
export class AppSettingHolder extends AppSettingAccessor {
    private _settings?: AppSetting
    private _settingChanged = signal<AppSetting>({} as AppSetting)
    private _readOnlySettingChanged = this._settingChanged.asReadonly()
    public get settingChanged() {
        return this._readOnlySettingChanged;
    }
    public set settings(value: AppSetting) {
        this._settings = value
        this._settingChanged.set(value)
    }
    override get settings(): AppSetting {
        return this._settings ?? {} as AppSetting
    }

}