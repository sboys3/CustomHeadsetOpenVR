import { ComponentFixture, TestBed } from '@angular/core/testing';

import { SystemReadyComponent } from './system-ready.component';

describe('SystemReadyComponent', () => {
  let component: SystemReadyComponent;
  let fixture: ComponentFixture<SystemReadyComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      imports: [SystemReadyComponent]
    })
    .compileComponents();

    fixture = TestBed.createComponent(SystemReadyComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
