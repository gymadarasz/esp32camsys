import { Component, OnInit } from '@angular/core';
import { RequireService } from '../require.service';

class CamDevice {

  id: string;
  type: string;
  name: string;
  connectedAt: number;
  status: string;

}

class CamDeviceList {

  camDevices: CamDevice[] = [];

  private addCamDevice(camDevice: CamDevice) {
    this.camDevices.push(camDevice);
  }

  joinDevice(id: string): CamDevice | null {
    let foundDevice: CamDevice = null;
    this.camDevices.forEach((camDevice) => {
      if (!foundDevice && camDevice.id === id) {
        camDevice.connectedAt = Math.round((new Date()).getTime());
        foundDevice = camDevice;
      }
    });

    if (!foundDevice) {
      foundDevice = new CamDevice();
      foundDevice.id = id;
      foundDevice.connectedAt = Math.round((new Date()).getTime());
      this.addCamDevice(foundDevice);
    }

    return foundDevice;
  }
}

@Component({
  selector: 'cam-device-list',
  templateUrl: './device-list.component.html',
  styleUrls: ['./device-list.component.css']
})
export class DeviceListComponent implements OnInit {

  static readonly port = 3000;

  camDeviceList: CamDeviceList = new CamDeviceList();
  express = this.require.import('express');
  bodyParser = this.require.import('body-parser');
  static httpServerApp: any = null;

  static getHttpServerApp(caller: DeviceListComponent) {
    if (!this.httpServerApp) {
      this.httpServerApp = caller.express();
      this.httpServerApp.use(caller.bodyParser.urlencoded({
        extended: true,
      }));

      this.httpServerApp.post('/join', (req: any, res: any) => {
        console.log("POST", req, res);
        res.send('HELLO POSTER');
      });

      // this.httpServerApp.get('/join/:id', (req: any, res: any) => {
      //   caller.camDeviceList.joinDevice(req.params.id);
      //   console.log(caller.camDeviceList);
      //   caller.refreshDeviceList();

      //   res.send('ATTACHED');
      // });
      this.httpServerApp.listen(DeviceListComponent.port);
    }
    return this.httpServerApp;
  }

  constructor(private require: RequireService) {
    this.loadDeviceList();

    DeviceListComponent.getHttpServerApp(this);

    setInterval(() => {
      let now = Math.round((new Date()).getTime());
      this.camDeviceList.camDevices.forEach((camDevice) => {
        camDevice.status = 'connected';
        console.log(now, camDevice.connectedAt, now - camDevice.connectedAt);
        if (now - camDevice.connectedAt > 3) {
          camDevice.status = '...';
        }
      });
    }, 5000);
  }

  ngOnInit(): void {
  }

  refreshDeviceList() {
    localStorage.setItem('camDevices', JSON.stringify(this.camDeviceList.camDevices));
    this.loadDeviceList();
  }

  loadDeviceList() {
    this.camDeviceList.camDevices = JSON.parse(localStorage.getItem('camDevices')) ?? [];
  }

  onForgetClick(id: string) {
    setTimeout(() => {
      let tmp = [];
      for (let key in this.camDeviceList.camDevices) {
        let camDevice = this.camDeviceList.camDevices[key];
        if (camDevice.id !== id) {
          tmp.push(camDevice);
        }
      }
      this.camDeviceList.camDevices = tmp;
      this.refreshDeviceList();
    }, 500);
  }

}
