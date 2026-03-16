import { Component } from '@angular/core';
import { MeganeX8KConfig } from '../../../services/JsonFileDefines';
import { HeadsetSettingsComponent, HeadsetSettingsConfig } from '../headset-settings/headset-settings.component';

@Component({
  selector: 'app-meganex-x8-k',
  imports: [HeadsetSettingsComponent],
  templateUrl: './meganex-x8-k.component.html',
  styleUrl: './meganex-x8-k.component.scss'
})
export class MeganexX8KComponent {
  headsetConfig: HeadsetSettingsConfig = {
    driverName: ['MeganeXSuperlight','MeganeX8KMark2','MeganeXsuperlight8K_Native','MeganeX8KMark2_Native'],
    settingField: 'meganeX8K',
    enableText: $localize`Enable MeganeX 8K Driver`,
    enableInfo: $localize`When enabled, the Custom Headset driver will run the MeganeX superlight 8K.`,
    resolutionInfo: $localize`The resolution to run the DisplayPort connection at.`,
    resolutionOptions: [
      { name: '4K', x: 3840, y: 3552 },
      { name: '2K', x: 2880, y: 2664 }
    ],
    showResolutionSelector: true,
    showDriverWarning: true,
    ipdInfo: $localize`The inter pupillary distance of the virtual cameras in SteamVR applications. You set the headset's physical IPD in the Shiftall Configurator. This IPD should be matched with what you set in the Shiftall Configurator to get the correct world scale.`,
    defaultMaxFovX: 115,
    defaultMaxFovY: 99
  };
}
