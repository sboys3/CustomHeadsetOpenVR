import { Component, output } from '@angular/core';
import { CrystalSuper50PPDConfig, HeadsetType } from '../../../services/JsonFileDefines';
import { HeadsetSettingsComponent, HeadsetSettingsConfig } from '../headset-settings/headset-settings.component';
import { PimaxLauncherComponent } from '../pimax-launcher/pimax-launcher.component';
import { MatIconModule } from '@angular/material/icon';

@Component({
  selector: 'app-crystal-super-50ppd',
  imports: [HeadsetSettingsComponent, PimaxLauncherComponent, MatIconModule],
  templateUrl: './crystal-super-50ppd.component.html',
  styleUrl: './crystal-super-50ppd.component.scss'
})
export class CrystalSuper50PpdComponent {
  navigateToGeneralTab = output<void>();
  HeadsetType = HeadsetType;
  
  headsetConfig: HeadsetSettingsConfig = {
    driverName: [],
    settingField: 'crystalSuper50PPD',
    enableText: $localize`Enable Crystal Super 50PPD Driver`,
    enableInfo: $localize`When enabled, the Custom Headset driver will run the Pimax Crystal Super 50PPD.`,
    resolutionInfo: $localize`The resolution to run the DisplayPort connection at.`,
    resolutionOptions: [
      { name: '4K', x: 3840, y: 3552 },
    ],
    showResolutionSelector: false,
    showDisplayRotation: false,
    showDriverWarning: false,
    ipdInfo: $localize`The inter pupillary distance of the virtual cameras in SteamVR applications. This IPD should be matched with what you set for the headset's physical IPD to get the correct world scale.`,
    defaultMaxFovX: 98,
    defaultMaxFovY: 88
  };
}