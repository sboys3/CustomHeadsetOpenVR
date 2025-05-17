import { HttpClient } from '@angular/common/http';
import { Injectable, signal } from '@angular/core';
import { firstValueFrom } from 'rxjs';
import { getVersion } from '@tauri-apps/api/app';
export type GitHubRelease = {
  tag_name: string,
  html_url: string
}
export type AppUpdateInfoSuccess = {
  fetchSuccess: true;
  latestVersion: string;
  currentVersion: string;
  updateAvailable: boolean;
  url: string;
};
export type AppUpdateInfoErr = {
  fetchSuccess: false
}
export type AppUpdateInfo = AppUpdateInfoSuccess | AppUpdateInfoErr;
@Injectable({
  providedIn: 'root'

})
export class AppUpdateService {
  private _updateInfo = signal<AppUpdateInfoSuccess | undefined>(undefined);
  public updateInfo = this._updateInfo.asReadonly()
  constructor(private http: HttpClient) {
    this.checkUpdate()
  }
  private isNewVerrsion(current: string, latest: string) {
    const cParts = current.split('.')
    const lParts = latest.split('.')
    for (let i = 0, c = Math.max(cParts.length, lParts.length); i < c; i++) {
      const c = parseInt(cParts[i] ?? '0');
      const l = parseInt(lParts[i] ?? '0');
      if (l > c) return true;
    }
    return false;
  }
  private currentCheckTask?: Promise<AppUpdateInfo>;
  private async checkUpdateInternal(): Promise<AppUpdateInfo> {
    try {
      const request = firstValueFrom(this.http.get<GitHubRelease>('https://api.github.com/repos/sboys3/CustomHeadsetOpenVR/releases/latest'));
      const response = await request;
      const current = await getVersion();
      const result: AppUpdateInfoSuccess = {
        fetchSuccess: true,
        latestVersion: response.tag_name,
        currentVersion: current,
        updateAvailable: this.isNewVerrsion(current, response.tag_name),
        url: response.html_url
      }
      console.log(response)
      this._updateInfo.set(result);
      return result;
    } catch (e) {
      console.warn(e)
      this._updateInfo.set(undefined);
      return {
        fetchSuccess: false
      }
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
