import { Component, effect } from '@angular/core';
import { DistortionProfileEntry, DriverSettingService } from '../../services/driver-setting.service';
import { MatListModule } from '@angular/material/list';
import { MatButtonModule } from '@angular/material/button';
import { ScrollingModule } from '@angular/cdk/scrolling';
import { open, save } from '@tauri-apps/plugin-dialog';
import { copyFile } from '@tauri-apps/plugin-fs';
import { basename } from '@tauri-apps/api/path';
import { DialogService } from '../../services/dialog.service';
import { Settings, MeganeX8KConfig } from '../../services/JsonFileDefines';
import { PathsService } from '../../services/paths.service';
import { MatIconModule } from '@angular/material/icon';
import { MatTooltipModule } from '@angular/material/tooltip'
@Component({
  selector: 'app-distortion-profile',
  imports: [MatListModule, MatButtonModule, ScrollingModule, MatIconModule, MatTooltipModule],
  templateUrl: './distortion-profile.component.html',
  styleUrl: './distortion-profile.component.scss'
})
export class DistortionProfileComponent {
  profiles: DistortionProfileEntry[] = [];
  settings?: MeganeX8KConfig
  config?: Settings;
  constructor(private fs: DriverSettingService, private ps: PathsService, private dialog: DialogService) {
    effect(() => {
      const config = fs.values();
      this.config = config;
      this.settings = config?.meganeX8K;
    });
    effect(() => {
      this.profiles = [
        ...this.fs.distortionProfileList().map(f => ({
          name: f.name.split('.').slice(0, -1).join('.'),
          isDefault: false,
          file: f
        }))]
    });
  }
  async deleteProfile(profile: DistortionProfileEntry) {
    if (await this.dialog.confirm($localize`Delete Profile`, $localize`Deleting [${profile.name}].\nThis action cannot be reverted. Are you sure?`)) {
      await this.fs.delete(profile.file!.name);
    }
  }
  async importFile() {
    const files = await open({
      multiple: true,
      directory: false,
      filters: [{ name: `json file`, extensions: ["json"] }]
    })
    if (files?.length) {
      const result = await this.fs.importFile(files);
      const message = []
      for (const f of files) {
        if (result[f]) {
          const r = result[f];
          if (!r.success) {
            message.push(`file:[${f}]\n${r.message}\n`);
          }
        }
      }
      if (message.length) {
        this.dialog.message($localize`Import Error`, message.join('\n'));
      }
    }
  }
  async exportFile(profile: DistortionProfileEntry) {
    const file = await save({ defaultPath: await basename(profile.name), filters: [{ name: `json file`, extensions: ["json"] }] });
    if (file) {
      await copyFile(await this.ps.getProfileFullPath(profile.file!.name), file);
    }
  }
}
