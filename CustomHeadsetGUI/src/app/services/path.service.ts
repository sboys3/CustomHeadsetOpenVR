import { Injectable } from '@angular/core';
import { appDataDir, join } from '@tauri-apps/api/path';

@Injectable({
  providedIn: 'root'
})
export class PathService {
  public readonly distortionDirPath = this.getDriverAppDirPath('Distortion');
  constructor() { }
   async getDriverAppDirPath(rel?: string) {
    const seg = [await appDataDir(), '../CustomHeadset']
    if (rel) {
      seg.push(rel)
    }
    return await join(...seg);
  }
  async getProfileFullPath(name: string) {
    return await join(await this.distortionDirPath, name);
  }
}
