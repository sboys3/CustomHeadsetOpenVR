import { Component, effect, Inject, signal } from '@angular/core';
import { RouterModule, RouterOutlet } from '@angular/router';
import { MatTabsModule } from '@angular/material/tabs';
import { DOCUMENT } from '@angular/common';
import { MatIconModule } from '@angular/material/icon';
import { AppSettingService } from './services/app-setting.service';
import { AppUpdateService } from './services/app-update.service';
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
  constructor(appSettingService: AppSettingService,public appUpdateService: AppUpdateService, @Inject(DOCUMENT) document: Document) {
    effect(() => {
      const settings = appSettingService.values();
      if (settings) {
        switch (settings.colorScheme) {
          case 'dark':
            document.documentElement.style.setProperty('--color-scheme', 'dark');
            break;
          case 'light':
            document.documentElement.style.setProperty('--color-scheme', 'light');
            break;
          default:
            document.documentElement.style.setProperty('--color-scheme', 'dark light');
            break;
        }
      }
    });
  }
}
