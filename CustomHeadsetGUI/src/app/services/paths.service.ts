import { Injectable } from '@angular/core';
import { appDataDir, configDir, join } from '@tauri-apps/api/path';
import { exists, mkdir } from '@tauri-apps/plugin-fs';
import { vendor } from '../../environment';
import { get_platform } from '../tauri_wrapper';

console.log("vendor", vendor || "neutral");

@Injectable({
  providedIn: 'root'
})
export class PathsService {

  private _distortionDirPath!: string;
  public get distortionDirPath(): string {
    return this._distortionDirPath;
  }
  private _appDataDirPath!: string;
  public get appDataDirPath(): string {
    return this._appDataDirPath;
  }
  private _infoPath!: string;
  public get infoPath(): string {
    return this._infoPath;
  }
  private _guiSettingPath!: string;
  public get guiSettingPath(): string {
    return this._guiSettingPath;
  }
  private _settingPath!: string;
  public get settingPath(): string {
    return this._settingPath;
  }
  private _diagnosticPath!: string;
  public get diagnosticPath(): string {
    return this._diagnosticPath;
  }
  async ensureAllDirCreated() {
    this._appDataDirPath = await this.getDriverAppDirPath();
    this._distortionDirPath = await this.getDriverAppDirPath('Distortion');
    this._infoPath = await this.getDriverAppDirPath('info.json');
    this._guiSettingPath = await this.getDriverAppDirPath('gui-settings.json');
    this._settingPath = await this.getDriverAppDirPath('settings.json');
    this._diagnosticPath = await this.getDriverAppDirPath('diagnostic.json');
    if (!await exists(this.appDataDirPath)) {
      await mkdir(this.appDataDirPath);
    }
    if (!await exists(this.distortionDirPath)) {
      await mkdir(this.distortionDirPath)
    }
  }
  private async getDriverAppDirPath(rel?: string) {
    let dataDir = 'CustomHeadset'
    switch(vendor){
      case 'pimax':
        dataDir = 'Pimax/CustomHeadset'
        break;
    }
    let seg = [await appDataDir(), '../' + dataDir]  
    const platform = await get_platform();
    if(platform === 'linux'){
      seg = [await configDir(), dataDir]
    }
    if (rel) {
      seg.push(rel)
    }
    return await join(...seg);
  }
  async getProfileFullPath(name: string) {
    return await join(this.distortionDirPath, name);
  }

}
