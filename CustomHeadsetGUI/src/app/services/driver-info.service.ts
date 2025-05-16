import { Injectable, signal } from '@angular/core';
import { JsonSettingServiceBase } from './JsonSettingServiceBase';
import { DriverInfo } from './JsonFileDefines';
import { PathsService } from './paths.service';

@Injectable({
  providedIn: 'root'
})
export class DriverInfoService extends JsonSettingServiceBase<DriverInfo> {

  constructor(pathService: PathsService) {
    super(pathService.infoPath, pathService.appDataDirPath, signal(undefined), false, true);

  }
}
