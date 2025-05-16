import { Component, signal } from '@angular/core';
import { getVersion } from '@tauri-apps/api/app';
import { DriverSettingService } from '../../services/driver-setting.service';
import { DriverInfoService } from '../../services/driver-info.service';
import { CommonModule } from '@angular/common';
import { PathsService } from '../../services/paths.service';
@Component({
  selector: 'app-driver-troubleshooter',
  imports: [CommonModule],
  templateUrl: './driver-troubleshooter.component.html',
  styleUrl: './driver-troubleshooter.component.scss'
})
export class DriverTroubleshooterComponent {
  public appVersion = signal('')
  public wait = signal(false);
  constructor(public dss: DriverSettingService, public dis: DriverInfoService, public ps: PathsService) {
    getVersion().then(this.appVersion.set);
    Promise.all([dss.initTask,dis.initTask]).then(()=>{
      this.wait.set(true)
    })
  }
}
