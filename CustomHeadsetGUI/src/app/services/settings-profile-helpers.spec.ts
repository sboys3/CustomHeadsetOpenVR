import {
  cloneJson,
  fullSettings,
  keepHeadsetEnableFlags,
  maxProfileNameLength,
  profileIdFromName,
  profileSettings,
  settingsMatch,
  sparseSettings,
  turnsOnPackedFoveation,
  validateProfileName,
} from './settings-profile-helpers';

describe('validateProfileName', () => {
  it('rejects empty and whitespace names', () => {
    expect(validateProfileName('', [])).toBeDefined();
    expect(validateProfileName('   \t\n  ', [])).toBeDefined();
  });

  it('rejects names longer than maxProfileNameLength after trimming', () => {
    const tooLong = 'a'.repeat(maxProfileNameLength + 1);
    expect(validateProfileName(tooLong, [])).toBeDefined();

    const exact = 'a'.repeat(maxProfileNameLength);
    expect(validateProfileName(exact, [])).toBeUndefined();
    expect(validateProfileName(`  ${exact}  `, [])).toBeUndefined();
  });

  it('rejects duplicate names regardless of case or surrounding whitespace', () => {
    expect(validateProfileName('Sim Racing', ['sim racing'])).toBeDefined();
    expect(validateProfileName('  SIM RACING  ', ['  Sim Racing '])).toBeDefined();
  });

  it('returns undefined for a valid unused name', () => {
    expect(validateProfileName('Flight Sim', ['Default', 'Sim Racing'])).toBeUndefined();
  });
});

describe('profileIdFromName', () => {
  it('replaces Windows-invalid and control characters with underscores', () => {
    expect(profileIdFromName('a<b>c:d"e/f\\g|h?i*j', [])).toBe('a_b_c_d_e_f_g_h_i_j');
    expect(profileIdFromName('a\x00b\x07c\x1fd', [])).toBe('a_b_c_d');
  });

  it('removes trailing dots and spaces', () => {
    expect(profileIdFromName('Racing...  ', [])).toBe('Racing');
    expect(profileIdFromName('Sim . . ', [])).toBe('Sim');
  });

  it('adds a profile_ prefix to Windows reserved device names in any case', () => {
    for (const reserved of ['CON', 'con', 'Prn', 'AUX', 'nul', 'COM1', 'com1', 'LPT9', 'lpt9']) {
      expect(profileIdFromName(reserved, [])).toBe(`profile_${reserved}`);
    }
    expect(profileIdFromName('CON...', [])).toBe('profile_CON');
  });

  it('adds a profile_ prefix to reserved device names with an extension', () => {
    expect(profileIdFromName('con.txt', [])).toBe('profile_con.txt');
    expect(profileIdFromName('LPT1 .cfg', [])).toBe('profile_LPT1 .cfg');
    expect(profileIdFromName('console', [])).toBe('console');
  });

  it('adds a profile_ prefix when the cleaned name is empty', () => {
    expect(profileIdFromName('', [])).toBe('profile_');
    expect(profileIdFromName('   ', [])).toBe('profile_');
    expect(profileIdFromName('...', [])).toBe('profile_');
  });

  it('resolves collisions case-insensitively with numbered suffixes', () => {
    expect(profileIdFromName('Sim', ['sim'])).toBe('Sim-2');
    expect(profileIdFromName('Sim', ['SIM', 'sim-2'])).toBe('Sim-3');
    expect(profileIdFromName('Sim', ['SIM', 'sim-2', 'SIM-3'])).toBe('Sim-4');
  });
});

describe('cloneJson', () => {
  it('returns undefined when given undefined', () => {
    expect(cloneJson(undefined)).toBeUndefined();
  });

  it('deep-copies nested objects and arrays without sharing references', () => {
    const source: any = { meganeX8K: { ipd: 63, subpixelOffsets: [0, 0.33, -0.33] } };
    const copy = cloneJson(source);

    expect(copy).toEqual(source);
    copy.meganeX8K.ipd = 65;
    copy.meganeX8K.subpixelOffsets[0] = 1;
    expect(source.meganeX8K.ipd).toBe(63);
    expect(source.meganeX8K.subpixelOffsets[0]).toBe(0);
  });
});

describe('sparseSettings', () => {
  it('drops leaf values that equal defaults and keeps changed leaves', () => {
    const defaults: any = {
      forceTracking: false,
      meganeX8K: { ipd: 63, maxFovX: 100, colorMultiplier: { r: 1, g: 1, b: 1 } },
    };
    const values: any = {
      forceTracking: false,
      meganeX8K: { ipd: 65, maxFovX: 100, colorMultiplier: { r: 1, g: 0.9, b: 1 } },
    };

    expect(sparseSettings(values, defaults)).toEqual({
      meganeX8K: { ipd: 65, colorMultiplier: { g: 0.9 } },
    });
  });

  it('keeps an array whole when any element differs and drops it when equal', () => {
    const defaults: any = {
      meganeX8K: { subpixelOffsets: [0, 0.33, -0.33] },
      customShader: { srgbColorCorrectionMatrix: [1, 0, 0, 0, 1, 0, 0, 0, 1] },
    };
    const values: any = {
      meganeX8K: { subpixelOffsets: [0, 0.33, -0.33] },
      customShader: { srgbColorCorrectionMatrix: [1, 0, 0, 0, 0.95, 0, 0, 0, 1] },
    };

    const result: any = sparseSettings(values, defaults);
    expect(result).toEqual({
      customShader: { srgbColorCorrectionMatrix: [1, 0, 0, 0, 0.95, 0, 0, 0, 1] },
    });

    // Returned arrays are cloned so later edits do not mutate the input.
    result.customShader.srgbColorCorrectionMatrix[0] = 2;
    expect(values.customShader.srgbColorCorrectionMatrix[0]).toBe(1);
  });

  it('removes nested objects when all leaves equal defaults or the object is empty', () => {
    const defaults: any = {
      meganeX8K: { ipd: 63, hiddenArea: { enable: false, detailLevel: 1 } },
    };
    const values: any = {
      meganeX8K: { ipd: 63, hiddenArea: { enable: false, detailLevel: 1 }, extra: {} },
    };

    expect(sparseSettings(values, defaults)).toEqual({});
  });

  it('sorts keys recursively so JSON output ignores insertion order', () => {
    const defaults: any = {
      a: { x: 0, y: 0 },
      b: { m: 0, n: 0 },
    };
    const first: any = {
      b: { n: 2, m: 1 },
      a: { y: 4, x: 3 },
    };
    const second: any = {
      a: { x: 3, y: 4 },
      b: { m: 1, n: 2 },
    };

    expect(JSON.stringify(sparseSettings(first, defaults))).toBe(
      JSON.stringify(sparseSettings(second, defaults))
    );
  });

  it('returns an empty object when values is undefined', () => {
    const defaults: any = { forceTracking: false, meganeX8K: { ipd: 63 } };
    expect(sparseSettings(undefined, defaults)).toEqual({});
  });

  it('keeps all non-empty values when defaults is undefined', () => {
    const values: any = {
      forceTracking: true,
      meganeX8K: { ipd: 63, subpixelOffsets: [0, 0.5] },
    };

    expect(sparseSettings(values, undefined)).toEqual(values);
  });
});

describe('fullSettings', () => {
  it('merges sparse overrides onto defaults', () => {
    const defaults: any = {
      forceTracking: false,
      meganeX8K: {
        ipd: 63,
        maxFovX: 100,
        subpixelOffsets: [0, 0.33, -0.33],
        colorMultiplier: { r: 1, g: 1, b: 1 },
      },
    };
    const sparse: any = {
      meganeX8K: {
        ipd: 65,
        subpixelOffsets: [0.1, 0.2, -0.2],
        colorMultiplier: { g: 0.9 },
      },
    };

    expect(fullSettings(sparse, defaults)).toEqual({
      forceTracking: false,
      meganeX8K: {
        ipd: 65,
        maxFovX: 100,
        subpixelOffsets: [0.1, 0.2, -0.2],
        colorMultiplier: { r: 1, g: 0.9, b: 1 },
      },
    });
  });

  it('does not mutate sparse or defaults and returns an independent copy', () => {
    const defaults: any = {
      meganeX8K: { ipd: 63, colorMultiplier: { r: 1, g: 1, b: 1 } },
    };
    const sparse: any = {
      meganeX8K: { colorMultiplier: { r: 0.8 }, subpixelOffsets: [0.1, 0.2] },
    };
    const defaultsBefore = JSON.stringify(defaults);
    const sparseBefore = JSON.stringify(sparse);

    const merged: any = fullSettings(sparse, defaults);
    expect(JSON.stringify(defaults)).toBe(defaultsBefore);
    expect(JSON.stringify(sparse)).toBe(sparseBefore);

    merged.meganeX8K.ipd = 70;
    merged.meganeX8K.colorMultiplier.r = 0.5;
    merged.meganeX8K.subpixelOffsets[0] = 9;
    expect(JSON.stringify(defaults)).toBe(defaultsBefore);
    expect(JSON.stringify(sparse)).toBe(sparseBefore);
  });

  it('replaces string arrays instead of joining them with the defaults', () => {
    expect(fullSettings<any>({ list: ['b'] }, { list: ['a', 'b'] })).toEqual({ list: ['b'] });
  });

  it('handles undefined sparse or defaults', () => {
    const defaults: any = { forceTracking: false, meganeX8K: { ipd: 63 } };
    const sparse: any = { meganeX8K: { ipd: 66 } };

    expect(fullSettings(undefined, defaults)).toEqual(defaults);
    expect(fullSettings(sparse, undefined)).toEqual(sparse);
    expect(fullSettings<any>(undefined, undefined)).toEqual({});
  });
});

describe('keepHeadsetEnableFlags', () => {
  it('copies enable and forceEnable from top-level headset sections and skips customShader', () => {
    const target: any = {
      meganeX8K: {
        enable: false,
        forceEnable: false,
        ipd: 64,
        hiddenArea: { enable: true },
      },
      crystalSuperUltrawide: {
        enable: true,
        forceEnable: true,
        maxFovOuter: 72,
      },
      customShader: {
        enable: true,
        contrast: 1.2,
      },
    };
    const current: any = {
      meganeX8K: {
        enable: true,
        forceEnable: true,
        ipd: 61,
        hiddenArea: { enable: false },
      },
      crystalSuperUltrawide: {
        enable: false,
        forceEnable: false,
        maxFovOuter: 60,
      },
      customShader: {
        enable: false,
        contrast: 0.8,
      },
    };

    const result = keepHeadsetEnableFlags(target, current);
    expect(result).toBe(target);
    expect(result).toEqual({
      meganeX8K: {
        enable: true,
        forceEnable: true,
        ipd: 64,
        hiddenArea: { enable: true },
      },
      crystalSuperUltrawide: {
        enable: false,
        forceEnable: false,
        maxFovOuter: 72,
      },
      customShader: {
        enable: true,
        contrast: 1.2,
      },
    });
  });

  it('creates missing target sections only when current has enable or forceEnable', () => {
    const target: any = { forceTracking: true };
    const current: any = {
      dreamAir: { enable: true, ipd: 62 },
      generalHeadset: { useViveBluetooth: true },
      forceTracking: false,
      extraList: [1, 2],
      emptyRef: null,
    };

    expect(keepHeadsetEnableFlags(target, current)).toEqual({
      forceTracking: true,
      dreamAir: { enable: true },
    });
  });

  it('returns target unchanged when current is undefined', () => {
    const target: any = {
      meganeX8K: { enable: true, forceEnable: false, ipd: 63 },
      customShader: { enable: true },
    };

    const result = keepHeadsetEnableFlags(target, undefined);
    expect(result).toBe(target);
    expect(result).toEqual({
      meganeX8K: { enable: true, forceEnable: false, ipd: 63 },
      customShader: { enable: true },
    });
  });
});

describe('settingsMatch', () => {
  const defaults: any = {
    forceTracking: false,
    crystalSuperUltrawide: {
      enable: false,
      forceEnable: false,
      maxFovOuter: 70,
      hiddenArea: { enable: false },
    },
    customShader: {
      enable: false,
      contrast: 1,
    },
  };

  it('returns true for equal settings and false when a FOV value differs', () => {
    const a: any = {
      forceTracking: false,
      crystalSuperUltrawide: { enable: true, forceEnable: false, maxFovOuter: 75 },
      customShader: { enable: true, contrast: 1.1 },
    };
    const same: any = {
      forceTracking: false,
      crystalSuperUltrawide: { enable: true, forceEnable: false, maxFovOuter: 75 },
      customShader: { enable: true, contrast: 1.1 },
    };
    const differentFov: any = {
      forceTracking: false,
      crystalSuperUltrawide: { enable: true, forceEnable: false, maxFovOuter: 68 },
      customShader: { enable: true, contrast: 1.1 },
    };

    expect(settingsMatch(a, same, defaults)).toBeTrue();
    expect(settingsMatch(a, differentFov, defaults)).toBeFalse();
  });

  it('ignores differences in headset enable and forceEnable flags', () => {
    const a: any = {
      crystalSuperUltrawide: { enable: true, forceEnable: true, maxFovOuter: 75 },
      customShader: { enable: false, contrast: 1 },
    };
    const b: any = {
      crystalSuperUltrawide: { enable: false, forceEnable: false, maxFovOuter: 75 },
      customShader: { enable: false, contrast: 1 },
    };

    expect(settingsMatch(a, b, defaults)).toBeTrue();
    expect(a.crystalSuperUltrawide.enable).toBeTrue();
    expect(b.crystalSuperUltrawide.enable).toBeFalse();
  });

  it('ignores a headset enable flag that only one side has, in either argument order', () => {
    const flagDefaults: any = { meganeX8K: { enable: false } };
    expect(settingsMatch<any>({ meganeX8K: { enable: true } }, {}, flagDefaults)).toBeTrue();
    expect(settingsMatch<any>({}, { meganeX8K: { enable: true } }, flagDefaults)).toBeTrue();
  });

  it('returns false when customShader.enable or a nested enable flag differs', () => {
    const base: any = {
      crystalSuperUltrawide: { enable: true, maxFovOuter: 70, hiddenArea: { enable: false } },
      customShader: { enable: false, contrast: 1 },
    };
    const shaderEnabled: any = {
      crystalSuperUltrawide: { enable: true, maxFovOuter: 70, hiddenArea: { enable: false } },
      customShader: { enable: true, contrast: 1 },
    };
    const hiddenAreaEnabled: any = {
      crystalSuperUltrawide: { enable: true, maxFovOuter: 70, hiddenArea: { enable: true } },
      customShader: { enable: false, contrast: 1 },
    };

    expect(settingsMatch(base, shaderEnabled, defaults)).toBeFalse();
    expect(settingsMatch(base, hiddenAreaEnabled, defaults)).toBeFalse();
  });

  it('ignores object key insertion order', () => {
    const a: any = {
      customShader: { contrast: 1.2, enable: true },
      crystalSuperUltrawide: { maxFovOuter: 75, enable: true },
    };
    const b: any = {
      crystalSuperUltrawide: { enable: true, maxFovOuter: 75 },
      customShader: { enable: true, contrast: 1.2 },
    };

    expect(settingsMatch(a, b, defaults)).toBeTrue();
  });

  it('matches when a key equals the default on one side and is absent on the other', () => {
    const withDefault: any = {
      forceTracking: false,
      crystalSuperUltrawide: { enable: false, forceEnable: false, maxFovOuter: 75 },
      customShader: { enable: false, contrast: 1 },
    };
    const withMissing: any = {
      crystalSuperUltrawide: { enable: false, forceEnable: false, maxFovOuter: 75 },
      customShader: { enable: false },
    };

    expect(settingsMatch(withDefault, withMissing, defaults)).toBeTrue();
    expect(settingsMatch(withMissing, withDefault, defaults)).toBeTrue();
  });

  it('handles undefined inputs', () => {
    const value: any = { forceTracking: false };
    expect(settingsMatch(undefined, undefined, defaults)).toBeTrue();
    expect(settingsMatch(value, undefined, defaults)).toBeFalse();
    expect(settingsMatch(undefined, value, defaults)).toBeFalse();
  });
});

describe('profileSettings', () => {
  it('leaves out headset enable flags but keeps customShader.enable', () => {
    const defaults: any = { meganeX8K: { enable: true, forceEnable: false, ipd: 63 }, customShader: { enable: false } };
    const values: any = { meganeX8K: { enable: false, forceEnable: true, ipd: 66 }, customShader: { enable: true } };
    expect(profileSettings(values, defaults)).toEqual({ customShader: { enable: true }, meganeX8K: { ipd: 66 } } as any);
    expect(values.meganeX8K.enable).toBeFalse();
  });

  it('returns an empty object for undefined values', () => {
    expect(profileSettings<any>(undefined, {})).toEqual({});
  });
});

describe('turnsOnPackedFoveation', () => {
  it('is true only when next enables it and current does not', () => {
    const on = { customShader: { enablePackedFoveation: true } };
    const off = { customShader: { enablePackedFoveation: false } };
    expect(turnsOnPackedFoveation(on, off)).toBeTrue();
    expect(turnsOnPackedFoveation(on, {})).toBeTrue();
    expect(turnsOnPackedFoveation(on, undefined)).toBeTrue();
    expect(turnsOnPackedFoveation(on, on)).toBeFalse();
    expect(turnsOnPackedFoveation(off, on)).toBeFalse();
    expect(turnsOnPackedFoveation({}, off)).toBeFalse();
    expect(turnsOnPackedFoveation(undefined, off)).toBeFalse();
  });
});
