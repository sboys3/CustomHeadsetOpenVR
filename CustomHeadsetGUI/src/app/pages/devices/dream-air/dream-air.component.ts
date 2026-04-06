import { Component, ChangeDetectorRef } from '@angular/core';
import { MatIconModule } from '@angular/material/icon';
import { MatButtonModule } from '@angular/material/button';
import { DreamAirConfig } from '../../../services/JsonFileDefines';
import { HeadsetSettingsComponent, HeadsetSettingsConfig } from '../headset-settings/headset-settings.component';
import { kill_process, launch_process } from '../../../tauri_wrapper';
import { openUrl } from '@tauri-apps/plugin-opener';

@Component({
  selector: 'app-dream-air',
  imports: [HeadsetSettingsComponent, MatIconModule, MatButtonModule],
  templateUrl: './dream-air.component.html',
  styleUrl: './dream-air.component.scss'
})
export class DreamAirComponent {
  constructor(private cdr: ChangeDetectorRef) {}
  
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

  daysSinceDreamAirBlocked = Math.floor((Date.now() - new Date('2026-02-14').getTime()) / (1000 * 60 * 60 * 24));

  isLaunching: boolean = false;
  private killTimeoutId: ReturnType<typeof setTimeout> | null = null;
  private steamvrTimeoutId: ReturnType<typeof setTimeout> | null = null;
  private cancelRequested: boolean = false;

  async onLaunchHeadset(): Promise<void> {
    if (this.isLaunching) {
      // Cancel the current launch sequence
      this.cancelLaunchSequence();
    } else {
      // Start the launch sequence
      this.isLaunching = true;
      this.cdr.detectChanges();
      this.cancelRequested = false;
      await this.launchDreamAirSequence();
    }
  }

  private cancelLaunchSequence(): void {
    this.cancelRequested = true;
    
    // ends itself
    // if (this.killTimeoutId !== null) {
    //   clearTimeout(this.killTimeoutId);
    //   this.killTimeoutId = null;
    // }
    if (this.steamvrTimeoutId !== null) {
      clearTimeout(this.steamvrTimeoutId);
      this.steamvrTimeoutId = null;
    }
    console.log('Launch sequence cancelled');
  }

  private async launchDreamAirSequence(): Promise<void> {
    const killInterval = 250; // 0.25 seconds
    const totalDuration = 30000; // 30 seconds
    const steamvrDelay = 5000; // 5 seconds

    let steamvrLaunched = false;
    const startTime = Date.now();
    
    // Start Pimax Play
    await launch_process("C:/Program Files/Pimax/PimaxClient/pimaxui/PimaxClient.exe", []);

    // Recursive timeout function for killing pi_server
    const killLoop = async (): Promise<void> => {
      if (this.cancelRequested) {
        console.log('Kill loop cancelled');
        this.isLaunching = false;
        this.cdr.detectChanges();
        return;
      }

      const elapsed = Date.now() - startTime;
      if (elapsed >= totalDuration || !this.isLaunching) {
        console.log('stopping pi_server killing');
        this.isLaunching = false;
        this.cdr.detectChanges();
        return;
      }

      await kill_process('pi_server.exe');

      // Schedule next kill
      this.killTimeoutId = setTimeout(killLoop, killInterval);
    };

    // Start the kill loop
    killLoop();

    // Launch SteamVR after 5 seconds
    this.steamvrTimeoutId = setTimeout(async () => {
      if (this.cancelRequested || steamvrLaunched) {
        return;
      }
      steamvrLaunched = true;
      
      // Try direct SteamVR executable paths first
      const steamvrPaths = [
        'C:/Program Files (x86)/Steam/steamapps/common/SteamVR/bin/win64/vrstartup.exe',
        'C:/Program Files/Steam/steamapps/common/SteamVR/bin/win64/vrstartup.exe',
      ];

      for (const path of steamvrPaths) {
        const success = await launch_process(path, []);
        if (success) {
          console.log('SteamVR launched successfully from', path);
          return;
        }
      }

      // Fallback to Steam protocol URL
      try {
        await openUrl('steam://rungameid/250820');
        console.log('SteamVR launch requested via Steam protocol');
      } catch (error) {
        console.log('Failed to launch SteamVR:', error);
      }
    }, steamvrDelay);

    // Wait for the full duration or cancellation
    await new Promise<void>(resolve => {
      const waitTimeout = setTimeout(() => {
        resolve();
      }, totalDuration);
      
      // Store reference so we can clear it on cancel
      (this as any).waitTimeoutId = waitTimeout;
    });
  }
}
