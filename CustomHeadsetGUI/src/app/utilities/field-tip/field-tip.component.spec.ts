import { ComponentFixture, TestBed } from '@angular/core/testing';

import { FieldTipComponent } from './field-tip.component';

describe('FieldTipComponent', () => {
  let component: FieldTipComponent;
  let fixture: ComponentFixture<FieldTipComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      imports: [FieldTipComponent]
    })
    .compileComponents();

    fixture = TestBed.createComponent(FieldTipComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
