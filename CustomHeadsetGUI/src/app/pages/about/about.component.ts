import { Component, signal } from '@angular/core';
import { open } from '@tauri-apps/plugin-shell';
import { MatButtonModule } from '@angular/material/button';
import { MatIconModule } from '@angular/material/icon';
import { AppUpdateInfoSuccess, AppUpdateService } from '../../services/app-update.service';
import { delay } from '../../helpers';
@Component({
  selector: 'app-about',
  imports: [MatButtonModule, MatIconModule],
  templateUrl: './about.component.html',
  styleUrl: './about.component.scss'
})
export class AboutComponent {
  public checking = signal<boolean>(false)
  public updateInfo = signal<AppUpdateInfoSuccess | undefined>(undefined)
  constructor(public aus: AppUpdateService) {

  }
  async openExternal(url: string) {
    await open(url)
  }

  async checkUpdate() {
    this.checking.set(true)
    try {
      const start = performance.now();
      await this.aus.checkUpdate();

      const wait = 2000 - (performance.now() - start);
      if (wait > 0) {
        await delay(wait)
      }
    } finally {
      this.checking.set(false)
    }
  }
}
