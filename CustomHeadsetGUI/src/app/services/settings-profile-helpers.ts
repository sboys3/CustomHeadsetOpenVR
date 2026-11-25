// Profile names become file names, so these characters and names are not allowed on Windows.
const invalidFileNameChars = /[<>:"/\\|?*\x00-\x1f]/g;
// Windows also reserves these names with any extension, such as "con.txt"
const reservedFileNames = /^(con|prn|aux|nul|com[0-9]|lpt[0-9]) *(\.|$)/i;
export const maxProfileNameLength = 64;

// Returns an error message, or undefined when the name is valid and unused.
export function validateProfileName(name: string, existingNames: string[]): string | undefined {
  const trimmed = name.trim();
  if (!trimmed) {
    return $localize`Enter a name`;
  }
  if (trimmed.length > maxProfileNameLength) {
    return $localize`Use at most ${maxProfileNameLength} characters`;
  }
  const lower = trimmed.toLowerCase();
  if (existingNames.some(n => n.trim().toLowerCase() == lower)) {
    return $localize`A profile with this name already exists`;
  }
  return undefined;
}

// Builds a file name stem from a profile name that does not collide with existingIds (compared case-insensitively).
export function profileIdFromName(name: string, existingIds: string[]): string {
  let base = name.trim().replace(invalidFileNameChars, '_').replace(/[. ]+$/, '');
  if (!base || reservedFileNames.test(base)) {
    base = `profile_${base}`;
  }
  const taken = new Set(existingIds.map(x => x.toLowerCase()));
  let id = base;
  for (let i = 2; taken.has(id.toLowerCase()); i++) {
    id = `${base}-${i}`;
  }
  return id;
}

export function isPlainObject(value: any): boolean {
  return typeof value === 'object' && value !== null && !Array.isArray(value);
}

export function cloneJson<T>(value: T): T {
  return value === undefined ? value : JSON.parse(JSON.stringify(value));
}

function sparseValue(value: any, defaultValue: any): any {
  if (isPlainObject(value)) {
    const result: any = {};
    for (const key of Object.keys(value).sort()) {
      const child = sparseValue(value[key], isPlainObject(defaultValue) ? defaultValue[key] : undefined);
      if (child !== undefined) {
        result[key] = child;
      }
    }
    return Object.keys(result).length ? result : undefined;
  }
  // arrays are stored whole, so a profile never holds a partial array
  if (JSON.stringify(value) === JSON.stringify(defaultValue)) {
    return undefined;
  }
  return cloneJson(value);
}

// Returns only the values that differ from the defaults, with sorted keys and no empty objects.
export function sparseSettings<T>(values: T | undefined, defaults: T | undefined): Partial<T> {
  return sparseValue(values, defaults) ?? {};
}

function mergeValue(base: any, override: any): any {
  if (override === undefined) {
    return cloneJson(base);
  }
  if (isPlainObject(base) && isPlainObject(override)) {
    const result: any = cloneJson(base);
    for (const key of Object.keys(override)) {
      result[key] = mergeValue(base[key], override[key]);
    }
    return result;
  }
  // arrays and leaf values replace the default, matching how sparseSettings stores them
  return cloneJson(override);
}

// Returns a full settings object: the defaults with the sparse values merged on top. Neither input changes.
export function fullSettings<T>(sparse: Partial<T> | undefined, defaults: T | undefined): T {
  return mergeValue(defaults ?? {}, sparse ?? {});
}

// The keys a profile never changes in a headset section, so switching profiles keeps the same headsets enabled.
const keptHeadsetKeys = ['enable', 'forceEnable'];
// Top-level sections that have an "enable" key but are not headsets.
const nonHeadsetSections = ['customShader'];

function headsetSections(settings: any): string[] {
  return Object.keys(settings ?? {}).filter(s => !nonHeadsetSections.includes(s) && isPlainObject(settings[s]));
}

// Copies the headset enable flags from current into target and returns target.
export function keepHeadsetEnableFlags<T>(target: T, current: T | undefined): T {
  const t = target as any;
  const c = current as any;
  for (const section of headsetSections(c)) {
    for (const key of keptHeadsetKeys) {
      if (key in c[section]) {
        if (!isPlainObject(t[section])) {
          t[section] = {};
        }
        t[section][key] = c[section][key];
      }
    }
  }
  return target;
}

export function withoutHeadsetEnableFlags<T>(settings: T): T {
  const result = cloneJson(settings) as any;
  for (const section of headsetSections(result)) {
    for (const key of keptHeadsetKeys) {
      delete result[section][key];
    }
  }
  return result;
}

// Returns the settings as a profile stores them: only values that differ from the defaults, without headset enable flags.
export function profileSettings<T>(values: T | undefined, defaults: T | undefined): Partial<T> {
  return sparseSettings(values === undefined ? undefined : withoutHeadsetEnableFlags(values), defaults);
}

// True when next turns packed foveation on and current has it off.
export function turnsOnPackedFoveation(
  next: { customShader?: { enablePackedFoveation?: boolean } } | undefined,
  current: { customShader?: { enablePackedFoveation?: boolean } } | undefined): boolean {
  return next?.customShader?.enablePackedFoveation === true && current?.customShader?.enablePackedFoveation !== true;
}

// Compares full settings objects as a profile would store them, ignoring the headset enable flags.
export function settingsMatch<T>(a: T | undefined, b: T | undefined, defaults: T | undefined): boolean {
  if (!a || !b) {
    return a === b;
  }
  const sparseA = sparseSettings(withoutHeadsetEnableFlags(a), defaults);
  const sparseB = sparseSettings(withoutHeadsetEnableFlags(b), defaults);
  return JSON.stringify(sparseA) == JSON.stringify(sparseB);
}
