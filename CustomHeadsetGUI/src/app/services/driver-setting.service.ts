import { computed, Injectable } from '@angular/core';
import { readDir, readTextFile, exists, DirEntry, size, copyFile, remove, watchImmediate } from '@tauri-apps/plugin-fs';
import { basename, join, } from '@tauri-apps/api/path';
import { Settings } from './JsonFileDefines';
import { debounceTime, Subject } from 'rxjs';
import { signal } from '@angular/core';
import { PathsService } from './paths.service';;
import { JsonSettingServiceBase } from './JsonSettingServiceBase';
import { cleanJsonComments } from '../helpers';
import { DriverInfoService } from './driver-info.service';
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
  constructor(private pathService: PathsService, driverInfoService: DriverInfoService) {
    super(pathService.settingPath, pathService.appDataDirPath, computed(() => driverInfoService.values()?.defaultSettings ?? ({} as Settings)), false, true);
  }
  protected override async init() {
    await super.init();
    await this.listDistortionProfiles();
    const distrotionProfileSbj = new Subject<void>();
    distrotionProfileSbj.pipe(debounceTime(20)).subscribe(() => {
      this.listDistortionProfiles();
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
          const dpObj = JSON.parse(cleanJsonComments(json));
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
