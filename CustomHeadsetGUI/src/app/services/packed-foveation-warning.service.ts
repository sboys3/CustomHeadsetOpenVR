import { inject, Injectable } from '@angular/core';
import { AppSettingService } from './app-setting.service';
import { DialogService } from './dialog.service';

/**
 * Packed foveation breaks the image in every app that does not pack its frames,
 * so every way of turning it on asks through this service first.
 */
@Injectable({
  providedIn: 'root'
})
export class PackedFoveationWarningService {
  private dialog = inject(DialogService);
  private ass = inject(AppSettingService);

  // Resolves true when packed foveation may be turned on: the user confirmed now, or ticked "Don't show this again" before.
  async confirmEnable(): Promise<boolean> {
    if (this.ass.values()?.packedFoveationWarningAcknowledged) {
      return true;
    }
    const result = await this.dialog.confirmWithDontShowAgain(
      $localize`Enable Packed Foveation`,
      $localize`THIS IS NOT STANDARD FOVEATION!!!\nIf what you are using does not specifically specify *packed foveation*, you should not have this enabled. Enabling it will break the image in all other applications. Packed foveation only works with applications that have packed foveation enabled.\nMake sure to disable this after you are finished using packed foveation.\n\nIf you are looking for standard foveation, install the Quad Views Foveated OpenXR layer or enable foveation in the application you are using and leave this disabled.\n\nAre you sure you want to enable it?`
    );
    if (result.dontShowAgain) {
      // read the values again after the dialog, and skip the save while guiSettings.json is unreadable, so other app settings are never lost
      const values = this.ass.values();
      if (values) {
        await this.ass.save({ ...values, packedFoveationWarningAcknowledged: true });
      }
    }
    return result.confirmed;
  }
}
