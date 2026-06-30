import { Injectable, signal, effect } from '@angular/core';
import { JsonSettingServiceBase } from './JsonSettingServiceBase';
import { DriverInfo, HeadsetType } from './JsonFileDefines';
import { PathsService } from './paths.service';

@Injectable({
  providedIn: 'root'
})
export class DriverInfoService extends JsonSettingServiceBase<DriverInfo> {

  currentHeadsetType = signal<HeadsetType | undefined>(undefined);

  constructor(pathService: PathsService) {
    super(pathService.infoPath, pathService.appDataDirPath, signal(undefined), false, true);

    effect(() => {
      const info = this.values();
      if (info && info.connectedHeadset !== undefined && info.connectedHeadset !== HeadsetType.None) {
        this.currentHeadsetType.set(info.connectedHeadset as HeadsetType);
      }
    });
  }
}
