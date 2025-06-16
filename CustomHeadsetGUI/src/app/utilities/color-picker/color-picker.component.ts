import { Component, effect, ElementRef, input, model } from '@angular/core';
import { LinearColor } from '../../services/JsonFileDefines'
import { MatInputModule } from '@angular/material/input'
import { FormsModule } from '@angular/forms'

@Component({
  selector: 'app-color-picker',
  imports: [
    MatInputModule,
    FormsModule,
  ],
  templateUrl: './color-picker.component.html',
  styleUrl: './color-picker.component.scss'
})
export class ColorPickerComponent {
  public colorValue = { r: 1, g: 1, b: 1 }
  public color = model<LinearColor>(this.colorValue);
  public useHSV = input(true)
  public showColorDisplay = input(false)
  public colorHSV: LinearColor = { r: 0, g: 0, b: 0 };
  public colorSaturated: LinearColor = { r: 1, g: 1, b: 1 };
  constructor(elementRef: ElementRef<HTMLElement>) {
    effect(() => {
      this.colorValue = this.color() || { r: 1, g: 1, b: 1 };
      let newHsv = this.rgbToHsv(this.colorValue);
      if (this.colorValue.r == this.colorValue.g && this.colorValue.g == this.colorValue.b) {
        newHsv.r = this.colorHSV.r;
      }
      if (this.colorValue.r == 0 && this.colorValue.g == 0 && this.colorValue.b == 0) {
        newHsv.g = this.colorHSV.g;
      }
      this.colorHSV = newHsv;
      elementRef.nativeElement.style.setProperty('--r', `${this.colorValue.r * 255}`)
      elementRef.nativeElement.style.setProperty('--g', `${this.colorValue.g * 255}`)
      elementRef.nativeElement.style.setProperty('--b', `${this.colorValue.b * 255}`)
      this.colorSaturated = this.hsvToRgb({ r: this.colorHSV.r, g: 1, b: 1 });
      elementRef.nativeElement.style.setProperty('--color-saturated-r', `${this.colorSaturated.r * 255}`)
      elementRef.nativeElement.style.setProperty('--color-saturated-g', `${this.colorSaturated.g * 255}`)
      elementRef.nativeElement.style.setProperty('--color-saturated-b', `${this.colorSaturated.b * 255}`)
    })
  }
  colorInputChanged() {
    this.color.set({ ...this.colorValue });
  }

  colorInputChangedHSV() {
    this.colorValue = this.hsvToRgb(this.colorHSV);
    this.color.set({ ...this.colorValue });
  }


  rgbToHsv(rgb: LinearColor): LinearColor {
    const r = rgb.r;
    const g = rgb.g;
    const b = rgb.b;
    const max = Math.max(r, g, b);
    const min = Math.min(r, g, b);
    let h = 0;
    let s = 0;
    let v = max;
    const d = max - min;
    s = max === 0 ? 0 : d / max;
    if (max === min) {
      h = 0; // achromatic
    } else {
      switch (max) {
        case r:
          h = (g - b) / d + (g < b ? 6 : 0);
          break;
        case g:
          h = (b - r) / d + 2;
          break;
        case b:
          h = (r - g) / d + 4;
          break;
      }
      h /= 6;
    }
    // round to prevent floating point errors
    h = Math.round(h * 1000000000) / 1000000000;
    s = Math.round(s * 1000000000) / 1000000000;
    v = Math.round(v * 1000000000) / 1000000000;
    return { r: h, g: s, b: v };
  }
  hsvToRgb(hsv: LinearColor): LinearColor {
    const h = hsv.r;
    const s = hsv.g;
    let v = hsv.b;
    const i = Math.floor(h * 6);
    const f = h * 6 - i;
    let p = v * (1 - s);
    let q = v * (1 - f * s);
    let t = v * (1 - (1 - f) * s);
    // round to prevent floating point errors
    v = Math.round(v * 1000000000) / 1000000000;
    p = Math.round(p * 1000000000) / 1000000000;
    q = Math.round(q * 1000000000) / 1000000000;
    t = Math.round(t * 1000000000) / 1000000000;

    let r = 0;
    let g = 0;
    let b = 0;
    switch (i % 6) {
      case 0:
        r = v;
        g = t;
        b = p;
        break;
      case 1:
        r = q;
        g = v;
        b = p;
        break;
      case 2:
        r = p;
        g = v;
        b = t;
        break;
      case 3:
        r = p;
        g = q;
        b = v;
        break;
      case 4:
        r = t;
        g = p;
        b = v;
        break;
      case 5:
        r = v;
        g = p;
        b = q;
        break
    }
    return { r: r, g: g, b: b };
  }
}
