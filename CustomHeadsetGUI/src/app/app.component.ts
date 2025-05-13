import { Component } from '@angular/core';
import { RouterModule, RouterOutlet } from '@angular/router';
import { MatTabsModule } from '@angular/material/tabs';
import { CommonModule } from '@angular/common';
@Component({
  selector: 'app-root',
  imports: [RouterOutlet, RouterModule, MatTabsModule, CommonModule],
  templateUrl: './app.component.html',
  styleUrl: './app.component.scss'
})
export class AppComponent {
  navigation = [
    {
      name: $localize`Basic`,
      link: '/basic-settings',
    },
    {
      name: $localize`Distortion Profile`,
      link: '/distortion-profile',
    }
  ];
}
