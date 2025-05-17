import { Injectable } from '@angular/core';
import { MatDialog } from '@angular/material/dialog';
import { ConfirmComponent } from '../dialogs/confirm/confirm.component';
import { firstValueFrom } from 'rxjs';
import { MessageComponent } from '../dialogs/message/message.component';

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
