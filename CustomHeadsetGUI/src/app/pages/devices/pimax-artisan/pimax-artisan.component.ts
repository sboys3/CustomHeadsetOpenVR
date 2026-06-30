import { Component, output } from '@angular/core';
import { PimaxArtisanConfig, HeadsetType } from '../../../services/JsonFileDefines';
import { HeadsetSettingsComponent, HeadsetSettingsConfig } from '../headset-settings/headset-settings.component';
import { PimaxLauncherComponent } from '../pimax-launcher/pimax-launcher.component';

@Component({
  selector: 'app-pimax-artisan',
  imports: [HeadsetSettingsComponent, PimaxLauncherComponent],
  templateUrl: './pimax-artisan.component.html',
  styleUrl: './pimax-artisan.component.scss'
})
export class PimaxArtisanComponent {
  navigateToGeneralTab = output<void>();
  HeadsetType = HeadsetType;
  
  headsetConfig: HeadsetSettingsConfig = {
    driverName: [],
    settingField: 'pimaxArtisan',
    enableText: $localize`Enable Pimax Artisan Driver`,
    enableInfo: $localize`When enabled, the Custom Headset driver will run the Pimax Artisan.`,
    resolutionInfo: $localize`The resolution to run the DisplayPort connection at.`,
    resolutionOptions: [
      { name: 'Auto', x: 0, y: 0 },
      { name: '2160x3840', x: 2160, y: 3840 },
      { name: '2160x3168', x: 2160, y: 3168 },
      { name: '2560x1440', x: 2560, y: 1440 },
      { name: '1440x2560', x: 1440, y: 2560 },
    ],
    showResolutionSelector: true,
    showDisplayRotation: false,
    showDriverWarning: false,
    ipdInfo: $localize`The inter pupillary distance of the virtual cameras in SteamVR applications. This IPD should be matched with what you set for the headset's physical IPD to get the correct world scale.`,
    defaultMaxFovX: 100,
    defaultMaxFovY: 90
  };
}