import { Routes } from '@angular/router';

export const routes: Routes = [
    { path: 'basic-settings', loadComponent: () => import('./basic-settings/basic-settings.component').then(x => x.BasicSettingsComponent) },
    { path: 'distortion-profile', loadComponent: () => import('./distortion-profile/distortion-profile.component').then(x => x.DistortionProfileComponent) },
    { path: 'app-settings', loadComponent: () => import('./app-settings/app-settings.component').then(x => x.AppSettingsComponent) },
    { path: '**', redirectTo: 'basic-settings' }
];
