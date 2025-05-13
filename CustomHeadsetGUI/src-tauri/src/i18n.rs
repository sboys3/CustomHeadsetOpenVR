use tauri::{App, Manager};
static SUPPORTED_LANGS: &[&str] = &["en-US", "zh-Hant", "ja"];
use locale_config::Locale;
fn determine_app_language_path(system_locale: &Locale) -> String {
    let system_locale_full_str_lower = system_locale.as_ref().to_lowercase();

    // Default to the first language in the list.
    let default_app_lang = SUPPORTED_LANGS[0];

    // 1. Direct check for an exact match.
    for app_lang in SUPPORTED_LANGS.iter() {
        if system_locale_full_str_lower == app_lang.to_lowercase() {
            return app_lang.to_string();
        }
    }

    // 2. No exact match => Split by '-' => Check for a prefix match with the primary language in the array.
    //    Extract the primary language part from system_locale_full_str_lower.
    let system_primary_lang_lower = match system_locale_full_str_lower.split('-').next() {
        Some(lang_code) if !lang_code.is_empty() => lang_code.to_string(), // Already lowercase.
        _ => return default_app_lang.to_string(), // If the system locale string is empty or invalid, fallback directly to default.
    };

    for app_lang in SUPPORTED_LANGS.iter() {
        let app_lang_lower = app_lang.to_lowercase();
        // Check if app_lang is identical to system_primary_lang_lower (e.g., "ja" vs "ja").
        // Or if app_lang starts with "system_primary_lang_lower-" (e.g., "zh-hant" vs "zh-").
        if app_lang_lower == system_primary_lang_lower
            || app_lang_lower.starts_with(&(system_primary_lang_lower.clone() + "-"))
        {
            return app_lang.to_string();
        }
    }

    // 3. If still no match => Load the first one (default).
    default_app_lang.to_string()
}
pub fn load_i18n_page(app: &App) {
    match app.get_webview_window("main") {
        Some(main_window) => {
            let system_locale = Locale::current();
            let lang_to_load_path = determine_app_language_path(&system_locale);
            let target_page = format!("{}/index.html", lang_to_load_path);

            match main_window.url() {
                Ok(current_url) => {
                    let current_path_str = current_url.path();
                    let target_path_to_compare = format!("/{}", target_page);

                    if current_path_str != target_path_to_compare {
                        let redirect_script =
                            format!("window.location.replace('/{}');", target_page);
                        if let Err(e) = main_window.eval(&redirect_script) {
                            eprintln!("Failed to execute redirect script (release mode): {}", e);
                        }
                    }
                }
                Err(e) => {
                    eprintln!("Failed to get current window URL (release mode): {}", e);
                }
            }
        }
        None => {
            eprintln!("Main window 'main' not found (release mode)");
        }
    }
}
