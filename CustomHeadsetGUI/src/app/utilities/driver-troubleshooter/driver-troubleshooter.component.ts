import { Component, signal } from '@angular/core';
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
import { DialogService } from '../../services/dialog.service';
import { join } from '@tauri-apps/api/path';
import { copyFile, exists, mkdir, readDir } from '@tauri-apps/plugin-fs';
import { open } from '@tauri-apps/plugin-dialog';
import { get_executable_path } from '../../tauri_wrapper';
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
  public installingDriver = signal(false)
  constructor(
    public dss: DriverSettingService,
    public dis: DriverInfoService,
    public ps: PathsService,
    public sds: SystemDiagnosticService,
    private dialog: DialogService
  ) {
    getVersion().then(this.appVersion.set);
    sds.initTask.then(() => this.wait.set(true));
    sds.startPullingSteamVRinstallState()
    sds.startPullingDriverinstallState()
  }
  async installSteamVR() {
    await openUrl('steam://install/250820')
  }
  async launchSteamVR() {
    await openUrl('steam://rungameid/250820')
  }
  async installDriver(steamVrPath: string) {
    this.installingDriver.set(true);
    try {
      let driverDir = await join(await get_executable_path(), '../CustomHeadsetOpenVR');
      if (!await exists(driverDir)) {
        if (await this.dialog.confirm($localize`Driver folder not found`, $localize`driver folder not in default location`, $localize`Locate`, 'primary')) {
          const path = await open({ directory: true, multiple: false })
          if (path) {
            driverDir = path;
          } else {
            return;
          }
        } else {
          return;
        }
      }
      if (await exists(await join(driverDir, 'driver.vrdrivermanifest'))) {
        const steamVrDriverDir = await join(steamVrPath, 'drivers');
        if (!await exists(steamVrDriverDir)) {
          await mkdir(steamVrDriverDir)
        }
        const driverPath = await join(steamVrDriverDir, 'CustomHeadsetOpenVR');
        await this.copyRec(driverPath, driverDir)
        this.sds.checkDriverInstalled()
      } else {
        await this.dialog.message($localize`Driver files not valid`, $localize`the folder seems not include driver file, please check again`)
      }
    } finally {
      this.installingDriver.set(false)
    }
  }
  private async copyRec(targetDir: string, sourceDir: string) {
    if (!await exists(targetDir)) {
      await mkdir(targetDir)
    }
    const content = await readDir(sourceDir);
    for (const e of content) {
      if (e.isFile) {
        await copyFile(await join(sourceDir, e.name), await join(targetDir, e.name));
      } else if (e.isDirectory) {
        await this.copyRec(await join(targetDir, e.name), await join(sourceDir, e.name));
      }
    }
  }
}
