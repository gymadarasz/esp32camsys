import { TestBed } from '@angular/core/testing';

import { RequireService } from './require.service';

describe('RequireService', () => {
  let service: RequireService;

  beforeEach(() => {
    TestBed.configureTestingModule({});
    service = TestBed.inject(RequireService);
  });

  it('should be created', () => {
    expect(service).toBeTruthy();
  });
});
