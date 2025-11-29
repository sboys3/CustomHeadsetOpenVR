mod i18n;
mod js_api;
#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_opener::init())
        .plugin(tauri_plugin_shell::init())
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_fs::init())
        .setup(|app| {
            if cfg!(debug_assertions) {
                app.handle().plugin(
                    tauri_plugin_log::Builder::default()
                        .level(log::LevelFilter::Info)
                        .build(),
                )?;
            }

            if cfg!(not(debug_assertions)) {
                i18n::load_i18n_page(app);
            }

            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            js_api::get_executable_path,
            js_api::is_vrmonitor_running,
            js_api::restart_vrcompositor,
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
