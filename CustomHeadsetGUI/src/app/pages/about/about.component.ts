import { Component, signal } from '@angular/core';
import { open } from '@tauri-apps/plugin-shell';
import { MatButtonModule } from '@angular/material/button';
import { MatIconModule } from '@angular/material/icon';
import { AppUpdateInfoSuccess, AppUpdateService } from '../../services/app-update.service';
import { delay, isNewVersion } from '../../helpers';
import { DriverInfoService } from '../../services/driver-info.service';
import { SystemDiagnosticService } from '../../services/system-diagnostic.service';
import { DialogService } from '../../services/dialog.service';
@Component({
  selector: 'app-about',
  imports: [MatButtonModule, MatIconModule],
  providers: [],
  templateUrl: './about.component.html',
  styleUrl: './about.component.scss'
})
export class AboutComponent {

  public isNewVersion = isNewVersion;
  public checking = signal<boolean>(false)
  public updateInfo = signal<AppUpdateInfoSuccess | undefined>(undefined)
  constructor(public aus: AppUpdateService, public dis: DriverInfoService, public sds: SystemDiagnosticService, private dialog: DialogService) {

  }
  async openExternal(url: string) {
    await open(url)
  }

  async checkUpdate() {
    this.checking.set(true)
    try {
      const start = performance.now();
      await this.aus.checkUpdate();

      const wait = 2000 - (performance.now() - start);
      if (wait > 0) {
        await delay(wait)
      }
    } finally {
      this.checking.set(false)
    }
  }
  async installDriver() {
    if (await this.sds.installDriver()) {
      this.dialog.message($localize`Install success`, $localize`please launch SteamVR to finish the installation`)
    }
  }
  async uninstallDriver(){
    if (await this.sds.uninstallDriver()) {
      this.dialog.message($localize`Uninstall success`, $localize`Successfully uninstalled the driver`)
    }
  }
}
