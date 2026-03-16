import { Component } from '@angular/core';
import { DreamAirConfig } from '../../../services/JsonFileDefines';
import { HeadsetSettingsComponent, HeadsetSettingsConfig } from '../headset-settings/headset-settings.component';

@Component({
  selector: 'app-dream-air',
  imports: [HeadsetSettingsComponent],
  templateUrl: './dream-air.component.html',
  styleUrl: './dream-air.component.scss'
})
export class DreamAirComponent {
  headsetConfig: HeadsetSettingsConfig = {
    driverName: [],
    settingField: 'dreamAir',
    enableText: $localize`Enable Dream Air Driver`,
    enableInfo: $localize`When enabled, the Custom Headset driver will run the Pimax Dream Air.`,
    resolutionInfo: $localize`The resolution to run the DisplayPort connection at.`,
    resolutionOptions: [
      { name: '4K', x: 3840, y: 3552 }
    ],
    showResolutionSelector: false,
    showDriverWarning: false,
    ipdInfo: $localize`The inter pupillary distance of the virtual cameras in SteamVR applications. This IPD should be matched with what you set for the headset's physical IPD to get the correct world scale.`,
    defaultMaxFovX: 96,
    defaultMaxFovY: 86
  };
}
