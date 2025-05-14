import { Injectable } from '@angular/core';
import { readDir, readTextFile, create, remove, exists, rename, watch, DirEntry } from '@tauri-apps/plugin-fs';
import { appDataDir, join } from '@tauri-apps/api/path';
import { Config } from './JsonFileDefines';
import { Subject, throttleTime } from 'rxjs';
import { debouncedFileWriter, delay } from '../helpers';
import { signal } from '@angular/core';
@Injectable({
  providedIn: 'root'
})
export class DriverSettingService {
  private saveSbj = new Subject<Config>();
  private readonly _setting = signal<Config | undefined>(undefined);
  private readonly _distortionProfileList = signal<DirEntry[]>([]);
  public readonly settings = this._setting.asReadonly();
  public readonly distortionProfileList = this._distortionProfileList.asReadonly();
  private readonly settingPath = this.getDriverAppDirPath('settings.json');
  private readonly distortionDirPath = this.getDriverAppDirPath('Distortion');
  private readonly debouncedFileWriter = debouncedFileWriter(this.settingPath, this.getDriverAppDirPath())
  constructor() {
    this.saveSbj.pipe(throttleTime(50, undefined, { trailing: true })).subscribe((setting) => {
      this.saveSettingInternal(setting)
    });
    this.init();
  }
  private async init() {
    await this.listDistortionProfiles();
    await this.loadSetting();
    watch(await this.settingPath, (ev) => {
      if (!this.debouncedFileWriter.isSavingFile && typeof ev.type === 'object') {
        if ('modify' in ev.type && ev.type.modify.kind == 'any') {
          this.loadSetting();
        }
      }
    }, { delayMs: 20 })
    watch(await this.distortionDirPath, (ev) => {
      if (ev.paths.some(x => x.match(/\.json$/))) {
        this.listDistortionProfiles();
      }
    }, { delayMs: 20 });
  }
  async getDriverAppDirPath(rel?: string) {
    const seg = [await appDataDir(), '../CustomHeadset']
    if (rel) {
      seg.push(rel)
    }
    return await join(...seg);
  }
  private async listDistortionProfiles() {
    const list = (await readDir(await this.distortionDirPath))
      .filter(x => x.isFile && x.name.endsWith('.json'))
    this._distortionProfileList.set(list);
  }
  private async loadSetting() {
    const path = await this.settingPath;
    if (await exists(path)) {
      const current = JSON.parse(this.cleanJsonComments(await readTextFile(path))) as Config;
      this._setting.set(current);
    } else {
      this._setting.set(undefined)
    }
  }


  async saveSetting(settings: Config) {
    this.saveSbj.next(settings);
  }
  private async saveSettingInternal(settings: Config) {
    this.debouncedFileWriter.save(JSON.stringify(settings, undefined, 4))
  }

  private cleanJsonComments(jsonString: string) {
    return jsonString.replace(/\\"|"(?:\\"|[^"])*"|(\/\/.*|\/\*[\s\S]*?\*\/)/g, (m, g) => g ? "" : m)
  }
}
