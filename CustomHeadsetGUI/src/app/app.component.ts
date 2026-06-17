import { Component, effect, Inject } from '@angular/core';
import { RouterModule, RouterOutlet } from '@angular/router';
import { MatTabsModule } from '@angular/material/tabs';
import { DOCUMENT } from '@angular/common';
import { MatIconModule } from '@angular/material/icon';
import { getCurrentWebviewWindow } from '@tauri-apps/api/webviewWindow';
import { AppSettingService } from './services/app-setting.service';
import { AppUpdateService } from './services/app-update.service';
import { PimaxLauncherService } from './services/pimax-launcher.service';
import { SystemDiagnosticService } from './services/system-diagnostic.service';
import { vendor } from '../environment'

@Component({
  selector: 'app-root',
  imports: [RouterOutlet, RouterModule, MatTabsModule, MatIconModule],
  templateUrl: './app.component.html',
  styleUrl: './app.component.scss'
})
export class AppComponent {
  navigation = [
    {
      name: $localize`Driver Settings`,
      route: '/driver-settings',
    },
    {
      name: $localize`Distortion Profile`,
      route: '/distortion-profile',
    },
    {
      name: $localize`App Settings`,
      route: '/app-settings'
    }
  ];
  constructor(appSettingService: AppSettingService, public appUpdateService: AppUpdateService, public sds: SystemDiagnosticService, @Inject(DOCUMENT) document: Document, @Inject(PimaxLauncherService) _launcher: PimaxLauncherService) {
    // Set the native window title with localization support
    let windowTitle = $localize`Custom Headset`
    switch(vendor){
      case "pimax":
        windowTitle = $localize`Pimax Native Headset`
        break;
    }
    getCurrentWebviewWindow().setTitle(windowTitle);

    effect(() => {
      const settings = appSettingService.values();
      if (settings) {
        const body = document.body;
        // Remove all theme classes first
        body.classList.remove('evo-theme');
        switch (settings.colorScheme) {
          case 'dark':
            document.documentElement.style.setProperty('--color-scheme', 'dark');
            break;
          case 'light':
            document.documentElement.style.setProperty('--color-scheme', 'light');
            break;
          case 'evo':
            document.documentElement.style.setProperty('--color-scheme', 'dark');
            body.classList.add('evo-theme');
            break;
          default:
            document.documentElement.style.setProperty('--color-scheme', 'dark light');
            break;
        }
      }
    });
  }
}
