import { Directive, effect, HostBinding, input } from '@angular/core';
import { MatTooltip } from '@angular/material/tooltip';
export type noteType = 'steamvrRestart' | 'gameRestart'
@Directive({
  selector: '[appSettingNote]',
  standalone: true,
  hostDirectives: [MatTooltip],

})
export class SettingNoteDirective {
  @HostBinding('attr.setting-note') settingNote?: string
  noteType = input.required<noteType>({ alias: 'appSettingNote' })
  static messages: Record<noteType, string> = {
    'steamvrRestart': $localize`Require restart SteamVR to apply changes.`,
    'gameRestart': $localize`Require restart game to apply changes.`
  }
  constructor(toolTip: MatTooltip) {
    effect(() => {
      const type = this.noteType()
      if (type in SettingNoteDirective.messages) {
        this.settingNote = type;
        toolTip.message = SettingNoteDirective.messages[type]
      } else {
        toolTip.message = ''
      }
    })
  }

}
