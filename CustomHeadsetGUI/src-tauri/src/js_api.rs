#[tauri::command]
pub fn get_executable_path() -> Result<String, String> {
    match std::env::current_exe() {
        Ok(path_buf) => {
            match path_buf.parent() {
                Some(parent_dir) => Ok(parent_dir.to_string_lossy().into_owned()),
                None => Err("can not get dir path for exe".to_string()),
            }
        }
        Err(e) => Err(format!("fail to get path for exe: {}", e)),
    }
}