

use sysinfo::{ProcessesToUpdate, System, Process};
use std::process::Command;
use std::time::Duration;
use std::thread::sleep;
use tauri::command;

/// Returns the current platform name (e.g., "windows", "linux", "macos").
#[command]
pub fn get_platform() -> String {
    if cfg!(target_os = "windows") {
        "windows".to_string()
    } else if cfg!(target_os = "linux") {
        "linux".to_string()
    } else if cfg!(target_os = "macos") {
        "macos".to_string()
    } else {
        "unknown".to_string()
    }
}

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
        "vrmonitor"
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

#[command]
pub fn restart_vrcompositor() -> bool {
    let mut system = System::new_all();
    system.refresh_processes(ProcessesToUpdate::All, false);
    let target_compositor_process_name = if cfg!(target_os = "windows") {
        "vrcompositor.exe"
    } else if cfg!(target_os = "linux") {
        "vrcompositor"
    } else {
        return false;
    };
    
    let target_server_process_name = if cfg!(target_os = "windows") {
        "vrserver.exe"
    } else if cfg!(target_os = "linux") {
        "vrserver"
    } else {
        return false;
    };
    
    let mut compositor_process: Option<&Process> = None;
    let mut server_process: Option<&Process> = None;
    for (_pid, process) in system.processes() {
        if process.name().eq_ignore_ascii_case(target_compositor_process_name) {
            compositor_process = Some(process);
            continue;
        }
        if process.name().eq_ignore_ascii_case(target_server_process_name) {
            server_process = Some(process);
            continue;
        }
    }
    // kill and get path of current compositor
    let mut path = match compositor_process {
        None => None,
        Some(process) => {
            let path = process.exe();
            match path {
                None => None,
                Some(valid_path) => {
                    if !process.kill() {
                        println!("failed to kill vrcompositor");
                        return false;
                    }else{
                        println!("killed vrcompositor");
                        sleep(Duration::from_millis(1000));
                        Some(valid_path)
                    }
                }
            }
        },
    };
    
    // infer path from vrserver
    let compositor_path_buf;
    if path == None {
        path = match server_process {
            None => None,
            Some(process) => {
                let path = process.exe();
                match path {
                    None => None,
                    Some(path) => {
                        let parent = path.parent();
                        match parent {
                            None => None,
                            Some(parent) => {
                                compositor_path_buf = parent.join(target_compositor_process_name);
                                let compositor_path = compositor_path_buf.as_path();
                                Some(compositor_path)
                            }
                        }
                    }
                }
            }
        }
    }
    
    let path_unwraped = match path {
        None => {
            println!("Could not find vrcompositor path from running SteamVR instance");
            return false
        },
        Some(path) => path,
    };
    
    println!("Starting vrcompositor {}", path_unwraped.display());
    
    let mut new_process = Command::new(path_unwraped);
    let result = new_process.spawn();
    
    match result {
        Err(err) => {
            println!("Could not start vrcompositor {}", err);
            return false
        },
        Ok(_child) => return true,
    }
}

#[command]
pub fn is_process_running(process_name: String) -> bool {
    let mut system = System::new_all();
    system.refresh_processes(ProcessesToUpdate::All, false);
    
    for (_pid, process) in system.processes() {
        if process.name().eq_ignore_ascii_case(&process_name) {
            return true;
        }
    }
    false
}

#[command]
pub fn kill_process(process_name: String) -> bool {
    let mut system = System::new_all();
    system.refresh_processes(ProcessesToUpdate::All, false);
    
    let mut found = false;
    for (_pid, process) in system.processes() {
        if process.name().eq_ignore_ascii_case(&process_name) {
            found = true;
            if !process.kill() {
                println!("failed to kill {}", process_name);
            } else {
                println!("killed {}", process_name);
            }
        }
    }
    found
}

#[command]
pub fn launch_process(path: String, args: Vec<String>) -> bool {
    use std::path::PathBuf;
    
    let path_buf = PathBuf::from(path);
    
    println!("Launching {:?} with args {:?}", path_buf, args);
    
    match Command::new(&path_buf).args(&args).spawn() {
        Ok(_) => {
            println!("Successfully launched {:?}", path_buf);
            true
        }
        Err(e) => {
            println!("Failed to launch {:?}: {}", path_buf, e);
            false
        }
    }
}

#[command]
pub fn run_process_sync(path: String, args: Vec<String>) -> Result<i32, String> {
    use std::path::PathBuf;
    
    let path_buf = PathBuf::from(path);
    
    println!("Running {:?} with args {:?}", path_buf, args);
    
    match Command::new(&path_buf).args(&args).output() {
        Ok(output) => {
            println!("Finished {:?} with exit code {:?}", path_buf, output.status.code());
            Ok(output.status.code().unwrap_or(-1))
        }
        Err(e) => {
            println!("Failed to run {:?}: {}", path_buf, e);
            Err(e.to_string())
        }
    }
}

#[command]
pub fn print_string(message: String) {
    println!("{}", message);
}