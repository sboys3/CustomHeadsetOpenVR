import { Component, Inject } from '@angular/core';
import { MAT_DIALOG_DATA, MatDialogModule } from '@angular/material/dialog';
import { MatButtonModule } from '@angular/material/button';
@Component({
  selector: 'app-confirm',
  imports: [MatDialogModule, MatButtonModule],
  templateUrl: './confirm.component.html',
  styleUrl: './confirm.component.scss'
})
export class ConfirmComponent {
  constructor(@Inject(MAT_DIALOG_DATA) public data: { title: string, message: string, yesText?: string, yesClass?: string }) { }
}
