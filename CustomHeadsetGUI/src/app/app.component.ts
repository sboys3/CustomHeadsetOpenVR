import { Component, effect,  Inject } from '@angular/core';
import { RouterModule, RouterOutlet } from '@angular/router';
import { MatTabsModule } from '@angular/material/tabs';
import {  DOCUMENT } from '@angular/common';
import { AppSettingService } from './services/app-setting.service';
@Component({
  selector: 'app-root',
  imports: [RouterOutlet, RouterModule, MatTabsModule],
  templateUrl: './app.component.html',
  styleUrl: './app.component.scss'
})
export class AppComponent {
  navigation = [
    {
      name: $localize`Basic`,
      route: '/basic-settings',
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
  constructor(appSettingService: AppSettingService, @Inject(DOCUMENT) document: Document) {
    effect(() => {
      const settings = appSettingService.values();
      console.log('effect',settings)
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
