import { Component, input } from '@angular/core';
import { MatIconModule } from '@angular/material/icon';
import { MatTooltipModule } from '@angular/material/tooltip';

@Component({
  selector: 'app-field-tip',
  imports: [MatIconModule, MatTooltipModule],
  templateUrl: './field-tip.component.html',
  styleUrl: './field-tip.component.scss'
})
export class FieldTipComponent {
  info = input.required<string>()
}
