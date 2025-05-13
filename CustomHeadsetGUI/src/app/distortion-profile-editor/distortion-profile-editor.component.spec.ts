import { ComponentFixture, TestBed } from '@angular/core/testing';

import { DistortionProfileEditorComponent } from './distortion-profile-editor.component';

describe('DistortionProfileEditorComponent', () => {
  let component: DistortionProfileEditorComponent;
  let fixture: ComponentFixture<DistortionProfileEditorComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      imports: [DistortionProfileEditorComponent]
    })
    .compileComponents();

    fixture = TestBed.createComponent(DistortionProfileEditorComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
