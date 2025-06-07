import { Component } from '@angular/core';
import { SystemReadyComponent } from '../../utilities/system-ready/system-ready.component';
import { MeganexX8KComponent } from '../devices/meganex-x8-k/meganex-x8-k.component';
import { MatTabsModule } from '@angular/material/tabs';
import { GeneralComponent } from '../devices/general/general.component';



@Component({
    selector: 'app-driver-settings',
    imports: [
        SystemReadyComponent,
        MeganexX8KComponent,
        GeneralComponent,
        MatTabsModule
    ],
    providers: [MeganexX8KComponent],
    templateUrl: './driver-settings.component.html',
    styleUrl: './driver-settings.component.scss'
})
export class DriverSettingsComponent {

    constructor() {

    }

}
