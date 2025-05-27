use sysinfo::{ProcessesToUpdate, System};
use tauri::command;

#[command]
pub fn get_executable_path() -> Result<String, String> {
    match std::env::current_exe() {
        Ok(path_buf) => match path_buf.parent() {
            Some(parent_dir) => Ok(parent_dir.to_string_lossy().into_owned()),
            None => Err("can not get dir path for exe".to_string()),
        },
        Err(e) => Err(format!("fail to get path for exe: {}", e)),
    }
}

#[command]
pub fn is_vrmonitor_running() -> bool {
    let mut system = System::new_all();
    system.refresh_processes(ProcessesToUpdate::All, false);

    let target_process_name = if cfg!(target_os = "windows") {
        "vrmonitor.exe"
    } else if cfg!(target_os = "linux") {
        "vrmonitor.sh"
    } else {
        return false;
    };

    for (_pid, process) in system.processes() {
        if process.name().eq_ignore_ascii_case(target_process_name) {
            return true;
        }
    }
    false
}
