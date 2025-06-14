import { Component, effect, signal } from '@angular/core';
import { getVersion } from '@tauri-apps/api/app';
import { DriverSettingService } from '../../services/driver-setting.service';
import { DriverInfoService } from '../../services/driver-info.service';
import { CommonModule } from '@angular/common';
import { PathsService } from '../../services/paths.service';
import { SystemDiagnosticService } from '../../services/system-diagnostic.service';
import { MatDividerModule } from '@angular/material/divider';
import { MatButtonModule } from '@angular/material/button';
import { openUrl } from '@tauri-apps/plugin-opener';
import { MatIconModule } from '@angular/material/icon';
@Component({
  selector: 'app-driver-troubleshooter',
  imports: [CommonModule, MatDividerModule, MatIconModule, MatButtonModule],
  providers: [],
  templateUrl: './driver-troubleshooter.component.html',
  styleUrl: './driver-troubleshooter.component.scss'
})
export class DriverTroubleshooterComponent {
  public appVersion = signal('')
  public wait = signal(false);
  public driverEnablePrompt = signal(false);
  constructor(
    public dss: DriverSettingService,
    public dis: DriverInfoService,
    public ps: PathsService,
    public sds: SystemDiagnosticService
  ) {
    getVersion().then(this.appVersion.set);
    sds.initTask.then(() => {
      this.wait.set(true);
      if (!sds.steamVRinstalled()) {
        sds.pullingSteamVRinstall.start()
      }
      if (!sds.driverInstalled()) {
        sds.PullingDriverinstall.start()
      }
    });
    effect(() => {
      const steamVrConfig = sds.steamVrConfig();
      if(steamVrConfig){
          let customEnabled = this.sds.getSteamVRDriverEnableState(steamVrConfig, 'CustomHeadsetOpenVR');
          this.driverEnablePrompt.set(!customEnabled);
      }
    })
  }
  async enableDriver(){
    await this.sds.enableSteamVRDriver('CustomHeadsetOpenVR');
  }
  async installSteamVR() {
    await openUrl('steam://install/250820')
  }
  async launchSteamVR() {
    await openUrl('steam://rungameid/250820')
  }

}
