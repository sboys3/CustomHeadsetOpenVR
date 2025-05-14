import { Injectable } from '@angular/core';
import { readDir, readTextFile, exists, watch, DirEntry, mkdir, size, copyFile, remove } from '@tauri-apps/plugin-fs';
import { appDataDir, basename, join, } from '@tauri-apps/api/path';
import { Config } from './JsonFileDefines';
import { Subject, throttleTime } from 'rxjs';
import { debouncedFileWriter } from '../helpers';
import { signal } from '@angular/core';
export type DistortionProfileEntry = {
  name: string;
  isDefault: boolean;
  file?: DirEntry;
};
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
  private cleanJsonComments(jsonString: string) {
    return jsonString.replace(/\\"|"(?:\\"|[^"])*"|(\/\/.*|\/\*[\s\S]*?\*\/)/g, (m, g) => g ? "" : m)
  }
  private async saveSettingInternal(settings: Config) {
    this.debouncedFileWriter.save(JSON.stringify(settings, undefined, 4))
  }
  async delete(name: string) {
    const path = await this.getProfileFullPath(name);
    if (await exists(path)) {
      await remove(path);
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
    return await join(await this.distortionDirPath, name);
  }
  async importFile(paths: string[]) {
    if (!await exists(await this.distortionDirPath)) {
      await mkdir(await this.distortionDirPath);
    }
    const message: { [path: string]: { success: boolean, message?: string } } = {}
    for (let path of paths) {
      try {
        const l = await size(path);
        if (l > 5000) {
          message[path] = { success: false, message: $localize`file to large` }
          continue;
        }
        const json = await readTextFile(path);
        try {
          const dpObj = JSON.parse(this.cleanJsonComments(json));
          if (!('type' in dpObj) || !('distortions' in dpObj)) {
            message[path] = { success: false, message: $localize`import file not valid, missing "type" or "distortions"?` }
            continue;
          }
        } catch (jex) {
          message[path] = { success: false, message: $localize`import file not valid ${jex}` }
        }
        await copyFile(path, await join(await this.distortionDirPath, await basename(path)));
        message[path] = { success: true }
      } catch (ex) {
        message[path] = { success: false, message: $localize`import file error ${ex}` }
      }
    }
    return message;
  }


  async saveSetting(settings: Config) {
    this.saveSbj.next(settings);
  }



}
