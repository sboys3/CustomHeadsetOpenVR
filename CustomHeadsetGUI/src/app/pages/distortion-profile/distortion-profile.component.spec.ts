import { ComponentFixture, TestBed } from '@angular/core/testing';

import { DistortionProfileComponent } from './distortion-profile.component';

describe('DistortionProfileComponent', () => {
  let component: DistortionProfileComponent;
  let fixture: ComponentFixture<DistortionProfileComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      imports: [DistortionProfileComponent]
    })
    .compileComponents();

    fixture = TestBed.createComponent(DistortionProfileComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
