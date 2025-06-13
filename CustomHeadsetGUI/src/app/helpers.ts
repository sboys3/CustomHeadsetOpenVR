import { copyFile, create, exists, remove, rename, writeTextFile } from "@tauri-apps/plugin-fs";
import { Subject, throttleTime } from "rxjs";
import { join } from '@tauri-apps/api/path';

export async function delay(ms: number = 1) {
  return new Promise<void>(done => {
    setTimeout(done, ms);
  })
}

export type DebouncedFileWriter = {
  save: (content: string) => void;
  isSavingFile: () => boolean;
};
export function cleanJsonComments(jsonString: string) {
  return jsonString.replace(/\\"|"(?:\\"|[^"])*"|(\/\/.*|\/\*[\s\S]*?\*\/)/g, (m, g) => g ? "" : m)
}

/**
 * Deeply compares two values, specifically performing content comparison for arrays.
 * This is used to check for exclusion at the leaf level.
 *
 * @param val1 - The first value.
 * @param val2 - The second value.
 * @returns True if the values are equal (arrays are compared by content), false otherwise.
 */
const areValuesEqual = (val1: any, val2: any): boolean => {
  if (val1 === val2) {
    return true;
  }

  if (Array.isArray(val1) && Array.isArray(val2)) {
    if (val1.length !== val2.length) {
      return false;
    }
    for (let i = 0; i < val1.length; i++) {
      if (!areValuesEqual(val1[i], val2[i])) {
        return false;
      }
    }
    return true;
  }

  return false;
};


/**
 * Deep copies an object, excluding properties at the leaf level (non-object values)
 * if their values are equal to the corresponding values in a specified exclusion object.
 * Arrays are compared by content for exclusion. Intermediate objects are always copied,
 * even if all their children are excluded, resulting in an empty object for that property.
 * The second parameter excludeObj is optional. If not provided, a simple deep copy is performed.
 *
 * @template T - The type of the object to copy.
 * @param obj - The source object to deep copy.
 * @param excludeObj - (Optional) The object used for exclusion. If a non-object property value
 * in the source object is equal to the corresponding property value in this object (arrays
 * are compared by content), that property will be excluded. Defaults to an empty object {}.
 * @returns The deep copied object, with leaf properties excluded if their values matched in excludeObj.
 */
export const deepCopy = <T>(obj: T, excludeObj: Partial<T> = {}): T => {
  if (typeof obj !== 'object' || obj === null) {
    if(areValuesEqual(obj, excludeObj)){
      return undefined as unknown as T;
    }
    return obj;
  }

  if (Array.isArray(obj)) {
    if (Array.isArray(excludeObj)) {
      if (areValuesEqual(obj, excludeObj)) {
        return undefined as unknown as T;
      }
      let outputArray = [];
      for (let i = 0; i < obj.length && i < excludeObj.length; i++) {
        outputArray.push(deepCopy(obj[i], excludeObj[i]));
      }
      return outputArray as unknown as T;
    }
    return obj;
  }

  const copy = {} as { [K in keyof T]: T[K] };
  Object.keys(obj).forEach(key => {
    const objValue = (obj as { [key: string]: any })[key];
    const excludeValue = (excludeObj as { [key: string]: any })[key];

    const relevantExcludeForValue = (typeof excludeValue === 'object' && excludeValue !== null && !Array.isArray(excludeValue))
      ? excludeValue
      : {};

    const copiedValue = deepCopy(objValue, relevantExcludeForValue);

    const isLeafValue = typeof objValue !== 'object' || objValue === null || Array.isArray(objValue);

    if (isLeafValue && Object.prototype.hasOwnProperty.call(excludeObj, key)) {
      if (areValuesEqual(objValue, excludeValue)) {
        return;
      }
    }

    copy[key as keyof T] = copiedValue;
  });

  return copy;
};
/**
 * @description Defines an interface to check if an item is an object.
 */
interface IIsObject {
  (item: any): boolean;
}

/**
 * @description Defines a generic object interface to constrain generic parameters to be object types.
 */
interface IObject {
  [key: string]: any;
}

/**
 * @description Method to check if an item is an object. Date and Function are considered
 * an object, so if you need to exclude those, please update the method accordingly.
 * @param item - The item that needs to be checked
 * @return {Boolean} Whether or not @item is an object
 */
export const isObject: IIsObject = (item: any): boolean => {
  return (item === Object(item) && !Array.isArray(item));
};

/**
 * @description Method to perform a deep merge of objects.
 *
 * @template TX The type of the target object.
 * @template TY The type of elements in the source objects array.
 * @template TR The type of the merged object, defaults to the intersection of TX and TY (TX & TY) to represent the combined type.
 *
 * @param target - The targeted object that needs to be merged with the supplied @sources
 * @param sources - The source(s) that will be used to update the @target object
 * @return {TR} The final merged object (which is the modified target object)
 */
export const deepMerge = <TX extends IObject, TY extends IObject, TR = TX & TY>(target: TX, ...sources: Array<TY>): TR => {
  if (!sources.length) {
    return target as any as TR;
  }

  const result: IObject = target;

  if (isObject(result)) {
    const len: number = sources.length;
    for (let i = 0; i < len; i += 1) {
      const elm: any = sources[i];
      if (isObject(elm)) {
        for (const key in elm) {
          if (Object.prototype.hasOwnProperty.call(elm, key)) {
            if (isObject(elm[key])) {
              if (!result[key] || !isObject(result[key])) {
                result[key] = {};
              }
              deepMerge(result[key], elm[key]);
            } else {
              if (Array.isArray(result[key]) && Array.isArray(elm[key])) {
                if(typeof elm[key][0] === 'string'){
                  // merge arrays of strings
                  result[key] = Array.from(new Set(result[key].concat(elm[key])));
                }else{
                  // overwrite if the array otherwise as numbers likely won't merge
                  result[key] = elm[key];
                }
              } else {
                result[key] = elm[key];
              }
            }
          }
        }
      }
    }
  }
  return result as TR;
};
export function debouncedFileWriter(path: string | Promise<string>, tempFileDir: string | Promise<string>, directWrite?: (() => boolean)): DebouncedFileWriter {
  const saveSbj = new Subject<string>();
  let savingFile = false
  saveSbj.pipe(throttleTime(50, undefined, { trailing: true })).subscribe(async (content) => {
    await navigator.locks.request(`saving file_${await path}`, async () => {
      savingFile = true;
      try {
        if (directWrite?.()) {
          try {
            await writeTextFile(await path, content);
          } catch (er) {
            console.error('write file error', await path)
          }
        } else {
          const tempPath = await join(await tempFileDir, `${self.crypto.randomUUID()}`);
          await writeTextFile(tempPath, content);
          try {
            await copyFile(tempPath, await path);
          } catch (er) {
            console.error('copy file error', tempPath, 'to', await path)
          } finally {
            await remove(tempPath);
          }
        }
      } finally {
        savingFile = false;
      }
    })
  });
  return {
    save: (content: string) => {
      saveSbj.next(content)
    },
    isSavingFile: () => savingFile
  }
}
export function isNewVersion(current: string, latest: string): boolean {
  if(current.startsWith('v') || current.startsWith('V')){
    current = current.substring(1);
  }
  if(latest.startsWith('v') || latest.startsWith('V')){
    latest = latest.substring(1);
  }
  const cParts = current.split(/[\.-]/g);
  const lParts = latest.split(/[\.-]/g);
  const maxLength = Math.max(cParts.length, lParts.length);
  for (let i = 0; i < maxLength; i++) {
    const cNum = parseInt(cParts[i] || '0', 10);
    const lNum = parseInt(lParts[i] || '0', 10);
    if(isNaN(parseInt(cParts[i])) && cParts[i] || isNaN(parseInt(lParts[i])) && lParts[i]){
      // compare strings
      if (!lParts[i]){
        // if the text is missing from the latest then it is no longer a pre-release
        return true;
      }
      if (!cParts[i]){
        // if the text is missing from the current then latest it an older pre-release
        return false;
      }
      if (lParts[i] > cParts[i]) {
        return true;
      }
      if (cParts[i] > lParts[i]) {
        return false;
      }
      continue
    }
    
    if (lNum > cNum) {
      return true;
    }
    if (cNum > lNum) {
      return false;
    }
  }
  return false;
}

function testIsNewVersion() {
  function test(current: string, latest: string, expected: boolean) {
    const result = isNewVersion(current, latest);
    console.log(`isNewVersion("${current}", "${latest}") = ${result} (expected: ${expected})`);
    if (result !== expected) {
      console.error(`Test failed`);
    }
  }
  test('1.0.0', '1.0.1', true);
  test('1.0.1', '1.0.0', false);
  test('1.0.0', '1.0.0', false);
  test('1.0.0', '1.1.0', true);
  test('1.1.0', '1.0.0', false);
  test('1.0.0', '2.0.0', true);
  test('2.0.0', '1.0.0', false);
  test('1.0.0-alpha', '1.0.0', true);
  test('1.0.0', '1.0.0-alpha', false);
  test('1.0.0-alpha', '1.0.0-beta', true);
  test('1.0.0-beta', '1.0.0-alpha', false);
  test('1.0.0-beta', '1.0.0-beta', false);
  test('1.0.0-beta.1', '1.0.0-beta.2', true);
  test('1.0.0-beta.2', '1.0.0-beta.1', false);
  test('1.0.0-beta.2', '1.0.0-beta.2', false);
  test('1.0.0-beta.2', '1.0.0-beta', false);
  test('1.0.0-beta', '1.0.0-beta.2', true);
  test('1.0.0-beta.2', '1.0.0', true);
  test('1.0.0', '1.0.0-beta.2', false);
  test('1.0.0', '1.0.1-beta.2', true);
  test('1.0.1-beta.2', '1.0.0', false);
  test('1.0.0', '1.0.1-beta.2', true);
  test('1.0.1-beta', '1.0.0', false);
  test('1.0.0', '1.0.1-beta', true);
  test('1.0', '1.0.1', true);
  test('1', '1.0.1', true);
  test('1', '2', true);
  test('1', '1', false);
  test('1.0.0', '1.0.0.1', true);
  test('1.0.0.1', '1.0.2', true);
  test('1.0.1', '1.0.0.1', false);
  test('v1.0.1', '1.0.2', true);
  test('v1.0.1', 'v1.0.2', true);
  test('1.0.1', 'v1.0.2', true);
  test('v1.0.1', '1.0.1', false);
  test('1.0.1', 'v1.0.1', false);
}
// testIsNewVersion();