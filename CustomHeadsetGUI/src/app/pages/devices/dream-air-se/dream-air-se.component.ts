import { Component, effect, inject, output } from '@angular/core';
import { DreamAirSEConfig, HeadsetType } from '../../../services/JsonFileDefines';
import { HeadsetSettingsComponent, HeadsetSettingsConfig } from '../headset-settings/headset-settings.component';
import { PimaxLauncherComponent } from '../pimax-launcher/pimax-launcher.component';
import { DriverSettingService } from '../../../services/driver-setting.service';
import { MatIconModule } from '@angular/material/icon';

@Component({
  selector: 'app-dream-air-se',
  imports: [HeadsetSettingsComponent, PimaxLauncherComponent, MatIconModule],
  templateUrl: './dream-air-se.component.html',
  styleUrl: './dream-air-se.component.scss'
})
export class DreamAirSeComponent {
  navigateToGeneralTab = output<void>();
  HeadsetType = HeadsetType;
  private dss = inject(DriverSettingService);
  
  headsetConfig: HeadsetSettingsConfig = {
    driverName: [],
    settingField: 'dreamAirSE',
    enableText: $localize`Enable Dream Air SE Driver`,
    enableInfo: $localize`When enabled, the Custom Headset driver will run the Pimax Dream Air SE.`,
    resolutionInfo: $localize`The resolution to run the DisplayPort connection at.`,
    resolutionOptions: [
      { name: '3K', x: 2544, y: 2544 },
      { name: '2K', x: 1920, y: 1920 },
    ],
    showResolutionSelector: true,
    showDisplayRotation: false,
    showDriverWarning: false,
    ipdInfo: $localize`The inter pupillary distance of the virtual cameras in SteamVR applications. This IPD should be matched with what you set for the headset's physical IPD to get the correct world scale.`,
    defaultMaxFovX: 90,
    defaultMaxFovY: 90
  };

  constructor() {
    effect(() => {
      const settings = this.dss.values();
      const forceEnable = settings?.dreamAirSE?.forceEnable ?? false;
      this.headsetConfig.showDisplayRotation = forceEnable;
    });
  }
}