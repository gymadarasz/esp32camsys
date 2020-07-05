import { Component } from '@angular/core';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent {
  title = 'camsys';
  
  constructor() {
    console.log('hello1');
    debugger;
    console.log('fooo');
  }
}
