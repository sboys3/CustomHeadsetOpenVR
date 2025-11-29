import { computed, effect, Injectable, OnDestroy, signal } from '@angular/core';
import { copyFile, remove, exists, mkdir, readDir, readTextFile, watchImmediate, writeTextFile } from '@tauri-apps/plugin-fs';
import { appLocalDataDir, join } from '@tauri-apps/api/path';
import { DriverSettingService } from './driver-setting.service';
import { DriverInfoService } from './driver-info.service';
import { debounceTime, Subject } from 'rxjs';
import { get_executable_path, restart_vrcompositor } from '../tauri_wrapper';
import { open } from '@tauri-apps/plugin-dialog';
import { DialogService } from './dialog.service';
import { PullingService } from './PullingService';
import { cleanJsonComments } from '../helpers';

@Injectable({providedIn: "root", })
export class SystemDiagnosticService implements OnDestroy {
  private _installingDriver = signal(false)
  public readonly installingDriver = this._installingDriver.asReadonly()
  private _steamVRinstalled = signal<string | undefined>(undefined);
  public readonly steamVRinstalled = this._steamVRinstalled.asReadonly();
  private _driverInstalled = signal<string | undefined>(undefined);
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
  public readonly pullingSteamVRinstall = new PullingService(() => this.checkSteamVrInstalled(), 'pullingSteamVRinstallk');
  public readonly PullingDriverinstall = new PullingService(() => this.checkDriverInstalled(), 'PullingDriverinstall');
  constructor(public dss: DriverSettingService, public dis: DriverInfoService, private dialog: DialogService) {
    let readySetup = false
    effect(() => {
      this.watchSteamVRSettings();
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
  async watchSteamVRSettings(){
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
      subject.next()
    }
  }
  async readySetup() {
    
  }
  ngOnDestroy(): void {
    for (const cfn of this.cleanUp) {
      cfn()
    }
  }

  public async checkDriverInstalled() {
    const steamVrPath = await this.checkSteamVrInstalled();
    if (steamVrPath) {
      const driverPath = await join(steamVrPath, 'drivers', 'CustomHeadsetOpenVR', 'driver.vrdrivermanifest')
      if (await exists(driverPath)) {
        let version = '0.0.0'
        try {
          const obj = JSON.parse(cleanJsonComments(await readTextFile(driverPath)))
          if (obj['version']) {
            version = obj['version']
          }
        } catch (ex) {
          console.warn('read version failed')
        }
        let infoDriverVersion = this.dis.values()?.driverVersion
        if (version === '0.0.0' && infoDriverVersion) {
          version = infoDriverVersion
        }
        this._driverInstalled.set(version);
        this.PullingDriverinstall.stop()
        return true;
      }
    }
    this._driverInstalled.set(undefined);
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
  public async enableSteamVRDriver(driverName: string) {
    await this.updateSteamVRSettings(settings => {
      const name = this.getDriverFieldName(driverName);
      if (!settings[name]) {
        settings[name] = {}
      }
      settings[name]['enable'] = true
      delete settings[name]['blocked_by_safe_mode']
      return true;
    })
  }
  public getSteamVRDriverEnableState(settings: any, driverName: string) {
    if (settings) {
      const driverSetting = settings[this.getDriverFieldName(driverName)]
      if (driverSetting) {
        return (driverSetting['enable'] ?? true) && !(driverSetting['blocked_by_safe_mode'] ?? false);
      }
    }
    return true;
  }
  private getDriverFieldName(driverName: string) {
    return `driver_${driverName}`;
  }
  private installing = false
  async installDriver() {
    if (this.installing) return false;
    const steamVrPath = this.steamVRinstalled();
    if (!steamVrPath) return false;
    this._installingDriver.set(true);
    this.installing = true
    try {
      let driverDir = await join(await get_executable_path(), '../CustomHeadsetOpenVR');
      if (!await exists(driverDir)) {
        if (await this.dialog.confirm($localize`Driver folder not found`, $localize`The driver folder is not in the default location. Unpack the entire zip and retry, or manually locate the new CustomHeadsetOpenVR folder to be installed.`, $localize`Locate`, 'primary')) {
          const path = await open({ directory: true, multiple: false })
          if (path) {
            driverDir = path;
          } else {
            return false;
          }
        } else {
          return false;
        }
      }
      if (await exists(await join(driverDir, 'driver.vrdrivermanifest'))) {
        const steamVrDriverDir = await join(steamVrPath, 'drivers');
        if (!await exists(steamVrDriverDir)) {
          await mkdir(steamVrDriverDir)
        }
        const driverPath = await join(steamVrDriverDir, 'CustomHeadsetOpenVR');
        try {
          await this.copyRec(driverPath, driverDir)
        } catch (e) {
          await this.dialog.message($localize`Install Failed, Make sure SteamVR is closed`, `${e}`)
          return false
        }
        this.checkDriverInstalled()
        return true;
      } else {
        await this.dialog.message($localize`Driver files not valid`, $localize`the folder seems not include driver file, please check again`)
        return false;
      }
    } finally {
      if(!this.dss.values() || this.dss.values()?.meganeX8K?.enable){
        await this.disableSteamVRDriver('MeganeXSuperlight');
      }
      await this.enableSteamVRDriver('CustomHeadsetOpenVR');
      this.installing = false
      this._installingDriver.set(false)
    }
  }
  async uninstallDriver() {
    const steamVrPath = this.steamVRinstalled();
    if (!steamVrPath) return false;
    const driverPath = await join(steamVrPath, 'drivers', 'CustomHeadsetOpenVR');
    if (await exists(driverPath)) {
      try{
        await remove(driverPath, {recursive: true});
      } catch(e){
        await this.dialog.message($localize`Uninstall Failed, Make sure SteamVR is closed`, `${e}`)
        return false;
      }
      this.checkDriverInstalled()
    }
    return true;
  }
  private async copyRec(targetDir: string, sourceDir: string) {
    if (!await exists(targetDir)) {
      await mkdir(targetDir)
    }
    const content = await readDir(sourceDir);
    for (const e of content) {
      if (e.isFile) {
        await copyFile(await join(sourceDir, e.name), await join(targetDir, e.name));
      } else if (e.isDirectory) {
        await this.copyRec(await join(targetDir, e.name), await join(sourceDir, e.name));
      }
    }
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
          this.pullingSteamVRinstall.stop()
          return steamVrPath;
        }
      }
    }
    this._steamVRinstalled.set(undefined);
    return undefined;
  }
  public async resetDriverSetting() {
    await writeTextFile(this.dss.filePath, "{}")
  }
  public async restartCompositor() {
    return await restart_vrcompositor()
  }
}
