import { ComponentFixture, TestBed } from '@angular/core/testing';

import { DriverTroubleshooterComponent } from './driver-troubleshooter.component';

describe('DriverTroubleshooterComponent', () => {
  let component: DriverTroubleshooterComponent;
  let fixture: ComponentFixture<DriverTroubleshooterComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      imports: [DriverTroubleshooterComponent]
    })
    .compileComponents();

    fixture = TestBed.createComponent(DriverTroubleshooterComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
