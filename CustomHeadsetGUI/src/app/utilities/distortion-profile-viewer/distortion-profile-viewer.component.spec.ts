import { ComponentFixture, TestBed } from '@angular/core/testing';

import { DistortionProfileViewerComponent } from './distortion-profile-viewer.component';

describe('DistortionProfileViewerComponent', () => {
  let component: DistortionProfileViewerComponent;
  let fixture: ComponentFixture<DistortionProfileViewerComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      imports: [DistortionProfileViewerComponent]
    })
    .compileComponents();

    fixture = TestBed.createComponent(DistortionProfileViewerComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
