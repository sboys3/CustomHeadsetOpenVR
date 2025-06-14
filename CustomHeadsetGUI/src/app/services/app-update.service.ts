import { HttpClient } from '@angular/common/http';
import { effect, Injectable, signal } from '@angular/core';
import { firstValueFrom } from 'rxjs';
import { getVersion } from '@tauri-apps/api/app';
import { isNewVersion } from '../helpers';
import { SystemDiagnosticService } from './system-diagnostic.service';
export type GitHubRelease = {
  tag_name: string,
  html_url: string
}
export type AppUpdateInfoSuccess = {
  fetchSuccess: boolean;
  latestVersion: string;
  currentVersion: string;
  updateAvailable: boolean;
  installAvailable: boolean;
  url: string;
};
export type AppUpdateInfo = AppUpdateInfoSuccess;
@Injectable({
  providedIn: 'root'

})
export class AppUpdateService {
  private _updateInfo = signal<AppUpdateInfoSuccess | undefined>(undefined);
  public updateInfo = this._updateInfo.asReadonly()
  constructor(private http: HttpClient, public sds: SystemDiagnosticService) {
    this.checkUpdate();
    effect(async () => {
      const driverVersion = this.sds.driverInstalled();
      let updateInfo = this._updateInfo();
      if(driverVersion){
        if(updateInfo){
          const current = await getVersion();
          updateInfo.installAvailable = isNewVersion(driverVersion, current);
          this._updateInfo.set(updateInfo);
        }
      }
    });
  }
  private currentCheckTask?: Promise<AppUpdateInfo>;
  private async checkUpdateInternal(): Promise<AppUpdateInfo> {
    
    const current = await getVersion();
    const result: AppUpdateInfoSuccess = {
      fetchSuccess: false,
      latestVersion: "Unknown",
      currentVersion: current,
      updateAvailable: false,
      installAvailable: false,
      url: ""
    }
    try {
      const request = firstValueFrom(this.http.get<GitHubRelease>('https://api.github.com/repos/sboys3/CustomHeadsetOpenVR/releases/latest'));
      const response = await request;
      result.latestVersion = response.tag_name
      result.updateAvailable = isNewVersion(current, result.latestVersion)
      result.url = response.html_url
    } catch (e) {
      console.warn(e)
    } finally {
      this.currentCheckTask = undefined;
      this._updateInfo.set(result);
      return result;
    }
  }
  async checkUpdate(): Promise<AppUpdateInfo> {
    if (this.currentCheckTask) {
      return this.currentCheckTask;
    }
    return this.currentCheckTask = this.checkUpdateInternal()
  }

}
