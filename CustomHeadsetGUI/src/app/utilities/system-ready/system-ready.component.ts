import { Component } from '@angular/core';
import { SystemDiagnosticService } from '../../services/system-diagnostic.service';
import { DriverTroubleshooterComponent } from '../driver-troubleshooter/driver-troubleshooter.component';

@Component({
  selector: 'app-system-ready',
  imports: [DriverTroubleshooterComponent],
  providers: [SystemDiagnosticService],
  templateUrl: './system-ready.component.html',
  styleUrl: './system-ready.component.scss'
})
export class SystemReadyComponent {
  constructor(public sds: SystemDiagnosticService) {

  }
}
