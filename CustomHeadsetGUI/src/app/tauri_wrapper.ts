
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