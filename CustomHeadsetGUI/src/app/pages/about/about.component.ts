import { Component, signal } from '@angular/core';
import { getVersion } from '@tauri-apps/api/app';
import { open } from '@tauri-apps/plugin-shell';
@Component({
  selector: 'app-about',
  imports: [],
  templateUrl: './about.component.html',
  styleUrl: './about.component.scss'
})
export class AboutComponent {
  public appVersion = signal('');
  constructor() {
    getVersion().then(this.appVersion.set)
  }
  async openGithub() {
    await open('https://github.com/sboys3/CustomHeadsetOpenVR')
  }
}
