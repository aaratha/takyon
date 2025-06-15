use tauri::{TitleBarStyle, WebviewUrl, WebviewWindowBuilder};

use tauri::webview::Color;
use tauri::window::{Effect, EffectsBuilder};
use tauri::LogicalPosition;

//use window_vibrancy::{apply_blur, apply_vibrancy, NSVisualEffectMaterial};

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .setup(|app| {
            let win_builder = WebviewWindowBuilder::new(app, "main", WebviewUrl::default())
                .title("Transparent Titlebar Window")
                .inner_size(800.0, 600.0)
                .transparent(true);
            //.decorations(false);
            //.resizable(true)
            //.fullscreen(false)
            //.always_on_top(false)
            //.visible(true);

            #[cfg(target_os = "macos")]
            let win_builder = win_builder
                .title_bar_style(TitleBarStyle::Overlay)
                .hidden_title(true)
                .background_color(Color(0, 0, 0, 1)) // <-
                .effects(
                    EffectsBuilder::new()
                        .effects(vec![Effect::HudWindow])
                        .build(),
                )
                .traffic_light_position(LogicalPosition::new(15.0, 20.0));

            let window = win_builder.build().unwrap();

            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
