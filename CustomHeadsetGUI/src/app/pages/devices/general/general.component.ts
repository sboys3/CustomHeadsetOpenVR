import { ChangeDetectorRef, Component, inject } from '@angular/core';
import { AppSetting, Settings } from '../../../services/JsonFileDefines';
import { DialogService } from '../../../services/dialog.service';
import { DeviceConfigComponentBase } from '../DeviceConfigComponentBase';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { MatButtonModule } from '@angular/material/button';
import { MatDividerModule } from '@angular/material/divider';
import { MatIconModule } from '@angular/material/icon';
import { MatInputModule } from '@angular/material/input';
import { MatSelectModule } from '@angular/material/select';
import { MatSlideToggleModule } from '@angular/material/slide-toggle';
import { MatExpansionModule } from '@angular/material/expansion';
import { MatSliderModule } from '@angular/material/slider';
import { MatTooltipModule } from '@angular/material/tooltip';
import { FieldNoteComponent } from '../../../utilities/field-note/field-note.component';
import { FieldTipComponent } from '../../../utilities/field-tip/field-tip.component';
import { ResetButtonComponent } from '../../../utilities/reset-button/reset-button.component';
import { ColorPickerComponent } from '../../../utilities/color-picker/color-picker.component';
import { vendor, vendorUi } from '../../../../environment';

@Component({
  selector: 'app-general',
  imports: [MatSliderModule,
    MatSelectModule,
    FormsModule,
    MatDividerModule,
    MatButtonModule,
    MatIconModule,
    MatInputModule,
    MatSlideToggleModule,
    MatExpansionModule,
    MatTooltipModule,
    ResetButtonComponent,
    FieldTipComponent,
    FieldNoteComponent,
    ColorPickerComponent,
    CommonModule],
  templateUrl: './general.component.html',
  styleUrl: './general.component.scss'
})
export class GeneralComponent extends DeviceConfigComponentBase<Settings & { enable: false }> {
  vendor = vendor;
  vendorUi = vendorUi;
  showMeganeX8K = !vendorUi || vendorUi == "shiftall";
  showPimax = !vendorUi || vendorUi == "pimax";
  private dialog = inject(DialogService);
  private cdr = inject(ChangeDetectorRef);
  async onPackedFoveationChange(value: boolean) {
    if (value) {
      const acknowledged = this.ass.values()?.packedFoveationWarningAcknowledged;
      if (!acknowledged) {
        const result = await this.dialog.confirmWithDontShowAgain(
          $localize`Enable Packed Foveation`,
          $localize`THIS IS NOT STANDARD FOVEATION!!!\nIf what you are using does not specifically specify *packed foveation*, you should not have this enabled. Enabling it will break the image in all other applications. Packed foveation only works with applications that have packed foveation enabled.\nMake sure to disable this after you are finished using packed foveation.\n\nIf you are looking for standard foveation, install the Quad Views Foveated OpenXR layer or enable foveation in the application you are using and leave this disabled.\n\nAre you sure you want to enable it?`
        );
        if (result.dontShowAgain) {
          const appSettings = (this.ass.values() ?? {}) as AppSetting;
          this.ass.save({ ...appSettings, packedFoveationWarningAcknowledged: true });
        }
        if (!result.confirmed) {
          // canceling means it should not be enabled
          this.settings!.customShader.enablePackedFoveation = false;
          // the ngModel already set the toggle to "on" when clicked; force the view to sync back to the reverted model
          this.cdr.detectChanges();
          return;
        }
      }
    }
    this.saveConfigSettings();
  }
}
