import { Component, output } from '@angular/core';
import { CrystalOGConfig, HeadsetType } from '../../../services/JsonFileDefines';
import { HeadsetSettingsComponent, HeadsetSettingsConfig } from '../headset-settings/headset-settings.component';
import { PimaxLauncherComponent } from '../pimax-launcher/pimax-launcher.component';
import { PimaxSlamSettingsComponent } from '../pimax-slam-settings/pimax-slam-settings.component';
import { MatIconModule } from '@angular/material/icon';

@Component({
  selector: 'app-crystal-og',
  imports: [HeadsetSettingsComponent, PimaxLauncherComponent, PimaxSlamSettingsComponent, MatIconModule],
  templateUrl: './crystal-og.component.html',
  styleUrl: './crystal-og.component.scss'
})
export class CrystalOgComponent {
  navigateToGeneralTab = output<void>();
  HeadsetType = HeadsetType;
  
  headsetConfig: HeadsetSettingsConfig = {
    driverName: [],
    settingField: 'crystalOG',
    enableText: $localize`Enable Crystal OG Driver`,
    enableInfo: $localize`When enabled, the Custom Headset driver will run the Pimax Crystal OG.`,
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