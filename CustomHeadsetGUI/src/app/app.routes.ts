import { Routes } from '@angular/router';

export const routes: Routes = [
    { path: 'basic-settings', loadComponent: () => import('./basic-settings/basic-settings.component').then(x => x.BasicSettingsComponent) },
    { path: 'distortion-profile-editor', loadComponent: () => import('./distortion-profile-editor/distortion-profile-editor.component').then(x => x.DistortionProfileEditorComponent) },
    { path: '**', redirectTo: 'basic-settings' }
];
