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
      name: 'Table',
      link: '',
    },
    {
      name: 'Stats',
      link: '/stats',
    },
    {
      name: 'Settings',
      link: '/settings',
    },
    {
      name: 'Export',
      link: '/export',
    }
  ];
}
