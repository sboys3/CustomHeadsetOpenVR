import { ComponentFixture, TestBed } from '@angular/core/testing';

import { FieldNoteComponent } from './field-note.component';

describe('FieldNoteComponent', () => {
  let component: FieldNoteComponent;
  let fixture: ComponentFixture<FieldNoteComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      imports: [FieldNoteComponent]
    })
    .compileComponents();

    fixture = TestBed.createComponent(FieldNoteComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
