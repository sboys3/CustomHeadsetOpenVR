import { effect, Injectable, inject, signal } from '@angular/core';
import { kill_process, launch_process } from '../tauri_wrapper';
import { openUrl } from '@tauri-apps/plugin-opener';
import { AppSettingService } from './app-setting.service';
import { DriverSettingService } from './driver-setting.service';

@Injectable({
  providedIn: 'root'
})
export class PimaxLauncherService {
  private ass = inject(AppSettingService);
  private dss = inject(DriverSettingService);

  public isLaunching = signal(false);
  private autoLaunchTriggered = false;
  private cancelRequested = false;
  private killTimeoutId: ReturnType<typeof setTimeout> | null = null;
  private steamvrTimeoutId: ReturnType<typeof setTimeout> | null = null;

  constructor() {
    // Auto-launch effect runs at service init (app startup), not when tab is selected
    effect(() => {
      if (this.autoLaunchTriggered) return;
      if (this.ass.values()?.launchPimaxOnStartup && this.dss.values()?.dreamAir?.enable) {
        this.autoLaunchTriggered = true;
        setTimeout(() => this.start(), 1000);
      }
    });
  }

  /**
   * Start or cancel the launch sequence.
   */
  async toggleLaunch(): Promise<void> {
    if (this.isLaunching()) {
      this.cancel();
    } else {
      this.start();
    }
  }

  /**
   * Start the Dream Air launch sequence.
   */
  async start(): Promise<void> {
    this.isLaunching.set(true);
    this.cancelRequested = false;
    await this.launchSequence();
  }

  /**
   * Cancel the current launch sequence.
   */
  cancel(): void {
    this.cancelRequested = true;
    if (this.steamvrTimeoutId !== null) {
      clearTimeout(this.steamvrTimeoutId);
      this.steamvrTimeoutId = null;
    }
    console.log('Launch sequence cancelled');
  }

  /**
   * Reset the auto-launch flag (for testing or manual re-trigger).
   */
  resetAutoLaunch(): void {
    this.autoLaunchTriggered = false;
  }

  private async launchSequence(): Promise<void> {
    const killInterval = 250; // 0.25 seconds
    const totalDuration = 40000; // 40 seconds
    const steamvrDelay = 5000; // 5 seconds

    let steamvrLaunched = false;
    const startTime = Date.now();

    // Start Pimax Play
    await launch_process("C:/Program Files/Pimax/PimaxClient/pimaxui/PimaxClient.exe", []);

    // Recursive timeout function for killing pi_server
    const killLoop = async (): Promise<void> => {
      if (this.cancelRequested) {
        console.log('Kill loop cancelled');
        this.isLaunching.set(false);
        return;
      }

      const elapsed = Date.now() - startTime;
      if (elapsed >= totalDuration || !this.isLaunching()) {
        console.log('stopping pi_server killing');
        this.isLaunching.set(false);
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
      setTimeout(() => resolve(), totalDuration);
    });
  }
}
