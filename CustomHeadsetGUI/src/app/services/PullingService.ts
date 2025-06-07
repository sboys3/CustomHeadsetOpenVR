import { delay } from "../helpers";

export class PullingService<T> {
    private running = false
    private currentTask?: Promise<any>
    private id = self.crypto.randomUUID()
    public readonly onUpdate = new Set<((update: T) => void)>()
    constructor(private task: () => Promise<T>, private name: string | undefined = undefined, private interval = 1000) {

    }
    public async update() {
        return await this.task();
    }
    private async pulling() {
        if (this.name) {
            console.log('PullingService', this.name, this.id, 'start')
        }
        while (this.running) {
            const result = await this.update()
            try {
                for (const upd of this.onUpdate) {
                    upd(result)
                }
            } catch (e) {
                console.error(e);

            }
            await delay(this.interval)
        }
        if (this.name) {
            console.log('PullingService', this.name, this.id, 'stop')
        }
    }
    public async start() {
        if (this.running) return;
        this.running = true;
        if (this.currentTask) {
            await this.currentTask;
        }
        this.currentTask = this.pulling()
    }
    public stop() {
        this.running = false;
        if (this.name && this.running) {
            console.log('PullingService', this.name, this.id, 'stop requested')
        }
    }

    public shared() {
        return new SharedPullingService(this)
    }
}
class SharedPullingService<T> {
    private pullingRequestRef = new Set<object>()
    constructor(private baseService: PullingService<T>) {
    }
    public createRef(onUpdate?: (values: T) => void) {
        const ref = {}
        const stop = () => {
            if (!this.pullingRequestRef.has(ref)) return;
            this.pullingRequestRef.delete(ref)
            if (onUpdate) {
                this.baseService.onUpdate.delete(onUpdate)
            }
            if (this.pullingRequestRef.size == 0) {
                this.baseService.stop()
            }
        }
        const start = async () => {
            if (this.pullingRequestRef.has(ref)) return;
            this.pullingRequestRef.add(ref)
            await this.baseService.start()
            if (onUpdate) {
                this.baseService.onUpdate.add(onUpdate)
            }
        }
        return {
            start,
            stop
        }
    }
}