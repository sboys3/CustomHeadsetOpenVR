import { Injectable, signal } from '@angular/core';
import { AppSetting } from './JsonFileDefines';
import { appDataDir, BaseDirectory, join } from '@tauri-apps/api/path';
import { exists, mkdir, readTextFile, writeTextFile } from '@tauri-apps/plugin-fs';
import { DebouncedFileWriter, debouncedFileWriter } from '../helpers';
import { DriverSettingService } from './driver-setting.service';

@Injectable({
  providedIn: 'root'
})
export class AppSettingService {
  private defaults: AppSetting = {
    colorScheme: 'system'
  }
  private _values = signal<AppSetting | undefined>(undefined);
  public values = this._values.asReadonly();
  private folder: Promise<string>;
  private filePath: Promise<string>;
  private debouncedFileWriter: DebouncedFileWriter;
  private initTask = this.init()
  constructor(dss: DriverSettingService) {
    this.folder = dss.getDriverAppDirPath();
    this.filePath = (async () => await join(await dss.getDriverAppDirPath(), 'gui-settings.json'))();
    this.debouncedFileWriter = debouncedFileWriter(this.filePath, this.folder);
  }
  async init() {
    if (!await exists(await appDataDir())) {
      await mkdir(await appDataDir());
    }
    if (!await exists(await this.filePath)) {
      await writeTextFile(await this.filePath, JSON.stringify(this.defaults, undefined, 4))
    }
    this._values.set(JSON.parse(await readTextFile(await this.filePath)));
  }
  async save(values: AppSetting) {
    await this.initTask;
    this.debouncedFileWriter.save(JSON.stringify(values, undefined, 4));
    this._values.set(Object.assign({}, values))
  }
}
