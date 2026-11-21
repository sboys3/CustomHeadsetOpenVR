import { ChangeDetectionStrategy, Component, inject, signal } from '@angular/core';
import { MatButtonModule } from '@angular/material/button';
import { MatIconModule } from '@angular/material/icon';
import { MatSelect, MatSelectModule } from '@angular/material/select';
import { MatTooltipModule } from '@angular/material/tooltip';
import { DialogService } from '../../services/dialog.service';
import { validateProfileName } from '../../services/settings-profile-helpers';
import { SettingsProfileService } from '../../services/settings-profile.service';

@Component({
  selector: 'app-profile-bar',
  imports: [MatSelectModule, MatButtonModule, MatIconModule, MatTooltipModule],
  templateUrl: './profile-bar.component.html',
  styleUrl: './profile-bar.component.scss',
  changeDetection: ChangeDetectionStrategy.OnPush
})
export class ProfileBarComponent {
  sps = inject(SettingsProfileService);
  private dialog = inject(DialogService);
  // true while a profile action runs, so a second click cannot start another one
  busy = signal(false);

  constructor() {
    this.sps.init();
  }

  private async run(action: () => Promise<void>) {
    if (this.busy()) {
      return;
    }
    this.busy.set(true);
    try {
      await action();
    } catch (e) {
      console.error('settings profile action failed', e);
      await this.dialog.message($localize`Profile error`, `${e}`);
    } finally {
      this.busy.set(false);
    }
  }

  // Asks what to do with unsaved changes. Resolves false when the user cancels.
  private async resolveUnsavedChanges(): Promise<boolean> {
    if (!this.sps.isDirty()) {
      return true;
    }
    const name = this.sps.activeProfile()?.name ?? '';
    const choice = await this.dialog.choose(
      $localize`Unsaved changes`,
      $localize`Profile "${name}" has unsaved changes.`,
      $localize`Save`,
      $localize`Discard`,
      'primary');
    if (choice === undefined) {
      return false;
    }
    if (choice) {
      await this.sps.saveActive();
    }
    return true;
  }

  private async askName(title: string, message: string, value: string) {
    const names = this.sps.profiles().map(p => p.name);
    return await this.dialog.prompt({
      title,
      message,
      label: $localize`Profile name`,
      value,
      okText: $localize`Create`,
      validate: v => validateProfileName(v, names)
    });
  }

  async select(id: string, profileSelect: MatSelect) {
    await this.run(async () => {
      if (id != this.sps.activeId() && await this.resolveUnsavedChanges()) {
        await this.sps.select(id);
      }
    });
    // the select keeps the clicked option even when the switch is cancelled or skipped, so put it back to the active profile
    profileSelect.value = this.sps.activeId();
  }

  save() {
    this.run(() => this.sps.saveActive());
  }

  discard() {
    this.run(async () => {
      const profile = this.sps.activeProfile();
      if (!profile) {
        return;
      }
      if (await this.dialog.confirm($localize`Discard changes`, $localize`Load the saved settings of profile "${profile.name}" again?`, $localize`Discard`)) {
        await this.sps.select(profile.id);
      }
    });
  }

  create() {
    this.run(async () => {
      if (!await this.resolveUnsavedChanges()) {
        return;
      }
      const name = await this.askName($localize`New profile`, $localize`The new profile starts from the driver defaults. Headsets stay enabled or disabled as they are now.`, '');
      if (name) {
        await this.sps.createFromDefaults(name);
      }
    });
  }

  copy() {
    this.run(async () => {
      const source = this.sps.activeProfile();
      const message = source
        ? (this.sps.isDirty()
          ? $localize`The new profile gets the current settings, including the unsaved changes. Profile "${source.name}" keeps its saved settings.`
          : $localize`The new profile gets a copy of the settings of "${source.name}".`)
        : $localize`The new profile gets the current settings.`;
      const name = await this.askName($localize`Copy profile`, message, source ? $localize`${source.name} copy` : '');
      if (name) {
        await this.sps.createFromCurrent(name);
      }
    });
  }

  delete() {
    this.run(async () => {
      const profile = this.sps.activeProfile();
      if (!profile) {
        return;
      }
      if (await this.dialog.confirm($localize`Delete profile`, $localize`Delete profile "${profile.name}"? The current settings stay as they are.`, $localize`Delete`)) {
        await this.sps.delete(profile.id);
      }
    });
  }
}
