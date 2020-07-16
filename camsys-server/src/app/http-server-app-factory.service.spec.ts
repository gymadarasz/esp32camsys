import { TestBed } from '@angular/core/testing';

import { HttpServerAppFactoryService } from './http-server-app-factory.service';

describe('HttpServerAppFactoryService', () => {
  let service: HttpServerAppFactoryService;

  beforeEach(() => {
    TestBed.configureTestingModule({});
    service = TestBed.inject(HttpServerAppFactoryService);
  });

  it('should be created', () => {
    expect(service).toBeTruthy();
  });
});
