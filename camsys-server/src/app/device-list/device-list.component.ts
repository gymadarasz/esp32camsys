import { Component, OnInit } from '@angular/core';
import { HttpServerAppFactoryService } from '../http-server-app-factory.service';

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

  joinDevice(id: string, type: string): CamDevice | null {
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
      foundDevice.type = type;
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


  camDeviceList: CamDeviceList = new CamDeviceList();

  constructor(private httpServerAppFactory: HttpServerAppFactoryService) {
    this.loadDeviceList();

    this.httpServerAppFactory.getHttpServerApp(this);

    setInterval(() => {
      let now = Math.round((new Date()).getTime());
      this.camDeviceList.camDevices.forEach((camDevice) => {
        camDevice.status = 'connected';
        console.log(now, camDevice.connectedAt, now - camDevice.connectedAt);
        if (now - camDevice.connectedAt > 10000) {
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
