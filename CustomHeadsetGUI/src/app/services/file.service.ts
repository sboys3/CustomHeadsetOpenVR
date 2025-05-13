import { Injectable } from '@angular/core';
import { readDir, readTextFile, create } from '@tauri-apps/plugin-fs';
import { appDataDir, join } from '@tauri-apps/api/path';
import { Config } from './JsonFileDefines';
import { Subject, throttleTime } from 'rxjs';
@Injectable({
  providedIn: 'root'
})
export class FileService {
  private saveSbj = new Subject<Config>();
  constructor() {
    this.saveSbj.pipe(throttleTime(100, undefined, { trailing: true })).subscribe((setting) => {
      this.saveSettingInternal(setting)
    })
  }
  async getDriverAppDirPath(rel: string) {
    return await join(await appDataDir(), '../CustomHeadset', rel);
  }
  async listDistortionProfiles() {
    return (await readDir(await this.getDriverAppDirPath('Distortion')))
      .filter(x => x.isFile && x.name.endsWith('.json'))
  }
  async loadSetting() {
    return JSON.parse(this.cleanJsonComments(await readTextFile(await this.getSettingPath()))) as Config;
  }
  private async getSettingPath() {
    return await this.getDriverAppDirPath('settings.json');
  }

  async saveSetting(settings: Config) {
    this.saveSbj.next(settings);
  }
  private async saveSettingInternal(settings: Config) {
    const json = JSON.stringify(settings, undefined, 4);
    const file = await create(await this.getSettingPath());
    await file.write(new TextEncoder().encode(json));
    await file.close();
  }

  private cleanJsonComments(jsonString: string) {
    return jsonString.replace(/\\"|"(?:\\"|[^"])*"|(\/\/.*|\/\*[\s\S]*?\*\/)/g, (m, g) => g ? "" : m)
  }
}
