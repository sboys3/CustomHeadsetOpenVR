#[tauri::command]
pub fn get_executable_path() -> Result<String, String> {
    match std::env::current_exe() {
        Ok(path_buf) => {
            // PathBuf 轉換為 String
            // 我們通常會想要取得包含執行檔的目錄路徑，而不是執行檔本身的路徑
            // 如果你需要執行檔本身的完整路徑，可以直接 path_buf.to_str()
            match path_buf.parent() {
                Some(parent_dir) => Ok(parent_dir.to_string_lossy().into_owned()),
                None => Err("無法取得執行檔的父目錄".to_string()),
            }
            // 如果確實需要exe檔案本身的完整路徑:
            // Ok(path_buf.to_string_lossy().into_owned())
        }
        Err(e) => Err(format!("無法取得執行檔路徑: {}", e)),
    }
}