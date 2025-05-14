import { ComponentFixture, TestBed } from '@angular/core/testing';

import { BasicSettingsComponent } from './basic-settings.component';

describe('BasicSettingsComponent', () => {
  let component: BasicSettingsComponent;
  let fixture: ComponentFixture<BasicSettingsComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      imports: [BasicSettingsComponent]
    })
    .compileComponents();

    fixture = TestBed.createComponent(BasicSettingsComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
