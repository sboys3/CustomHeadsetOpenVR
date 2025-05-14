import { create, exists, remove, rename } from "@tauri-apps/plugin-fs";
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

export function debouncedFileWriter(path: string | Promise<string>, tempFileDir: string| Promise<string>): DebouncedFileWriter {
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
                const tempPath = await join(await tempFileDir, `${self.crypto.randomUUID()}`);
                const temp = await create(tempPath);
                await temp.write(new TextEncoder().encode(content));
                await temp.close();
                if (await exists(await path)) {
                    await remove(await path);
                }
                await delay(10);
                await rename(tempPath, await path);
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