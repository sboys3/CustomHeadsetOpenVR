# CustomHeadsetGUI

## Start Development
1. clone repository
2. install [node.js](https://nodejs.org/) & [rust](https://www.rust-lang.org/tools/install) & vs code
3. install vs code extensions, `Angular Language Service` & `rust-analyzer`
4. run `npm i` in `CustomHeadsetGUI`
5. run `npm start` in `CustomHeadsetGUI` to start dev server

## Build
1. clone repository
2. install [node.js](https://nodejs.org/) & [rust](https://www.rust-lang.org/tools/install)
3. run `npm i` in `CustomHeadsetGUI`
4. run `npm run build` in `CustomHeadsetGUI`
5. output to `..\output\CustomHeadsetGUI`

## Localization
Run `npx ng extract-i18n` to extract i18n strings.  
Translations are located in `src/locale` folder.

## Project Structure

```
CustomeHeadsetGUI
├───public //angular static files
├───src //angular sources
│    ├────app
│    │     ├───dialogs  //dialogs, all components opened using MatDialog in here
│    │     ├───pages    //app pages, all tabs main components
│    │     │    ├────devices //device configuration components
│    │     │    │    ├────general //general device settings
│    │     │    │    └────meganex-x8-k //Meganex X8-K specific settings
│    │     │    └────... //other pages
│    │     ├───services //services, all injectable service
│    │     ├───utilities //utility components
│    │     └───tauri_wrapper.ts // wrapper for native functions in js_api.rs
│    ├────fonts //custom fonts
│    └────locale //i18n strings
└───src-tauri //tauri sources
     └──src
         └─js_api.rs // custom rust api for javascript
```