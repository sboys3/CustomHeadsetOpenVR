
import { signal } from '@angular/core';
import { exists, mkdir, readTextFile, writeTextFile } from '@tauri-apps/plugin-fs';
import { cleanJsonComments, DebouncedFileWriter, debouncedFileWriter, deepCopy, deepMerge } from '../helpers';
export abstract class JsonSettingServiceBase<T> {
    protected _values = signal<T | undefined>(undefined);
    public values = this._values.asReadonly();
    protected readonly debouncedFileWriter: DebouncedFileWriter;
    protected initTask: Promise<void>;
    protected defaults!: T;
    constructor(private filePath: string, private fileDir: string, private defaultValue: () => Promise<T> | T, private autoCreate: boolean) {
        this.debouncedFileWriter = debouncedFileWriter(filePath, fileDir);
        this.initTask = this.init();
    }
    protected async init() {
        if (!await exists(this.fileDir)) {
            await mkdir(this.fileDir);
        }
        if (!await exists(this.filePath) && this.autoCreate) {
            await writeTextFile(this.filePath, JSON.stringify("{}"));
        }
        await this.loadSetting();
    }
     
    async loadSetting() {
        if (await exists(this.filePath)) {
            this.defaults = await this.defaultValue();
            const copiedDefaults = deepCopy(this.defaults);
            this._values.set(deepMerge(copiedDefaults as any, JSON.parse(cleanJsonComments(await readTextFile(this.filePath)))));
        } else {
            this._values.set(undefined)
        }
    }

    async save(values: T) {
        await this.initTask;
        this.debouncedFileWriter.save(JSON.stringify(deepCopy(values, this.defaults), undefined, 4));
        this._values.set(Object.assign({}, values))
    }
}