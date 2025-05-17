import { computed, Injectable, OnDestroy, signal } from '@angular/core';
import { exists, readTextFile } from '@tauri-apps/plugin-fs';
import { appLocalDataDir, join } from '@tauri-apps/api/path';
import { DriverSettingService } from './driver-setting.service';
import { DriverInfoService } from './driver-info.service';

@Injectable()
export class SystemDiagnosticService implements OnDestroy {
  private _steamVRinstalled = signal<string | undefined>(undefined);
  public steamVRinstalled = this._steamVRinstalled.asReadonly();
  private _driverInstalled = signal<boolean>(false);
  public driverInstalled = this._driverInstalled.asReadonly();
  public settingFileInited = computed(() => this.dss.values() && this.dis.values());
  public systemReady = computed(() => this.steamVRinstalled() && this.driverInstalled() && this.settingFileInited())
  private cleanUp: (() => void)[] = []
  private _initTask: Promise<any>;
  public get initTask() {
    return this._initTask;
  }
  constructor(public dss: DriverSettingService, public dis: DriverInfoService) {
    this._initTask = (async () => {
      await dss.initTask;
      await dis.initTask;
      await this.checkDriverInstalled()
    })();
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
  public async checkSteamVrInstalled(): Promise<string | undefined> {
    const openVrConfigPath = await join(await appLocalDataDir(), '../openvr/openvrpaths.vrpath');
    const vrPathExists = await exists(openVrConfigPath);
    if (vrPathExists) {
      try {
        const vrpathObj = JSON.parse(await readTextFile(openVrConfigPath));
        const vrRuntime = vrpathObj['runtime'];
        if (vrRuntime && typeof vrRuntime == 'object' && Array.isArray(vrRuntime)) {
          const steamVrPath = vrRuntime.find(x => typeof x == 'string' && x.endsWith('SteamVR'));
          if (steamVrPath) {
            if (await exists(await join(steamVrPath, 'bin', 'version.txt'))) {
              this._steamVRinstalled.set(steamVrPath);
              return steamVrPath;
            }
          }
        }
      } catch (ex) {
        this._steamVRinstalled.set(undefined);
        console.error(ex);
      }
    }
    this._steamVRinstalled.set(undefined);
    return undefined;
  }
}
