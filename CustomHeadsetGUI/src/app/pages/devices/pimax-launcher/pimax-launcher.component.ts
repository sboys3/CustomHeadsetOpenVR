import { Component, ChangeDetectorRef, effect, inject } from '@angular/core';
import { MatIconModule } from '@angular/material/icon';
import { MatButtonModule } from '@angular/material/button';
import { MatSlideToggleModule } from '@angular/material/slide-toggle';
import { MatTooltipModule } from '@angular/material/tooltip';
import { FormsModule } from '@angular/forms';
import { AppSetting } from '../../../services/JsonFileDefines';
import { AppSettingService } from '../../../services/app-setting.service';
import { PimaxLauncherService } from '../../../services/pimax-launcher.service';

@Component({
  selector: 'app-pimax-launcher',
  imports: [MatIconModule, MatButtonModule, MatSlideToggleModule, MatTooltipModule, FormsModule],
  templateUrl: './pimax-launcher.component.html',
  styleUrl: './pimax-launcher.component.scss'
})
export class PimaxLauncherComponent {
  private ass = inject(AppSettingService);
  private launcher = inject(PimaxLauncherService);
  private cdr = inject(ChangeDetectorRef);

  public advanceMode = this.ass.values()?.advanceMode ?? false;
  public launchDreamAirOnStartup = this.ass.values()?.launchPimaxOnStartup ?? false;
  public isLaunching = this.launcher.isLaunching;

  daysSinceDreamAirBlocked = Math.floor((Date.now() - new Date('2026-02-14').getTime()) / (1000 * 60 * 60 * 24));

  constructor() {
    effect(() => {
      this.advanceMode = this.ass.values()?.advanceMode ?? false;
    });

    effect(() => {
      this.launchDreamAirOnStartup = this.ass.values()?.launchPimaxOnStartup ?? false;
    });

    // React to shared isLaunching signal
    effect(() => {
      this.launcher.isLaunching();
      this.cdr.detectChanges();
    });
  }

  saveLaunchSetting() {
    const settings = this.ass.values() || {} as AppSetting;
    settings.launchPimaxOnStartup = this.launchDreamAirOnStartup;
    this.ass.save(settings);
  }

  async onLaunchHeadset(): Promise<void> {
    await this.launcher.toggleLaunch();
  }
}
