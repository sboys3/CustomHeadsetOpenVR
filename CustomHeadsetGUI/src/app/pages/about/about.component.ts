import { Component, effect, inject, signal } from '@angular/core';
import { open } from '@tauri-apps/plugin-shell';
import { MatButtonModule } from '@angular/material/button';
import { MatIconModule } from '@angular/material/icon';
import { MatSlideToggleModule } from '@angular/material/slide-toggle';
import { FormsModule } from '@angular/forms';
import { AppUpdateInfoSuccess, AppUpdateService } from '../../services/app-update.service';
import { delay, isNewVersion } from '../../helpers';
import { DriverInfoService } from '../../services/driver-info.service';
import { SystemDiagnosticService } from '../../services/system-diagnostic.service';
import { DialogService } from '../../services/dialog.service';
import { DriverSettingService } from '../../services/driver-setting.service'
import {FieldTipComponent} from '../../utilities/field-tip/field-tip.component'
import { customHeadsetDriverName } from '../../../environment';
@Component({
  selector: 'app-about',
  imports: [MatButtonModule, MatIconModule, MatSlideToggleModule, FormsModule, FieldTipComponent,],
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
  public driverVersionMismatch = signal<boolean>(false);
  private _driverEnabledState = signal<boolean>(true);
  public driverEnabledToggle = true;
  constructor(public aus: AppUpdateService, public dis: DriverInfoService, public sds: SystemDiagnosticService, private dialog: DialogService) {
    effect(() => {
      const installedVersion = this.sds.driverInstalled();
      const lastRunVersion = this.dis.values()?.driverVersion;
      this.driverVersionMismatch.set(!!installedVersion && !!lastRunVersion && installedVersion !== lastRunVersion);
    });
    effect(() => {
      const config = this.sds.steamVrConfig();
      const enabled = this.sds.getSteamVRDriverEnableState(config, customHeadsetDriverName);
      this._driverEnabledState.set(enabled);
      this.driverEnabledToggle = enabled;
    });
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
  async openExternal(event: Event, url: string) {
    event.preventDefault()
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
  async toggleDriverEnabled() {
    if (this._driverEnabledState()) {
      await this.sds.disableSteamVRDriver(customHeadsetDriverName);
    } else {
      await this.sds.enableSteamVRDriver(customHeadsetDriverName);
    }
  }
}
