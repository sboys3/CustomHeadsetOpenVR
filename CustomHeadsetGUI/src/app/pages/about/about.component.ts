import { Component, effect, inject, signal } from '@angular/core';
import { open } from '@tauri-apps/plugin-shell';
import { MatButtonModule } from '@angular/material/button';
import { MatIconModule } from '@angular/material/icon';
import { AppUpdateInfoSuccess, AppUpdateService } from '../../services/app-update.service';
import { delay, isNewVersion } from '../../helpers';
import { DriverInfoService } from '../../services/driver-info.service';
import { SystemDiagnosticService } from '../../services/system-diagnostic.service';
import { DialogService } from '../../services/dialog.service';
import { DriverSettingService } from '../../services/driver-setting.service'
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
  public dss = inject(DriverSettingService)
  private oldMeganeXEdidVendor: number | undefined = undefined
  private oldDreamAirEidVendor: number | undefined = undefined
  constructor(public aus: AppUpdateService, public dis: DriverInfoService, public sds: SystemDiagnosticService, private dialog: DialogService) {
    effect(() => {
        let newSettings = this.dss.values()
        
        if(newSettings?.meganeX8K?.edidVendorIdOverride != undefined && this.oldMeganeXEdidVendor != undefined && newSettings.meganeX8K.edidVendorIdOverride != this.oldMeganeXEdidVendor) {
          sds.restartCompositor()
        }
        this.oldMeganeXEdidVendor = newSettings?.meganeX8K?.edidVendorIdOverride
        if(newSettings?.dreamAir?.edidVendorIdOverride != undefined && this.oldDreamAirEidVendor != undefined && newSettings.dreamAir.edidVendorIdOverride != this.oldDreamAirEidVendor) {
          sds.restartCompositor()
        }
        this.oldDreamAirEidVendor = newSettings?.dreamAir?.edidVendorIdOverride
    });
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
