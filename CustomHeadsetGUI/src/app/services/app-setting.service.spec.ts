import { TestBed } from '@angular/core/testing';

import { AppSettingService } from './app-setting.service';

describe('AppSettingService', () => {
  let service: AppSettingService;

  beforeEach(() => {
    TestBed.configureTestingModule({});
    service = TestBed.inject(AppSettingService);
  });

  it('should be created', () => {
    expect(service).toBeTruthy();
  });
});
