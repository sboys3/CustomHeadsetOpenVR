import { computed, effect, Injectable, OnDestroy, signal } from '@angular/core';
import { exists, readTextFile, watchImmediate, writeTextFile } from '@tauri-apps/plugin-fs';
import { appLocalDataDir, join } from '@tauri-apps/api/path';
import { DriverSettingService } from './driver-setting.service';
import { DriverInfoService } from './driver-info.service';
import { debounceTime, Subject } from 'rxjs';

@Injectable()
export class SystemDiagnosticService implements OnDestroy {
  private _steamVRinstalled = signal<string | undefined>(undefined);
  public readonly steamVRinstalled = this._steamVRinstalled.asReadonly();
  private _driverInstalled = signal<boolean>(false);
  public readonly driverInstalled = this._driverInstalled.asReadonly();
  public readonly settingFileInited = computed(() => this.dss.values() && this.dis.values());
  public readonly systemReady = computed(() => this.steamVRinstalled() && this.driverInstalled() && this.settingFileInited())
  private _steamVrConfig = signal<any>(undefined);
  public readonly steamVrConfig = this._steamVrConfig.asReadonly();
  private cleanUp: (() => void)[] = []
  private _initTask: Promise<any>;
  public get initTask() {
    return this._initTask;
  }
  constructor(public dss: DriverSettingService, public dis: DriverInfoService) {
    let readySetup = false
    effect(() => {
      const ready = this.systemReady()
      if (ready && !readySetup) {
        readySetup = true;
        this.readySetup();
      }
    })
    this._initTask = (async () => {
      await dss.initTask;
      await dis.initTask;
      await this.checkDriverInstalled()
    })();
  }
  async readySetup() {
    const subject = new Subject<void>();
    const usub = subject.pipe(debounceTime(50)).subscribe(async () => {
      const value = await this.getSteamVRSettings();
      this._steamVrConfig.set(value)
    })
    this.cleanUp.push(() => {
      usub.unsubscribe();
      subject.complete();
    })
    const path = await this.getSteamVRConfigFilePath();
    if (path) {
      watchImmediate(path, (e) => {
        subject.next()
      });
    }
  }
  ngOnDestroy(): void {
    for (const cfn of this.cleanUp) {
      cfn()
    }
  }
  private pullingSteamVRinstallState = false;
  public startPullingSteamVRinstallState() {
    if (this.pullingSteamVRinstallState) return;
    this.pullingSteamVRinstallState = true
    let interval = setInterval(() => this.checkSteamVrInstalled(), 1000);
    this.cleanUp.push(() => clearInterval(interval))
  }
  private pullingDriverinstallState = false;
  public startPullingDriverinstallState() {
    if (this.pullingDriverinstallState) return;
    this.pullingDriverinstallState = true
    let interval = setInterval(() => this.checkDriverInstalled(), 1000);
    this.cleanUp.push(() => clearInterval(interval))
  }
  public async checkDriverInstalled() {
    const steamVrPath = await this.checkSteamVrInstalled();
    if (steamVrPath) {
      const driverPath = await join(steamVrPath, 'drivers', 'CustomHeadsetOpenVR', 'driver.vrdrivermanifest')
      if (await exists(driverPath)) {
        this._driverInstalled.set(true);
        return true;
      }
    }
    this._driverInstalled.set(false);
    return false;
  }
  public async getOpenvrpaths() {
    const openVrConfigPath = await join(await appLocalDataDir(), '../openvr/openvrpaths.vrpath');
    const vrPathExists = await exists(openVrConfigPath);
    if (vrPathExists) {
      try {
        return JSON.parse(await readTextFile(openVrConfigPath));
      } catch (e) {
        console.error(e);
      }
    }
    return undefined;
  }
  public async disableSteamVRDriver(driverName: string) {
    await this.updateSteamVRSettings(settings => {
      const name = this.getDriverFieldName(driverName);
      if (!settings[name]) {
        settings[name] = {}
      }
      settings[name]['enable'] = false
      return true;
    })
  }
  public getSteamVRDriverEnableState(settings: any, driverName: string) {
    if (settings) {
      const driverSetting = settings[this.getDriverFieldName(driverName)]
      if (driverSetting) {
        return !!driverSetting['enable']
      }
    }
    return true;
  }
  private getDriverFieldName(driverName: string) {
    return `driver_${driverName}`;
  }

  /**
   * 
   * @param update return true to save
   */
  public async updateSteamVRSettings(update: (steamVrSettings: any) => boolean) {
    const settings = await this.getSteamVRSettings();
    if (settings) {
      const path = await this.getSteamVRConfigFilePath();
      if (update(settings) && path) {
        await writeTextFile(path, JSON.stringify(settings, undefined, 4))
      }
    }
  }
  public async getSteamVRSettings() {
    const path = await this.getSteamVRConfigFilePath();
    if (path) {
      return JSON.parse(await readTextFile(path));
    }
  }
  public async getSteamVRConfigFilePath(): Promise<string | undefined> {
    const dirPath = await this.getSteamVRConfigDirPath()
    if (dirPath) {
      return await join(dirPath, 'steamvr.vrsettings')
    }
    return undefined;
  }
  public async getSteamVRConfigDirPath(): Promise<string | undefined> {
    const openvrpaths = await this.getOpenvrpaths();
    if (openvrpaths) {
      const path = openvrpaths?.['config'];
      if (path && typeof path == 'object' && Array.isArray(path)) {
        const configFolderPath = path[0];
        if (configFolderPath) {
          return configFolderPath;
        }
      }
    }
    return undefined;
  }
  public async checkSteamVrInstalled(): Promise<string | undefined> {
    const openvrpaths = await this.getOpenvrpaths();
    const runtime = openvrpaths?.['runtime'];
    if (runtime && typeof runtime == 'object' && Array.isArray(runtime)) {
      const steamVrPath = runtime.find(x => typeof x == 'string' && x.endsWith('SteamVR'));
      if (steamVrPath) {
        if (await exists(await join(steamVrPath, 'bin', 'version.txt'))) {
          this._steamVRinstalled.set(steamVrPath);
          return steamVrPath;
        }
      }
    }
    this._steamVRinstalled.set(undefined);
    return undefined;
  }
}
