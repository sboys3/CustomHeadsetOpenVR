import { ApplicationConfig, provideZoneChangeDetection, provideAppInitializer } from '@angular/core';
import { provideRouter } from '@angular/router';

import { routes } from './app.routes';
import { provideAnimationsAsync } from '@angular/platform-browser/animations/async';
import { appInitializer } from './appInitializer';
import { provideHttpClient } from '@angular/common/http';
import { AppSettingAccessor, AppSettingHolder } from './services/AppSettingAccessor';

export const appConfig: ApplicationConfig = {

  providers: [
    provideZoneChangeDetection({ eventCoalescing: true }),
    provideRouter(routes),
    provideAnimationsAsync(),
    provideHttpClient(),
    provideAppInitializer(appInitializer),
    AppSettingHolder,
    {
      provide: AppSettingAccessor,
      useExisting: AppSettingHolder
    }
  ]
};

