import { computed, inject, Injectable, signal } from '@angular/core';
import { toObservable } from '@angular/core/rxjs-interop';
import { join } from '@tauri-apps/api/path';
import { exists, mkdir, readDir, readTextFile, remove, rename, watchImmediate, writeTextFile } from '@tauri-apps/plugin-fs';
import { debounceTime, filter, firstValueFrom, Subject } from 'rxjs';
import { cleanJsonComments, delay } from '../helpers';
import { AppSettingService } from './app-setting.service';
import { DriverInfoService } from './driver-info.service';
import { DriverSettingService } from './driver-setting.service';
import { Settings } from './JsonFileDefines';
import { PathsService } from './paths.service';
import { cloneJson, fullSettings, isPlainObject, keepHeadsetEnableFlags, profileIdFromName, profileSettings, settingsMatch, turnsOnPackedFoveation } from './settings-profile-helpers';

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

// Windows file names ignore case, so profile ids do too.
function sameId(a: string, b: string) {
  return a.toLowerCase() == b.toLowerCase();
}

function sortProfiles(profiles: SettingsProfile[]) {
  return profiles.sort((a, b) => a.name.localeCompare(b.name, undefined, { sensitivity: 'base' }));
}

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
  public readonly activeProfile = computed(() => this.profiles().find(p => sameId(p.id, this.activeId())));
  // true when settings.json differs from the active profile, or from every profile when none is active
  public readonly isDirty = computed(() => {
    const values = this.dss.values();
    if (!values) {
      return false;
    }
    const active = this.activeProfile();
    const candidates = active ? [active] : this.profiles();
    return !candidates.some(p => this.matches(values, p));
  });
  // false while settings.json is missing or broken, so a profile action cannot overwrite it blindly
  public readonly usable = computed(() => this.ready() && !!this.dss.values() && !this.dss.readFileError());
  private readonly defaults = computed(() => {
    // dss.values() changes after dss.defaults, so reading it here keeps this computed current
    this.dss.values();
    return this.dss.defaults as Settings | undefined;
  });
  private readonly settingsLoaded = firstValueFrom(toObservable(this.dss.values).pipe(filter(v => !!v)));
  private readonly appSettingsLoaded = firstValueFrom(toObservable(this.ass.values).pipe(filter(v => !!v)));
  private readonly infoLoaded = firstValueFrom(toObservable(this.dis.values).pipe(filter(v => !!v)));
  private initTask?: Promise<void>;

  private matches(values: Settings, profile: SettingsProfile) {
    const defaults = this.defaults();
    return settingsMatch(values, fullSettings(profile.settings, defaults), defaults);
  }

  // Runs folder reads and writes one at a time, so a reload never puts back a profile that a write just replaced.
  private withFolderLock<T>(action: () => Promise<T>): Promise<T> {
    return navigator.locks.request('settings-profiles-folder', action);
  }

  public init() {
    return this.initTask ??= this.doInit().catch(e => {
      console.error('settings profiles failed to start', e);
    });
  }

  private async doInit() {
    await Promise.all([this.dss.initTask, this.ass.initTask, this.settingsLoaded, this.appSettingsLoaded]);
    if (!await exists(this.paths.profilesDirPath)) {
      await mkdir(this.paths.profilesDirPath);
    }
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

  private reload() {
    return this.withFolderLock(() => this.readProfiles());
  }

  private async readProfiles() {
    const entries = await readDir(this.paths.profilesDirPath);
    const profiles: SettingsProfile[] = [];
    for (const entry of entries) {
      // only lower case ".json", so the id always maps back to the same file name on Linux
      const id = entry.name.slice(0, -'.json'.length);
      if (!entry.isFile || !entry.name.endsWith('.json') || !id) {
        continue;
      }
      try {
        const parsed = JSON.parse(cleanJsonComments(await readTextFile(await join(this.paths.profilesDirPath, entry.name))));
        if (parsed?.settings !== undefined && !isPlainObject(parsed.settings)) {
          throw new Error('"settings" is not an object');
        }
        const settings = parsed?.settings ?? {};
        const name = typeof parsed?.name === 'string' && parsed.name.trim() ? parsed.name.trim() : id;
        profiles.push({ id, name, settings });
      } catch (e) {
        // keep the last good copy when a file is read mid-write or was broken by hand
        const previous = this._profiles().find(p => sameId(p.id, id));
        if (previous) {
          profiles.push(previous);
        }
        console.error('failed to read settings profile', entry.name, e);
      }
    }
    this._profiles.set(sortProfiles(profiles));
  }

  private async createDefaultProfileOnFirstRun() {
    if (this.ass.values()?.profilesInitialized) {
      return;
    }
    let activeProfile = this.activeId();
    if (this.profiles().length == 0) {
      const profile = await this.writeNewProfile($localize`Default`, profileSettings(this.dss.values(), this.defaults()));
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

  // Call only while holding the folder lock.
  private async writeProfileFile(profile: SettingsProfile) {
    const file: SettingsProfileFile = { name: profile.name, settings: profile.settings };
    // write a temporary file and rename it, so the folder watcher never reads a half-written profile
    const tempPath = await join(this.paths.profilesDirPath, `${profile.id}.${self.crypto.randomUUID()}.tmp`);
    try {
      await writeTextFile(tempPath, JSON.stringify(file, undefined, 4));
      await rename(tempPath, await this.profilePath(profile.id));
    } finally {
      if (await exists(tempPath)) {
        await remove(tempPath);
      }
    }
    const others = this._profiles().filter(p => !sameId(p.id, profile.id));
    this._profiles.set(sortProfiles([...others, profile]));
  }

  private writeProfile(profile: SettingsProfile) {
    return this.withFolderLock(() => this.writeProfileFile(profile));
  }

  private writeNewProfile(name: string, settings: Partial<Settings>) {
    return this.withFolderLock(async () => {
      // take ids from the folder, not the loaded list, so a file that failed to parse is never overwritten
      const files = await readDir(this.paths.profilesDirPath);
      const takenIds = files.filter(f => f.name.toLowerCase().endsWith('.json')).map(f => f.name.slice(0, -'.json'.length));
      const profile: SettingsProfile = {
        id: profileIdFromName(name, takenIds),
        name: name.trim(),
        settings
      };
      await this.writeProfileFile(profile);
      return profile;
    });
  }

  // Writes the current settings into the active profile.
  public async saveActive() {
    const profile = this.activeProfile();
    const values = this.dss.values();
    if (!profile || !values) {
      return;
    }
    await this.writeProfile({ ...profile, settings: profileSettings(values, this.defaults()) });
  }

  // True when selecting the profile would turn packed foveation on, so the caller can warn first.
  public selectTurnsOnPackedFoveation(id: string): boolean {
    const profile = this.profiles().find(p => sameId(p.id, id));
    return !!profile && turnsOnPackedFoveation(fullSettings(profile.settings, this.defaults()), this.dss.values());
  }

  // Writes the profile's settings to settings.json and makes it the active profile.
  public async select(id: string) {
    const profile = this.profiles().find(p => sameId(p.id, id));
    const current = this.dss.values();
    if (!profile || !current) {
      return;
    }
    const next = keepHeadsetEnableFlags(fullSettings(profile.settings, this.defaults()), current);
    await this.dss.save(next);
    await this.saveAppSettings({ activeProfile: profile.id });
  }

  // Creates a profile from the driver defaults and selects it. Headsets stay enabled or disabled as they are now.
  public async createFromDefaults(name: string) {
    const settings = keepHeadsetEnableFlags(cloneJson(this.defaults() ?? {} as Settings), this.dss.values());
    const profile = await this.writeNewProfile(name, profileSettings(settings, this.defaults()));
    await this.select(profile.id);
  }

  // Creates a profile from the current settings, including unsaved changes, and selects it.
  public async createFromCurrent(name: string) {
    const values = this.dss.values();
    if (!values) {
      throw new Error($localize`settings.json could not be read`);
    }
    const profile = await this.writeNewProfile(name, profileSettings(values, this.defaults()));
    await this.saveAppSettings({ activeProfile: profile.id });
  }

  // Deletes the profile file. Deleting the active profile leaves settings.json as it is.
  public async delete(id: string) {
    await this.withFolderLock(async () => {
      const path = await this.profilePath(id);
      if (await exists(path)) {
        await remove(path);
      }
      this._profiles.set(this._profiles().filter(p => !sameId(p.id, id)));
    });
    if (sameId(this.activeId(), id)) {
      await this.saveAppSettings({ activeProfile: '' });
    }
  }
}
