import { Component, computed, Inject, signal } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { MatButtonModule } from '@angular/material/button';
import { MAT_DIALOG_DATA, MatDialogModule, MatDialogRef } from '@angular/material/dialog';
import { MatInputModule } from '@angular/material/input';

export type PromptData = {
  title: string,
  message?: string,
  label: string,
  value?: string,
  okText?: string,
  // returns an error message, or undefined when the value is valid
  validate?: (value: string) => string | undefined
};

@Component({
  selector: 'app-prompt',
  imports: [MatDialogModule, MatButtonModule, MatInputModule, FormsModule],
  templateUrl: './prompt.component.html',
  styleUrl: './prompt.component.scss'
})
export class PromptComponent {
  value = signal('');
  error = computed(() => this.data.validate?.(this.value()));
  constructor(@Inject(MAT_DIALOG_DATA) public data: PromptData, private dialogRef: MatDialogRef<PromptComponent, string>) {
    this.value.set(data.value ?? '');
  }
  submit() {
    if (!this.error()) {
      this.dialogRef.close(this.value().trim());
    }
  }
}
