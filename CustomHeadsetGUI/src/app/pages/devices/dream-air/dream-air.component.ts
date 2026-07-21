import { Component, effect, inject, output } from '@angular/core';
import { DreamAirConfig, HeadsetType } from '../../../services/JsonFileDefines';
import { HeadsetSettingsComponent, HeadsetSettingsConfig } from '../headset-settings/headset-settings.component';
import { PimaxLauncherComponent } from '../pimax-launcher/pimax-launcher.component';
import { PimaxSlamSettingsComponent } from '../pimax-slam-settings/pimax-slam-settings.component';
import { DriverSettingService } from '../../../services/driver-setting.service';

@Component({
  selector: 'app-dream-air',
  imports: [HeadsetSettingsComponent, PimaxLauncherComponent, PimaxSlamSettingsComponent],
  templateUrl: './dream-air.component.html',
  styleUrl: './dream-air.component.scss'
})
export class DreamAirComponent {
  navigateToGeneralTab = output<void>();
  HeadsetType = HeadsetType;
  private dss = inject(DriverSettingService);
  
  headsetConfig: HeadsetSettingsConfig = {
    driverName: [],
    settingField: 'dreamAir',
    enableText: $localize`Enable Dream Air Driver`,
    enableInfo: $localize`When enabled, the Custom Headset driver will run the Pimax Dream Air.`,
    resolutionInfo: $localize`The resolution to run the DisplayPort connection at.`,
    resolutionOptions: [
      { name: '4K', x: 3840, y: 3552 },
      { name: '3840x3840', x: 3840, y: 3840 },
      { name: '3840x3744', x: 3840, y: 3744 },
      { name: '2880x2880', x: 2880 , y: 2880 },
      { name: '2560x2560', x: 2560 , y: 2560 },
      { name: '2560x2496', x: 2560 , y: 2496 },
      { name: '2544x2544', x: 2544 , y: 2544 },
      { name: '1920x1920', x: 1920 , y: 1920 },
    ],
    showResolutionSelector: false,
    showDisplayRotation: false,
    showDriverWarning: false,
    ipdInfo: $localize`The inter pupillary distance of the virtual cameras in SteamVR applications. This IPD should be matched with what you set for the headset's physical IPD to get the correct world scale.`,
    defaultMaxFovX: 98,
    defaultMaxFovY: 88
  };

  constructor() {
    effect(() => {
      const settings = this.dss.values();
      const forceEnable = settings?.dreamAir?.forceEnable ?? false;
      this.headsetConfig.showResolutionSelector = forceEnable;
      this.headsetConfig.showDisplayRotation = forceEnable;
    });
  }
}