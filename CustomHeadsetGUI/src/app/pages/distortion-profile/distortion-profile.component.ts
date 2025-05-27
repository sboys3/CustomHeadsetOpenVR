import { Component, computed, effect, AfterViewInit, OnDestroy, viewChild } from '@angular/core';
import { DistortionProfileEntry, DriverSettingService } from '../../services/driver-setting.service';
import { MatListModule } from '@angular/material/list';
import { MatButtonModule } from '@angular/material/button';
import { ScrollingModule, CdkVirtualScrollViewport } from '@angular/cdk/scrolling';
import { open, save } from '@tauri-apps/plugin-dialog';
import { copyFile } from '@tauri-apps/plugin-fs';
import { basename } from '@tauri-apps/api/path';
import { DialogService } from '../../services/dialog.service';
import { Settings, MeganeX8KConfig } from '../../services/JsonFileDefines';
import { PathsService } from '../../services/paths.service';
import { MatIconModule } from '@angular/material/icon';
import { MatTooltipModule } from '@angular/material/tooltip'
import { deepCopy } from '../../helpers';
import { DistortionProfileViewerComponent } from "../../utilities/distortion-profile-viewer/distortion-profile-viewer.component";
import { openPath } from '@tauri-apps/plugin-opener';
import { SystemReadyComponent } from '../../utilities/system-ready/system-ready.component';
@Component({
  selector: 'app-distortion-profile',
  imports: [
    MatListModule,
    MatButtonModule,
    ScrollingModule,
    MatIconModule,
    MatTooltipModule,
    DistortionProfileViewerComponent,
    SystemReadyComponent
  ],
  templateUrl: './distortion-profile.component.html',
  styleUrl: './distortion-profile.component.scss'
})
export class DistortionProfileComponent implements AfterViewInit, OnDestroy {
  profiles = computed(() => {
    const localSettingFiles = this.dss.distortionProfileList();
    return [
      ...localSettingFiles.map(f => ({
        name: f.name.split('.').slice(0, -1).join('.'),
        isDefault: false,
        file: f
      }))].sort((a, b) => a.name.localeCompare(b.name))
  })
  viewport = viewChild.required(CdkVirtualScrollViewport)

  settings?: MeganeX8KConfig
  config?: Settings;
  displayedCurve: string[] = [];
  activedProfile?: string;

  private resizeObserver!: ResizeObserver;

  constructor(public dss: DriverSettingService, private ps: PathsService, private dialog: DialogService) {
    effect(() => {
      const config = dss.values();;
      this.config = config;
      this.settings = config?.meganeX8K;
      this.updateDisplayCurve()
    });
  }

  ngAfterViewInit(): void {
    // not very sure why cdk scroll need this to work correctly , it was ok last version , temporary fix
    this.viewport().checkViewportSize();
    this.resizeObserver = new ResizeObserver(() => {
      this.viewport().checkViewportSize();
    });
    this.resizeObserver.observe(this.viewport().elementRef.nativeElement);
  }

  ngOnDestroy(): void {
      this.resizeObserver?.disconnect();
  }

  private updateDisplayCurve() {
    const n: string[] = []
    if (this.settings?.distortionProfile) {
      n.push(this.settings.distortionProfile)
    }
    if (this.activedProfile && !n.includes(this.activedProfile)) {
      n.push(this.activedProfile)
    }
    this.displayedCurve = n;
  }
  async showProfile(profile: DistortionProfileEntry) {
    this.activedProfile = profile.name;
    this.updateDisplayCurve()

  }
  async openDir() {
    await openPath(this.ps.distortionDirPath);
  }
  async applyProfile(name: string) {
    const saving = deepCopy(this.config)!;
    saving.meganeX8K.distortionProfile = name;
    await this.dss.save(saving);
  }
  async deleteProfile(profile: DistortionProfileEntry) {
    if (await this.dialog.confirm($localize`Delete Profile`, $localize`Deleting [${profile.name}].\nThis action cannot be reverted. Are you sure?`)) {
      await this.dss.delete(profile.file!.name);
    }
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
  async exportFile(profile: DistortionProfileEntry) {
    const file = await save({ defaultPath: await basename(profile.name), filters: [{ name: `json file`, extensions: ["json"] }] });
    if (file) {
      await copyFile(await this.ps.getProfileFullPath(profile.file!.name), file);
    }
  }
}
