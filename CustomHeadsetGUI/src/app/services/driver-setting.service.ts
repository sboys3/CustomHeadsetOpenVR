import { Injectable } from '@angular/core';
import { readDir, readTextFile, exists, DirEntry, size, copyFile, remove, watchImmediate } from '@tauri-apps/plugin-fs';
import { basename, join, } from '@tauri-apps/api/path';
import { DriverInfo, Settings } from './JsonFileDefines';
import { debounceTime, Subject } from 'rxjs';
import { signal } from '@angular/core';
import { PathsService } from './paths.service';;
import { JsonSettingServiceBase } from './JsonSettingServiceBase';
export type DistortionProfileEntry = {
  name: string;
  isDefault: boolean;
  file?: DirEntry;
};
@Injectable({
  providedIn: 'root'
})
export class DriverSettingService extends JsonSettingServiceBase<Settings> {
  private readonly _distortionProfileList = signal<DirEntry[]>([]);
  public readonly distortionProfileList = this._distortionProfileList.asReadonly();
  private readonly _driverInfo = signal<DriverInfo | undefined>(undefined);
  public readonly driverInfo = this._driverInfo.asReadonly();
  constructor(private pathService: PathsService) {
    super(pathService.settingPath, pathService.appDataDirPath, async () => {
      if (await exists(pathService.infoPath)) {
        const info = await readTextFile(pathService.infoPath);
        const value = (JSON.parse(info) ?? {}) as DriverInfo;
        this._driverInfo.set(value);
        return value.defaultSettings;
      }
      console.warn('info.json not found')
      return {} as Settings
    }, false);

  }
  protected override async init() {
    await super.init();
    await this.listDistortionProfiles();
    const settingFileSbj = new Subject<void>();
    settingFileSbj.pipe(debounceTime(20)).subscribe(() => {
      this.loadSetting();
    })
    const distrotionProfileSbj = new Subject<void>();
    distrotionProfileSbj.pipe(debounceTime(20)).subscribe(() => {
      this.listDistortionProfiles();
    })
    watchImmediate(this.pathService.appDataDirPath, (ev) => {
      if (!this.debouncedFileWriter.isSavingFile() && ev.paths.some(x => x == this.pathService.settingPath)) {
        settingFileSbj.next();
      }
    })
    watchImmediate(this.pathService.distortionDirPath, (ev) => {
      if (ev.paths.some(x => x.match(/\.json$/))) {
        distrotionProfileSbj.next();
      }
    });
  }
  private async listDistortionProfiles() {
    const list = (await readDir(this.pathService.distortionDirPath))
      .filter(x => x.isFile && x.name.endsWith('.json'))
    this._distortionProfileList.set(list);
  }

  async delete(name: string) {
    const path = await this.pathService.getProfileFullPath(name);
    if (await exists(path)) {
      await remove(path);
    }
  }

  async importFile(paths: string[]) {
    const message: { [path: string]: { success: boolean, message?: string } } = {}
    for (let path of paths) {
      try {
        const l = await size(path);
        if (l > 5000) {
          message[path] = { success: false, message: $localize`file too large` }
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
        await copyFile(path, await join(this.pathService.distortionDirPath, await basename(path)));
        message[path] = { success: true }
      } catch (ex) {
        message[path] = { success: false, message: $localize`import file error ${ex}` }
      }
    }
    return message;
  }

}
