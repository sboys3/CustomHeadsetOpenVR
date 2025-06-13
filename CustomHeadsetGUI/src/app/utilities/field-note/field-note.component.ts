import { Component, computed, HostBinding, input } from '@angular/core';
import { MatTooltipModule } from '@angular/material/tooltip';
export type noteType = 'steamvrRestart' | 'gameRestart'
@Component({
  selector: 'app-field-note',
  imports: [MatTooltipModule],
  templateUrl: './field-note.component.html',
  styleUrl: './field-note.component.scss',
})
export class FieldNoteComponent {
  type = input.required<noteType>()
  text = computed(() => {
    const type = this.type()
    if (type in FieldNoteComponent.messages) {
      return FieldNoteComponent.messages[type]
    }
    return ''
  })
  @HostBinding('class') get cssClass() {
    return this.type()
  }
  static messages: Record<noteType, string> = {
    'steamvrRestart': $localize`Requires restarting SteamVR to apply changes.`,
    'gameRestart': $localize`Requires restarting the game to apply changes.`
  }
}
