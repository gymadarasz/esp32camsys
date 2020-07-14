import { Component, OnInit } from '@angular/core';
import { ClientDeviceService } from '../client-device.service';
import { ClientDevice } from '../client-device';
import { ClientDevicesChangeListener } from '../client-devices-change-listener';
//import { ClientDevice } from '../client-device';
//import { ClientDeviceModel } from '../client-device-model';

@Component({
  selector: 'app-camera-server',
  templateUrl: './camera-server.component.html',
  styleUrls: ['./camera-server.component.css']
})
export class CameraServerComponent implements OnInit, ClientDevicesChangeListener {

  WebSocketServer = this.require('websocket').server;
  http = this.require('http');

  server: any;
  wsServer: any;

  //clientDevices: ClientDeviceModel[] = [];

  clientDevicesCount = 0;

  constructor(private clientDeviceService: ClientDeviceService) { }

  ngOnInit(): void {
    this.server = this.http.createServer((request: any, response: any) => {
      console.log((new Date()) + ' Received request for ' + request.url);
      response.writeHead(404);
      response.end();
    });
    this.server.listen(8080, () => {
      console.log((new Date()) + ' Server is listening on port 8080');
    });

    this.wsServer = new this.WebSocketServer({
      httpServer: this.server,
      // You should not use autoAcceptConnections for production
      // applications, as it defeats all standard cross-origin protection
      // facilities built into the protocol and the browser.  You should
      // *always* verify the connection's origin and decide whether or not
      // to accept it.
      autoAcceptConnections: false
    });

    this.wsServer.on('request', (request: any) => {
      console.log('request:', request);
      this.clientDeviceService.createClientDevice().onConnected(request);
    });

    this.clientDeviceService.registerClientDevicesChange(this);
  }

  onClientDevicesChange(clientDevices: ClientDevice[]) {
    this.clientDevicesCount = clientDevices.length;
  }

  // TODO require service
  require(moduleName: string) {
    const req = 'require';
    const require = window[req] ? window[req] : this.fakeRequire;
    const module = require(moduleName);
    return module;
  }

  fakeRequire(moduleName: string) {
    console.warn('window.require("' + moduleName + '") won\'t work.');
  }


}
