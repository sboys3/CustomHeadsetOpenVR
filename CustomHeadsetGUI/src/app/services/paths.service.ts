import { Injectable } from '@angular/core';
import { appDataDir, join } from '@tauri-apps/api/path';
import { exists, mkdir } from '@tauri-apps/plugin-fs';

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
  async ensureAllDirCreated() {
    this._appDataDirPath = await this.getDriverAppDirPath();
    this._distortionDirPath = await this.getDriverAppDirPath('Distortion');
    this._infoPath = await this.getDriverAppDirPath('info.json');
    this._guiSettingPath = await this.getDriverAppDirPath('gui-settings.json');
    this._settingPath = await this.getDriverAppDirPath('settings.json');
    if (!await exists(this.appDataDirPath)) {
      await mkdir(this.appDataDirPath);
    }
    if (!await exists(this.distortionDirPath)) {
      await mkdir(this.distortionDirPath)
    }
  }
  private async getDriverAppDirPath(rel?: string) {
    const seg = [await appDataDir(), '../CustomHeadset']
    if (rel) {
      seg.push(rel)
    }
    return await join(...seg);
  }
  async getProfileFullPath(name: string) {
    return await join(this.distortionDirPath, name);
  }

}
