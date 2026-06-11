import { Component, effect, signal } from '@angular/core';
import { MatButtonModule } from '@angular/material/button';
import { MatIconModule } from '@angular/material/icon';
import { MatTooltipModule } from '@angular/material/tooltip';
import { MatTableModule } from '@angular/material/table';
import { open, save } from '@tauri-apps/plugin-dialog';
import { copyFile, readTextFile, writeTextFile } from '@tauri-apps/plugin-fs';
import { basename, join } from '@tauri-apps/api/path';
import { openPath } from '@tauri-apps/plugin-opener';
import { DialogService } from '../../services/dialog.service';
import { DistortionProfileEntry, DistortionProfileFile, DriverSettingService } from '../../services/driver-setting.service';
import { DriverInfoService } from '../../services/driver-info.service';
import { PathsService } from '../../services/paths.service';
import { BuiltInDistortionProfile, DriverInfo } from '../../services/JsonFileDefines';
import { cleanJsonComments } from '../../helpers';
import { SystemReadyComponent } from '../../utilities/system-ready/system-ready.component';
import { DistortionProfileDisplayNames } from '../devices/DeviceConfigComponentBase';

export interface ProfileInfo {
  name: string;
  displayName: string;
  isBuiltIn: boolean;
  device?: string;
  description?: string;
  author?: string;
  creationDate?: number;
  type?: string;
  distortionPoints?: number;
  hasChromaticAberration?: boolean;
  smoothAmount?: number;
  file?: DistortionProfileFile;
  rawBuiltIn?: BuiltInDistortionProfile;
}

@Component({
  selector: 'app-distortion-profile',
  imports: [
    MatButtonModule,
    MatIconModule,
    MatTooltipModule,
    MatTableModule,
    SystemReadyComponent
  ],
  templateUrl: './distortion-profile.component.html',
  styleUrl: './distortion-profile.component.scss'
})
export class DistortionProfileComponent {
  builtInProfiles = signal<ProfileInfo[]>([]);
  customProfiles = signal<ProfileInfo[]>([]);
  loaded = signal(false);

  displayedColumnsBuiltIn: string[] = ['name', 'details', 'author', 'description', 'actions'];
  displayedColumnsCustom: string[] = ['name', 'details', 'author', 'description', 'actions'];

  constructor(
    public dss: DriverSettingService,
    private dis: DriverInfoService,
    private ps: PathsService,
    private dialog: DialogService
  ) {
    effect(async () => {
      const info = (dis.values() ?? {}) as DriverInfo;
      const builtIn = info?.builtInDistortionProfiles ?? {};
      const customFiles = this.dss.distortionProfileList();

      // Build built-in profile entries
      const builtInEntries: ProfileInfo[] = Object.keys(builtIn).map(name => {
        const profile = (builtIn as any)[name] as BuiltInDistortionProfile;
        const distortionPoints = profile.distortions ? profile.distortions.length / 2 : 0;
        const hasChromaticAberration = (profile.distortionsRed?.length ?? 0) > 0 || (profile.distortionsBlue?.length ?? 0) > 0;
        return {
          name,
          displayName: DistortionProfileDisplayNames.get(name) || name,
          isBuiltIn: true,
          device: profile.device,
          description: profile.description,
          author: profile.author,
          creationDate: profile.creationDate,
          type: profile.type,
          distortionPoints,
          hasChromaticAberration,
          smoothAmount: profile.smoothAmount,
          rawBuiltIn: profile
        };
      });

      // Build custom profile entries
      const customEntries: ProfileInfo[] = [];
      for (const file of customFiles) {
        const name = file.name.split('.').slice(0, -1).join('.');
        const entry: ProfileInfo = {
          name,
          displayName: DistortionProfileDisplayNames.get(name) || name,
          isBuiltIn: false,
          device: file.device,
          file
        };
        try {
          const fullPath = await join(this.ps.distortionDirPath, file.name);
          const content = await readTextFile(fullPath);
          const parsed = JSON.parse(cleanJsonComments(content)) as any;
          entry.description = parsed.description;
          entry.author = parsed.author;
          entry.creationDate = parsed.creationDate;
          entry.type = parsed.type;
          entry.distortionPoints = parsed.distortions ? parsed.distortions.length / 2 : 0;
          entry.hasChromaticAberration = (parsed.distortionsRed?.length ?? 0) > 0 || (parsed.distortionsBlue?.length ?? 0) > 0;
          entry.smoothAmount = parsed.smoothAmount;
        } catch (e) {
          // ignore parse errors
        }
        customEntries.push(entry);
      }

      this.builtInProfiles.set(builtInEntries);
      this.customProfiles.set(customEntries);
      this.loaded.set(true);
    });
  }

  formatDate(timestamp?: number): string {
    if (!timestamp || timestamp === 0) {
      return 'N/A';
    }
    const date = new Date(timestamp * 1000);
    // return date.toLocaleDateString('en-US', {
    //   year: 'numeric',
    //   month: 'short',
    //   day: 'numeric'
    // });
    return date.toLocaleDateString()
  }
  
  async openDir() {
    await openPath(this.ps.distortionDirPath);
  }
  
  async importFile() {
    const files = await open({
      multiple: true,
      directory: false,
      filters: [{ name: `json file`, extensions: ["json"] }]
    })
    if (files?.length) {
      const result = await this.dss.importFile(files);
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
  
  async deleteProfile(profile: ProfileInfo) {
    if (await this.dialog.confirm($localize`Delete Profile`, $localize`Deleting [${profile.name}].\nThis action cannot be reverted. Are you sure?`)) {
      await this.dss.delete(profile.file!.name);
    }
  }
  
  async exportFile(profile: ProfileInfo) {
    const file = await save({ defaultPath: await basename(profile.name), filters: [{ name: `json file`, extensions: ["json"] }] });
    if (file) {
      await copyFile(await this.ps.getProfileFullPath(profile.file!.name), file);
    }
  }
  
  async exportBuiltIn(profile: ProfileInfo) {
    const raw = profile.rawBuiltIn as any;
    if (!raw) return;

    // Build JSON manually to match custom profile format (tabs, distortions pairs per line)
    const lines: string[] = [];
    lines.push('{');
    
    const push = (key: string, value: unknown) => {
      if (typeof value === 'string') {
        lines.push(`\t"${key}":${JSON.stringify(value)},`);
      } else {
        lines.push(`\t"${key}": ${JSON.stringify(value)},`);
      }
    };
    
    const pushArray = (key: string, arr: number[]) => {
      const pairs: string[] = [];
      for (let i = 0; i < arr.length; i += 2) {
        const isLast = i + 2 >= arr.length;
        pairs.push(`\t\t${arr[i]}, ${arr[i + 1]}${isLast ? '' : ','}`);
      }
      lines.push(`\t"${key}":[`);
      lines.push(...pairs);
      lines.push(`\t],`);
    };
    
    if (raw['description']) push('description', raw['description']);
    if (raw['device']) push('device', raw['device']);
    if (raw['author']) push('author', raw['author']);
    if (raw['creationDate']) push('creationDate', raw['creationDate']);
    if (raw['type']) push('type', raw['type']);
    if (raw['legacySmoothing']) push('legacySmoothing', raw['legacySmoothing']);
    if (raw['legacySmoothing'] && raw['smoothAmount'] !== undefined) push('smoothAmount', raw['smoothAmount']);
    if (raw['offsetX'] !== undefined) push('offsetX', raw['offsetX']);
    if (raw['offsetY'] !== undefined) push('offsetY', raw['offsetY']);
    
    if (raw['distortions']) pushArray('distortions', raw['distortions']);
    if (raw['distortionsRed'] && raw['distortionsRed'].length > 0) pushArray('distortionsRed', raw['distortionsRed']);
    if (raw['distortionsBlue'] && raw['distortionsBlue'].length > 0) pushArray('distortionsBlue', raw['distortionsBlue']);
    
    // Remove trailing comma from last line
    if (lines[lines.length - 1].endsWith(',')) {
      lines[lines.length - 1] = lines[lines.length - 1].slice(0, -1);
    }
    
    lines.push('}');
    const content = lines.join('\n') + '\n';
    
    const file = await save({ defaultPath: profile.name + '.json', filters: [{ name: `json file`, extensions: ["json"] }] });
    if (file) {
      await writeTextFile(file, content);
    }
  }
}
