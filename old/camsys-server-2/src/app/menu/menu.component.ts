import { Component, OnInit } from '@angular/core';

@Component({
  selector: 'cam-menu',
  templateUrl: './menu.component.html',
  styleUrls: ['./menu.component.css']
})
export class MenuComponent implements OnInit {

  selectedPage: string = 'deviceList';

  constructor() { }

  ngOnInit(): void {
  }

}
