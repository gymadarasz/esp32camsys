import { Injectable } from '@angular/core';

@Injectable({
  providedIn: 'root'
})
export class RequireService {

  constructor() { }

  import(module: string): any {
    const req = 'require';
    const require = window[req] ? window[req] : this.fakeRequire;
    return require(module);
  }

  private fakeRequire(module: string): any {
    console.warn('require(' + module + ') won\t work here..');
    return null;
  }

}
