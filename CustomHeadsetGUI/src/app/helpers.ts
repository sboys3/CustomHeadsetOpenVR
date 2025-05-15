import { copyFile, create, exists, remove, rename, writeTextFile } from "@tauri-apps/plugin-fs";
import { Subject, throttleTime } from "rxjs";
import { join } from '@tauri-apps/api/path';

export async function delay(ms = 1) {
    return new Promise<void>(done => {
        setTimeout(done, ms);
    })
}

export type DebouncedFileWriter = {
    save: (content: string) => void;
    isSavingFile: () => boolean;
};
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
    return obj;
  }

  if (Array.isArray(obj)) {
     if (Array.isArray(excludeObj) && areValuesEqual(obj, excludeObj)) {
         return obj;
     }
    return obj.map(item => deepCopy(item, excludeObj)) as unknown as T;
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
                result[key] = Array.from(new Set(result[key].concat(elm[key])));
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
    let fileSaveTask: Promise<void> | undefined;
    let savingFile = false
    saveSbj.pipe(throttleTime(50, undefined, { trailing: true })).subscribe((content) => {
        let task: Promise<void>
        if (fileSaveTask) {
            task = fileSaveTask;
        }
        fileSaveTask = new Promise(async done => {
            await task;
            savingFile = true;
            try {
                if (directWrite?.()) {
                    await writeTextFile(await path, content);
                } else {
                    const tempPath = await join(await tempFileDir, `${self.crypto.randomUUID()}`);
                    await writeTextFile(tempPath, content);
                    await copyFile(tempPath, await path);
                    await remove(tempPath);
                }
            } finally {
                savingFile = false;
            }
            done();
        });
    });
    return {
        save: (content: string) => {
            saveSbj.next(content)
        },
        isSavingFile: () => savingFile
    }
}