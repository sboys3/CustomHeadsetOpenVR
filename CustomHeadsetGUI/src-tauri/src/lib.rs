use std::path::PathBuf;
use tauri::Manager;
use tauri_plugin_fs::FsExt;
mod i18n;
#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_fs::init())
        .setup(|app| {
            let app_data_path = app.path().app_data_dir().unwrap();
            let driver_app_data_dir: PathBuf = app_data_path.join(r"..\CustomHeadset");
            let fs_scope = app.fs_scope();
            let _ = fs_scope.allow_directory(&driver_app_data_dir, true);
            dbg!(fs_scope.allowed_patterns());
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
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
