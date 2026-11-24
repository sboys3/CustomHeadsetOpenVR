import { Component, Inject } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { MAT_DIALOG_DATA, MatDialogModule } from '@angular/material/dialog';
import { MatButtonModule } from '@angular/material/button';
import { MatCheckboxModule } from '@angular/material/checkbox';
@Component({
  selector: 'app-confirm',
  imports: [MatDialogModule, MatButtonModule, MatCheckboxModule, FormsModule],
  templateUrl: './confirm.component.html',
  styleUrl: './confirm.component.scss'
})
export class ConfirmComponent {
  dontShowAgain = false;
  constructor(@Inject(MAT_DIALOG_DATA) public data: { title: string, message: string, yesText?: string, yesClass?: string, dontShowAgain?: boolean }) { }
  cancelValue() {
    return this.data.dontShowAgain ? { confirmed: false, dontShowAgain: this.dontShowAgain } : undefined;
  }
  yesValue() {
    return this.data.dontShowAgain ? { confirmed: true, dontShowAgain: this.dontShowAgain } : true;
  }
}
