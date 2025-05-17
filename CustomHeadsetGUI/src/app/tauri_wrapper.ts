
import { invoke } from '@tauri-apps/api/core';

export async function get_executable_path() {
    return await invoke('get_executable_path') as string;

}