import { Component, effect, inject, output } from '@angular/core';
import { Pimax8KXConfig, HeadsetType } from '../../../services/JsonFileDefines';
import { HeadsetSettingsComponent, HeadsetSettingsConfig } from '../headset-settings/headset-settings.component';
import { PimaxLauncherComponent } from '../pimax-launcher/pimax-launcher.component';
import { DriverSettingService } from '../../../services/driver-setting.service';

@Component({
  selector: 'app-pimax-8k',
  imports: [HeadsetSettingsComponent, PimaxLauncherComponent],
  templateUrl: './pimax-8k.component.html',
  styleUrl: './pimax-8k.component.scss'
})
export class Pimax8KComponent {
  navigateToGeneralTab = output<void>();
  HeadsetType = HeadsetType;
  private dss = inject(DriverSettingService);
  
  headsetConfig: HeadsetSettingsConfig = {
    driverName: [],
    settingField: 'pimax8KX',
    enableText: $localize`Enable Pimax 8K Driver`,
    enableInfo: $localize`When enabled, the Custom Headset driver will run the Pimax 8K.`,
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

  constructor() {
    effect(() => {
      const settings = this.dss.values();
      const forceEnable = settings?.pimax8KX?.forceEnable ?? false;
      // this.headsetConfig.showResolutionSelector = forceEnable;
      this.headsetConfig.showDisplayRotation = forceEnable;
    });
  }
}