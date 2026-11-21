import { computed, inject, Injectable, signal } from '@angular/core';
import { toObservable } from '@angular/core/rxjs-interop';
import { join } from '@tauri-apps/api/path';
import { exists, readDir, readTextFile, remove, rename, watchImmediate, writeTextFile } from '@tauri-apps/plugin-fs';
import { debounceTime, filter, firstValueFrom, Subject } from 'rxjs';
import { cleanJsonComments, delay } from '../helpers';
import { AppSettingService } from './app-setting.service';
import { DriverInfoService } from './driver-info.service';
import { DriverSettingService } from './driver-setting.service';
import { Settings } from './JsonFileDefines';
import { PathsService } from './paths.service';
import { cloneJson, fullSettings, keepHeadsetEnableFlags, profileIdFromName, settingsMatch, sparseSettings } from './settings-profile-helpers';

export type SettingsProfile = {
  // file name in the Profiles folder without .json
  id: string;
  name: string;
  // only the values that differ from the driver defaults
  settings: Partial<Settings>;
};
type SettingsProfileFile = {
  name: string;
  settings: Partial<Settings>;
};

/**
 * Named copies of settings.json in the Profiles folder.
 * Edits in the GUI still go to settings.json right away. A profile changes only when the user saves it.
 * Selecting a profile writes its settings to settings.json, except the headset enable flags.
 */
@Injectable({
  providedIn: 'root'
})
export class SettingsProfileService {
  private dss = inject(DriverSettingService);
  private dis = inject(DriverInfoService);
  private ass = inject(AppSettingService);
  private paths = inject(PathsService);

  private readonly _profiles = signal<SettingsProfile[]>([]);
  public readonly profiles = this._profiles.asReadonly();
  private readonly _ready = signal(false);
  public readonly ready = this._ready.asReadonly();
  public readonly activeId = computed(() => this.ass.values()?.activeProfile ?? '');
  public readonly activeProfile = computed(() => this.profiles().find(p => p.id == this.activeId()));
  // true when settings.json differs from the saved active profile
  public readonly isDirty = computed(() => {
    const profile = this.activeProfile();
    const values = this.dss.values();
    if (!profile || !values) {
      return false;
    }
    const defaults = this.defaults();
    return !settingsMatch(values, fullSettings(profile.settings, defaults), defaults);
  });
  private readonly defaults = computed(() => {
    // dss.values() changes after dss.defaults, so reading it here keeps this computed current
    this.dss.values();
    return this.dss.defaults as Settings | undefined;
  });
  private readonly settingsLoaded = firstValueFrom(toObservable(this.dss.values).pipe(filter(v => !!v)));
  private readonly appSettingsLoaded = firstValueFrom(toObservable(this.ass.values).pipe(filter(v => !!v)));
  private readonly infoLoaded = firstValueFrom(toObservable(this.dis.values).pipe(filter(v => !!v)));
  private initTask?: Promise<void>;

  public init() {
    return this.initTask ??= this.doInit();
  }

  private async doInit() {
    await Promise.all([this.dss.initTask, this.ass.initTask, this.settingsLoaded, this.appSettingsLoaded]);
    // info.json holds the driver defaults, which keep profiles small. It is missing until the driver first runs, so wait only briefly.
    await Promise.race([this.infoLoaded, delay(3000)]);
    await this.reload();
    await this.createDefaultProfileOnFirstRun();
    const changes = new Subject<void>();
    changes.pipe(debounceTime(50)).subscribe(() => this.reload());
    await watchImmediate(this.paths.profilesDirPath, ev => {
      if (ev.paths.some(p => p.toLowerCase().endsWith('.json'))) {
        changes.next();
      }
    });
    this._ready.set(true);
  }

  private async reload() {
    const entries = await readDir(this.paths.profilesDirPath);
    const profiles: SettingsProfile[] = [];
    for (const entry of entries) {
      if (!entry.isFile || !entry.name.toLowerCase().endsWith('.json')) {
        continue;
      }
      const id = entry.name.slice(0, -'.json'.length);
      try {
        const parsed = JSON.parse(cleanJsonComments(await readTextFile(await join(this.paths.profilesDirPath, entry.name))));
        const settings = typeof parsed?.settings === 'object' && parsed.settings !== null ? parsed.settings : {};
        const name = typeof parsed?.name === 'string' && parsed.name.trim() ? parsed.name.trim() : id;
        profiles.push({ id, name, settings });
      } catch (e) {
        // keep the last good copy when a file is read mid-write or was broken by hand
        const previous = this._profiles().find(p => p.id == id);
        if (previous) {
          profiles.push(previous);
        }
        console.error('failed to read settings profile', entry.name, e);
      }
    }
    profiles.sort((a, b) => a.name.localeCompare(b.name, undefined, { sensitivity: 'base' }));
    this._profiles.set(profiles);
  }

  private async createDefaultProfileOnFirstRun() {
    if (this.ass.values()?.profilesInitialized) {
      return;
    }
    let activeProfile = this.activeId();
    if (this.profiles().length == 0) {
      const profile = await this.writeNewProfile($localize`Default`, sparseSettings(this.dss.values(), this.defaults()));
      activeProfile = profile.id;
    }
    await this.saveAppSettings({ profilesInitialized: true, activeProfile });
  }

  private async saveAppSettings(changes: { activeProfile?: string, profilesInitialized?: boolean }) {
    const values = this.ass.values();
    if (values) {
      await this.ass.save({ ...values, ...changes });
    }
  }

  private async profilePath(id: string) {
    return await join(this.paths.profilesDirPath, `${id}.json`);
  }

  private async writeProfile(profile: SettingsProfile) {
    const file: SettingsProfileFile = { name: profile.name, settings: profile.settings };
    // write a temporary file and rename it, so the folder watcher never reads a half-written profile
    const tempPath = await join(this.paths.profilesDirPath, `${profile.id}.${self.crypto.randomUUID()}.tmp`);
    await writeTextFile(tempPath, JSON.stringify(file, undefined, 4));
    await rename(tempPath, await this.profilePath(profile.id));
    const others = this._profiles().filter(p => p.id != profile.id);
    this._profiles.set([...others, profile].sort((a, b) => a.name.localeCompare(b.name, undefined, { sensitivity: 'base' })));
  }

  private async writeNewProfile(name: string, settings: Partial<Settings>) {
    const profile: SettingsProfile = {
      id: profileIdFromName(name, this.profiles().map(p => p.id)),
      name: name.trim(),
      settings
    };
    await this.writeProfile(profile);
    return profile;
  }

  // Writes the current settings into the active profile.
  public async saveActive() {
    const profile = this.activeProfile();
    const values = this.dss.values();
    if (!profile || !values) {
      return;
    }
    await this.writeProfile({ ...profile, settings: sparseSettings(values, this.defaults()) });
  }

  // Writes the profile's settings to settings.json and makes it the active profile.
  public async select(id: string) {
    const profile = this.profiles().find(p => p.id == id);
    if (!profile) {
      return;
    }
    const next = keepHeadsetEnableFlags(fullSettings(profile.settings, this.defaults()), this.dss.values());
    await this.dss.save(next);
    await this.saveAppSettings({ activeProfile: id });
  }

  // Creates a profile from the driver defaults and selects it. Headsets stay enabled or disabled as they are now.
  public async createFromDefaults(name: string) {
    const settings = keepHeadsetEnableFlags(cloneJson(this.defaults() ?? {} as Settings), this.dss.values());
    const profile = await this.writeNewProfile(name, sparseSettings(settings, this.defaults()));
    await this.select(profile.id);
  }

  // Creates a profile from the current settings, including unsaved changes, and selects it.
  public async createFromCurrent(name: string) {
    const profile = await this.writeNewProfile(name, sparseSettings(this.dss.values(), this.defaults()));
    await this.saveAppSettings({ activeProfile: profile.id });
  }

  // Deletes the profile file. Deleting the active profile leaves settings.json as it is.
  public async delete(id: string) {
    const path = await this.profilePath(id);
    if (await exists(path)) {
      await remove(path);
    }
    this._profiles.set(this._profiles().filter(p => p.id != id));
    if (this.activeId() == id) {
      await this.saveAppSettings({ activeProfile: '' });
    }
  }
}
