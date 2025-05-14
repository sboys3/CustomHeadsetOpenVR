import { Routes } from '@angular/router';

export const routes: Routes = [
    { path: 'basic-settings', loadComponent: () => import('./pages/basic-settings/basic-settings.component').then(x => x.BasicSettingsComponent) },
    { path: 'distortion-profile', loadComponent: () => import('./pages/distortion-profile/distortion-profile.component').then(x => x.DistortionProfileComponent) },
    { path: 'app-settings', loadComponent: () => import('./pages/app-settings/app-settings.component').then(x => x.AppSettingsComponent) },
    { path: '**', redirectTo: 'basic-settings' }
];
