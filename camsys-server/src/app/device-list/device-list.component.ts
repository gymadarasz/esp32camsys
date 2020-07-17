import { Component, OnInit } from '@angular/core';
import { HttpServerAppFactoryService } from '../http-server-app-factory.service';

class CamDeviceWatcher {
  x: number;
  y: number;
  size: number;
  raster: number;
  threshold: number;
}

class CamDevice {
  id: string;
  type: string;
  name: string;
  updatedAt: number;
  status: string;
  diff: number;
  watcher: CamDeviceWatcher = new CamDeviceWatcher();
}

class CamDeviceList {

  camDevices: CamDevice[] = [];

  private addCamDevice(camDevice: CamDevice) {
    this.camDevices.push(camDevice);
  }

  joinDevice(id: string, type: string, diffSumMax: number, watcher: CamDeviceWatcher): CamDevice | null {
    let foundDevice: CamDevice = null;
    this.camDevices.forEach((camDevice) => {
      if (!foundDevice && camDevice.id === id) {
        //camDevice.connectedAt = Math.round((new Date()).getTime());
        foundDevice = camDevice;
      }
    });

    if (!foundDevice) {
      foundDevice = new CamDevice();
      this.addCamDevice(foundDevice);
    }
  
    foundDevice.id = id;
    foundDevice.updatedAt = Math.round((new Date()).getTime());
    foundDevice.type = type;
    if (type === 'motion') {
      foundDevice.diff = diffSumMax;
      foundDevice.watcher = watcher;
    } else if (type === 'camera') {
      // TODO...
    } else {
      console.error('Invalid type: ', type);
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
console.log("CONSTRUCT");
    this.httpServerAppFactory.getHttpServerApp(this);

    this.loadDeviceList();
    
    setInterval(() => {
      let now = Math.round((new Date()).getTime());
      this.camDeviceList.camDevices.forEach((camDevice) => {
        camDevice.status = 'connected';
        console.log(now, camDevice.updatedAt, now - camDevice.updatedAt);
        if (now - camDevice.updatedAt > 10000) {
          camDevice.status = '...';
        }
      });
    }, 500);

  }

  ngOnInit(): void {
    console.log("NG ON INIT");
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
