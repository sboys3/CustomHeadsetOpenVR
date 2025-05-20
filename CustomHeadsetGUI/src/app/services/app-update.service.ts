import { HttpClient } from '@angular/common/http';
import { Injectable, signal } from '@angular/core';
import { firstValueFrom } from 'rxjs';
import { getVersion } from '@tauri-apps/api/app';
import { isNewVersion } from '../helpers';
export type GitHubRelease = {
  tag_name: string,
  html_url: string
}
export type AppUpdateInfoSuccess = {
  fetchSuccess: boolean;
  latestVersion: string;
  currentVersion: string;
  updateAvailable: boolean;
  url: string;
};
export type AppUpdateInfo = AppUpdateInfoSuccess;
@Injectable({
  providedIn: 'root'

})
export class AppUpdateService {
  private _updateInfo = signal<AppUpdateInfoSuccess | undefined>(undefined);
  public updateInfo = this._updateInfo.asReadonly()
  constructor(private http: HttpClient) {
    this.checkUpdate()
  }
  private currentCheckTask?: Promise<AppUpdateInfo>;
  private async checkUpdateInternal(): Promise<AppUpdateInfo> {
    
    const current = await getVersion();
    const result: AppUpdateInfoSuccess = {
      fetchSuccess: false,
      latestVersion: "Unknown",
      currentVersion: current,
      updateAvailable: false,
      url: ""
    }
    try {
      const request = firstValueFrom(this.http.get<GitHubRelease>('https://api.github.com/repos/sboys3/CustomHeadsetOpenVR/releases/latest'));
      const response = await request;
      result.latestVersion = response.tag_name
      result.updateAvailable = isNewVersion(current, response.tag_name)
      result.url = response.html_url
      this._updateInfo.set(result);
      return result;
    } catch (e) {
      console.warn(e)
      this._updateInfo.set(result);
      return result
    } finally {
      this.currentCheckTask = undefined;
    }
  }
  async checkUpdate(): Promise<AppUpdateInfo> {
    if (this.currentCheckTask) {
      return this.currentCheckTask;
    }
    return this.currentCheckTask = this.checkUpdateInternal()
  }

}
