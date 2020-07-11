import { TestBed } from '@angular/core/testing';

import { ClientDeviceService } from './client-device.service';

describe('ClientDeviceService', () => {
  let service: ClientDeviceService;

  beforeEach(() => {
    TestBed.configureTestingModule({});
    service = TestBed.inject(ClientDeviceService);
  });

  it('should be created', () => {
    expect(service).toBeTruthy();
  });
});
