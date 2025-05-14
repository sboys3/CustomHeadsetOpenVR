import { Injectable, signal } from '@angular/core';
import { AppSetting } from './JsonFileDefines';
import { appDataDir,  join } from '@tauri-apps/api/path';
import { exists, mkdir, readTextFile, writeTextFile } from '@tauri-apps/plugin-fs';
import { DebouncedFileWriter, debouncedFileWriter } from '../helpers';
import { PathService } from './path.service';

@Injectable({
  providedIn: 'root'
})
export class AppSettingService {
  private defaults: AppSetting = {
    colorScheme: 'system',
    updateMode: 'rewrite'
  }
  private _values = signal<AppSetting | undefined>(undefined);
  public values = this._values.asReadonly();
  private folder: Promise<string>;
  private filePath: Promise<string>;
  private debouncedFileWriter: DebouncedFileWriter;
  private initTask = this.init()
  constructor(ps:PathService) {
    this.folder = ps.getDriverAppDirPath();
    this.filePath = (async () => await join(await ps.getDriverAppDirPath(), 'gui-settings.json'))();
    this.debouncedFileWriter = debouncedFileWriter(this.filePath, this.folder);
  }
  async init() {
    if (!await exists(await appDataDir())) {
      await mkdir(await appDataDir());
    }
    if (!await exists(await this.filePath)) {
      await writeTextFile(await this.filePath, JSON.stringify(this.defaults, undefined, 4))
    }
    this._values.set(Object.assign(Object.assign({}, this.defaults), JSON.parse(await readTextFile(await this.filePath))));
  }
  async save(values: AppSetting) {
    await this.initTask;
    this.debouncedFileWriter.save(JSON.stringify(values, undefined, 4));
    this._values.set(Object.assign({}, values))
  }
}
