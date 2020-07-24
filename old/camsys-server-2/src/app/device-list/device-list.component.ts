import { Component, OnInit } from '@angular/core';
import { HttpServerAppFactoryService } from '../http-server-app-factory.service';
import { HttpClient } from '@angular/common/http';
import { RequireService } from '../require.service';

class CamDeviceWatcher {
  x: number = 43;
  y: number = 43;
  size: number = 10;
  raster: number = 5;
  threshold: number = 250;
}

class CamUserSet {
  watcher: CamDeviceWatcher = new CamDeviceWatcher();
}

class CamDevice {
  id: string;
  type: string;
  base: string;
  name: string = "Unnamed";
  updatedAt: number;
  status: string;
  diff: number;
  watcher: CamDeviceWatcher = new CamDeviceWatcher();
  userSet: CamUserSet = new CamUserSet();
  streamUrl: string;
  resetRequest: boolean = false;
  stopStream: boolean = false;
  recordRequest: boolean = false;
  recordStopRequest: boolean = false;
  recording: boolean = false;
}

class CamDeviceList {

  camDevices: CamDevice[] = [];

  private addCamDevice(camDevice: CamDevice) {
    this.camDevices.push(camDevice);
  }

  joinDevice(id: string, type: string, base: string, diffSumMax: number, watcher: CamDeviceWatcher, recording: boolean): CamDevice | null {
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
    foundDevice.updatedAt = StaticTime.getNow();
    foundDevice.type = type;
    foundDevice.base = base;
    
    if (type === 'motion') {
      foundDevice.diff = diffSumMax;
      foundDevice.watcher = watcher;
    } else if (type === 'camera') {
      foundDevice.recording = recording;
    } else {
      console.error('Invalid type: ', type);
    }

    return foundDevice;
  }
}

class StaticTime {

  public static getNow(): number {
    return Math.round((new Date()).getTime())
  }

}

class IntervalTimer {

  private timerId: number;
  private startTime: number;
  private remaining = 0;
  private state = 0; //  0 = idle, 1 = running, 2 = paused, 3= resumed

  constructor(private callback: Function, private interval: number) {
    this.startTime = StaticTime.getNow();
    this.timerId = window.setInterval(callback, interval);
    this.state = 1;
  }


  pause() {
    if (this.state != 1) return;

    this.remaining = this.interval - (StaticTime.getNow() - this.startTime);
    window.clearInterval(this.timerId);
    this.state = 2;
  }

  resume() {
    if (this.state != 2) return;

    this.state = 3;
    window.setTimeout(this.timeoutCallback, this.remaining);
  }

  timeoutCallback() {
    if (this.state != 3) return;

    this.callback();

    this.startTime = StaticTime.getNow();
    this.timerId = window.setInterval(this.callback, this.interval);
    this.state = 1;
  }

  kill() {
    window.clearInterval(this.timerId);
  }

}


@Component({
  selector: 'cam-device-list',
  templateUrl: './device-list.component.html',
  styleUrls: ['./device-list.component.css']
})
export class DeviceListComponent implements OnInit {


  camDeviceList: CamDeviceList = new CamDeviceList();

  refreshTimer: IntervalTimer;
  // refreshImagesTimer: IntervalTimer;

  userInCamDeviceSetField = false;

  now: number;

  recordButtonValue: string = 'Record';


  constructor(private require: RequireService, private httpServerAppFactory: HttpServerAppFactoryService, private httpClient: HttpClient) {

    this.httpServerAppFactory.getHttpServerApp(this);

    this.loadDeviceList();

    this.refreshTimer = new IntervalTimer(() => {
      this.now = StaticTime.getNow();
      //console.log(this.camDeviceList.camDevices);
      this.camDeviceList.camDevices.forEach((camDevice) => {
        //console.log(this.now, camDevice.updatedAt, this.now - camDevice.updatedAt);
        if (this.now - camDevice.updatedAt > 10000) {
          camDevice.status = 'disconnected';
        } else {
          if (camDevice.status != 'connected') {
            camDevice.streamUrl = this.getCamDeviceStreamUrl(camDevice);
          }
          camDevice.status = 'connected';
        }
      });
    }, 100);


    // --------------------- TEST (TODO VIDEO SAVING TO FILES) ----------------------
    // var request = this.require.import("request");
    // var MjpegConsumer = this.require.import("mjpeg-consumer");
    // var FileOnWrite = this.require.import("file-on-write");

    // var writer = new FileOnWrite({ 
    //   path: './video',
    //   ext: '.jpg'
    // });
    // var consumer = new MjpegConsumer();

    // request("http://192.168.0.106/stream?ts=123").pipe(consumer).pipe(writer);
    // ------------------------------------------------

    // this.refreshImagesTimer = new IntervalTimer(() => {
    //   this.camDeviceList.camDevices.forEach((camDevice, key) => {
    //     if (camDevice.status == 'connected') {
    //       //setTimeout(() => {
    //         //CamDevice.refreshImage(camDevice);
    //         var src = 'http://' + camDevice.base + '/image?ts=' + StaticTime.getNow();
    //         var img = document.createElement('img');
    //         img.src = src;
    //         var _camDevice = camDevice;
    //         img.onload = () => {
    //           console.log('------------------------------------------------------------------------', _camDevice.id);
    //           document.getElementById('camimg-'+_camDevice.id).setAttribute('src', src);
    //         }
            
    //       //}, 100);
    //     }
    //   });
    // }, 100);

  }

  getCamDeviceStreamUrl(camDevice: CamDevice): string {
    return 'http://' + camDevice.base + '/stream' + (camDevice.type=='motion' ? '-bmp' : '') + '?secret=' + this.httpServerAppFactory.getSecret() + '&ts=' + StaticTime.getNow();
  }

  ngOnInit(): void {
  }

  ngOnDestroy(): void {
    for (let key in this.camDeviceList.camDevices) {
      this.camDeviceList.camDevices[key].stopStream = true;
    }

    this.refreshTimer.kill();
    // this.refreshImagesTimer.kill();
    this.saveDeviceList();
  }

  saveDeviceList() {
    localStorage.setItem('camDevices', JSON.stringify(this.camDeviceList.camDevices));
  }

  loadDeviceList() {
    this.camDeviceList.camDevices = JSON.parse(localStorage.getItem('camDevices')) ?? [];

    for (let key in this.camDeviceList.camDevices) {
      this.camDeviceList.camDevices[key].resetRequest = false;
      this.camDeviceList.camDevices[key].stopStream = false;
      this.camDeviceList.camDevices[key].recordRequest = false;
      this.camDeviceList.camDevices[key].recordStopRequest = false;
      this.camDeviceList.camDevices[key].streamUrl = this.getCamDeviceStreamUrl(this.camDeviceList.camDevices[key]);
    }
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
    }, 500);
  }

  onResetClick(id: string) {
    setTimeout(() => {
      for (let key in this.camDeviceList.camDevices) {
        let camDevice = this.camDeviceList.camDevices[key];
        if (camDevice.id === id) {
          camDevice.resetRequest = true;
          break;
        }
      }
      
    }, 500);
  }

  onRecordClick(camDevice: CamDevice) {
    if (this.recordButtonValue == 'Record') {
      camDevice.recordRequest = true;
      this.recordButtonValue = 'Stop';
    } else {
      camDevice.recordStopRequest = true;
      this.recordButtonValue = 'Record';
    }
  }

  onStopStreamClick(camDevice: CamDevice) {
    camDevice.stopStream = true;
  }

  getMessageToDevice(camDevice: CamDevice): string {
    let message = "";

    if (camDevice.resetRequest == true) {
      camDevice.resetRequest = false;
      message += "reset=1\n";
    }

    if (camDevice.stopStream == true) {
      camDevice.stopStream = false;
      message += "stopStream=1\n";
    }

    if (camDevice.type === 'motion') {
      Object.keys(camDevice.watcher).forEach((key) => {
        if (camDevice.watcher[key] != camDevice.userSet.watcher[key]) {
          message += "watcher." + key + "=" + camDevice.userSet.watcher[key] + "\n";
        }
      });
    } else if (camDevice.type === 'camera') {
      if (camDevice.recordRequest == true) {
        camDevice.recordRequest = false;
        message += "recordRequest=" + StaticTime.getNow() + "\n";
      }
      if (camDevice.recordStopRequest == true) {
        camDevice.recordStopRequest = false;
        message += "recordStopRequest=1\n";
      }
    } else {
      console.error("Incorrect device type: " + camDevice.type);
    }
    console.log("MESSAGE:", message);
    return message;
  }

}
