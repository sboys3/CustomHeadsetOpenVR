import { delay } from "../helpers";

export class PullingService {
    private running = false
    private currentTask?: Promise<any>
    private id = self.crypto.randomUUID()
    constructor(private task: () => Promise<any> | void, private name: string | undefined = undefined, private interval = 1000) {

    }
    public async update() {
        await this.task();
    }
    private async pulling() {
        if (this.name) {
            console.log('PullingService', this.name, this.id, 'start')
        }
        while (this.running) {
            this.update()
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
}