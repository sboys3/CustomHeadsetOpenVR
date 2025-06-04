import { effect, inject, Injectable, signal } from '@angular/core';
import { AppSetting } from './JsonFileDefines';
import { PathsService } from './paths.service';
import { JsonSettingServiceBase } from './JsonSettingServiceBase';
import { AppSettingHolder } from './AppSettingAccessor';

@Injectable({
  providedIn: 'root'
})
export class AppSettingService extends JsonSettingServiceBase<AppSetting> {
  constructor(paths: PathsService) {
    super(paths.guiSettingPath, paths.appDataDirPath, signal({
      colorScheme: 'system',
      updateMode: 'rewrite',
      advanceMode: false
    }), true, true)
    const appSettingHolder = inject(AppSettingHolder)
    effect(() => {
      const values = this.values() ?? {} as AppSetting;
      appSettingHolder.settings = values;
    });
  }
}
