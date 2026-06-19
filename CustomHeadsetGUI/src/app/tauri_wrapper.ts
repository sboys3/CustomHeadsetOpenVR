import { invoke } from '@tauri-apps/api/core';

export async function get_executable_path() {
    return await invoke('get_executable_path') as string;

}
export async function is_vrmonitor_running() {
    return await invoke('is_vrmonitor_running') as boolean;
}
export async function restart_vrcompositor() {
    return await invoke('restart_vrcompositor') as boolean;
}
export async function kill_process(process_name: string): Promise<boolean> {
    return await invoke('kill_process', { processName: process_name }) as boolean;
}
export async function launch_process(path: string, args: string[]): Promise<boolean> {
    return await invoke('launch_process', { path, args }) as boolean;
}
export async function run_process_sync(path: string, args: string[]): Promise<number> {
    return await invoke('run_process_sync', { path, args }) as number;
}

export async function print_string(message: string): Promise<void> {
    await invoke('print_string', { message });
}

// Expose on global window for direct access
declare global {
    interface Window {
        printString: (message: string) => Promise<void>;
    }
}

window.printString = print_string;