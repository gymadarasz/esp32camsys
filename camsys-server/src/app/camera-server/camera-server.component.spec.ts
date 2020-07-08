import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { CameraServerComponent } from './camera-server.component';

describe('CameraServerComponent', () => {
  let component: CameraServerComponent;
  let fixture: ComponentFixture<CameraServerComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ CameraServerComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(CameraServerComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
