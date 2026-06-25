import { Component, effect, inject, output } from '@angular/core';
import { CrystalSuperMicroOLEDConfig, HeadsetType } from '../../../services/JsonFileDefines';
import { HeadsetSettingsComponent, HeadsetSettingsConfig } from '../headset-settings/headset-settings.component';
import { PimaxLauncherComponent } from '../pimax-launcher/pimax-launcher.component';
import { DriverSettingService } from '../../../services/driver-setting.service';
import { MatIconModule } from '@angular/material/icon';

@Component({
  selector: 'app-crystal-super-micro-oled',
  imports: [HeadsetSettingsComponent, PimaxLauncherComponent, MatIconModule],
  templateUrl: './crystal-super-micro-oled.component.html',
  styleUrl: './crystal-super-micro-oled.component.scss'
})
export class CrystalSuperMicroOledComponent {
  navigateToGeneralTab = output<void>();
  HeadsetType = HeadsetType;
  private dss = inject(DriverSettingService);
  
  headsetConfig: HeadsetSettingsConfig = {
    driverName: [],
    settingField: 'crystalSuperMicroOLED',
    enableText: $localize`Enable Crystal Super Micro-OLED Driver`,
    enableInfo: $localize`When enabled, the Custom Headset driver will run the Pimax Crystal Super Micro-OLED.`,
    resolutionInfo: $localize`The resolution to run the DisplayPort connection at.`,
    resolutionOptions: [
      { name: '4K', x: 3840, y: 3552 },
    ],
    showResolutionSelector: false,
    showDisplayRotation: false,
    showDriverWarning: false,
    ipdInfo: $localize`The inter pupillary distance of the virtual cameras in SteamVR applications. This IPD should be matched with what you set for the headset's physical IPD to get the correct world scale.`,
    defaultMaxFovX: 96,
    defaultMaxFovY: 86
  };

  constructor() {
    effect(() => {
      const settings = this.dss.values();
      const forceEnable = settings?.crystalSuperMicroOLED?.forceEnable ?? false;
      this.headsetConfig.showResolutionSelector = forceEnable;
      this.headsetConfig.showDisplayRotation = forceEnable;
    });
  }
}
