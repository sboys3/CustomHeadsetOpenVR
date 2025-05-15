import { Injectable } from '@angular/core';
import { appDataDir, join } from '@tauri-apps/api/path';
import { exists, mkdir } from '@tauri-apps/plugin-fs';

@Injectable({
  providedIn: 'root'
})
export class PathService {

  private _distortionDirPath!: string;
  public get distortionDirPath(): string {
    return this._distortionDirPath;
  }
  private _appDataDirPath!: string;
  public get appDataDirPath(): string {
    return this._appDataDirPath;
  }
  async ensureAllDirCreated() {
    this._appDataDirPath = await this.getDriverAppDirPath();
    this._distortionDirPath = await this.getDriverAppDirPath('Distortion');
    if (!await exists(this.appDataDirPath)) {
      await mkdir(this.appDataDirPath);
    }
    if (!await exists(this.distortionDirPath)) {
      await mkdir(this.distortionDirPath)
    }
  }
  async getDriverAppDirPath(rel?: string) {
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
