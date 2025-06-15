use leptos::task::spawn_local;
use leptos::{ev::Event, prelude::*};
use serde::{Deserialize, Serialize};
use wasm_bindgen::prelude::*;

#[wasm_bindgen]
extern "C" {
    #[wasm_bindgen(js_namespace = ["window", "__TAURI__", "core"])]
    async fn invoke(cmd: &str, args: JsValue) -> JsValue;
}

#[derive(Serialize, Deserialize)]
struct GreetArgs<'a> {
    name: &'a str,
}

#[component]
pub fn App() -> impl IntoView {
    let (content, set_content) = signal(String::new());

    let on_input = move |ev: Event| {
        set_content.set(event_target_value(&ev));
    };

    view! {
        <main> // style="padding: 2rem; max-width: 800px; margin: auto;">
            <div class="title">
                <h1>"Basic Text Editor"</h1>
                <button>Test</button>
            </div>
            <textarea
                on:input=on_input
                placeholder="Start typing..."
            >
                { move || content.get() }
            </textarea>
        </main>
    }
}
