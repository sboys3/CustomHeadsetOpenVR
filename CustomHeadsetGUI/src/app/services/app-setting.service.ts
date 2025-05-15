import { Injectable, signal } from '@angular/core';
import { AppSetting } from './JsonFileDefines';
import { join } from '@tauri-apps/api/path';
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
  private guiSettingPath: Promise<string>;
  private debouncedFileWriter: DebouncedFileWriter;
  private initTask: Promise<void>;
  constructor(ps: PathService) {
    this.guiSettingPath = (async () => await join(await ps.getDriverAppDirPath(), 'gui-settings.json'))();
    this.debouncedFileWriter = debouncedFileWriter(this.guiSettingPath, ps.appDataDirPath);
    this.initTask = this.init();
  }
  async init() {
    if (!await exists(await this.guiSettingPath)) {
      await writeTextFile(await this.guiSettingPath, JSON.stringify(this.defaults, undefined, 4))
    }
    this._values.set(Object.assign(Object.assign({}, this.defaults), JSON.parse(await readTextFile(await this.guiSettingPath))));
  }
  async save(values: AppSetting) {
    await this.initTask;
    this.debouncedFileWriter.save(JSON.stringify(values, undefined, 4));
    this._values.set(Object.assign({}, values))
  }
}
