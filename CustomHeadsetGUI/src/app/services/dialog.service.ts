import { Injectable } from '@angular/core';
import { MatDialog } from '@angular/material/dialog';
import { ConfirmComponent } from '../dialogs/confirm/confirm.component';
import { firstValueFrom } from 'rxjs';
import { MessageComponent } from '../dialogs/message/message.component';
import { PromptComponent, PromptData } from '../dialogs/prompt/prompt.component';

@Injectable({
  providedIn: 'root'
})
export class DialogService {

  constructor(private matDialog: MatDialog) { }

  async confirm(title: string, message: string, yesText?: string, yesClass?: string): Promise<true | undefined> {
    return await firstValueFrom(this.matDialog.open(ConfirmComponent, {
      hasBackdrop: true,
      data: {
        title,
        message,
        yesText,
        yesClass
      }
    }).afterClosed());
  }
  // Resolves true for yes, false for no, and undefined for cancel.
  async choose(title: string, message: string, yesText: string, noText: string, yesClass?: string): Promise<boolean | undefined> {
    // Cancel, Escape and a backdrop click all close with undefined
    const result = await firstValueFrom(this.matDialog.open(ConfirmComponent, {
      hasBackdrop: true,
      data: {
        title,
        message,
        yesText,
        yesClass,
        noText
      }
    }).afterClosed());
    return typeof result === 'boolean' ? result : undefined;
  }
  async confirmWithDontShowAgain(title: string, message: string, yesText?: string, yesClass?: string): Promise<{ confirmed: boolean, dontShowAgain: boolean }> {
    // Escape and a backdrop click close with undefined, which counts as cancel
    const result = await firstValueFrom(this.matDialog.open(ConfirmComponent, {
      hasBackdrop: true,
      data: {
        title,
        message,
        yesText,
        yesClass,
        dontShowAgain: true
      }
    }).afterClosed()) as { confirmed: boolean, dontShowAgain: boolean } | undefined;
    return result ?? { confirmed: false, dontShowAgain: false };
  }
  // Resolves the trimmed text, or undefined when cancelled.
  async prompt(data: PromptData): Promise<string | undefined> {
    const result = await firstValueFrom(this.matDialog.open(PromptComponent, {
      hasBackdrop: true,
      data
    }).afterClosed());
    // the Cancel button closes with an empty string
    return result || undefined;
  }
  async message(title: string, message: string): Promise<void> {
    return await firstValueFrom(this.matDialog.open(MessageComponent, {
      hasBackdrop: true,
      data: {
        title,
        message
      }
    }).afterClosed());
  }
}
