import { Component, OnInit } from '@angular/core';

interface PortModel {
  comName: string;
  locationId?: string;
  manufacturer?: string;
  pnpId?: string;
  productId?: string;
  serialNumber?: string;
  vendorId?: string;
}

interface WifiModel {
  ssid: string;
  password?: string;
}

@Component({
  selector: 'app-device-setup',
  templateUrl: './device-setup.component.html',
  styleUrls: ['./device-setup.component.css']
})
export class DeviceSetupComponent implements OnInit {

  SerialPort = this.require('serialport');

  error: string = '';

  ports: PortModel[] = [];

  ssid: string = '';
  password: string = '';

  wifis: WifiModel[] = [];

  port: string = '';

  comm: any;

  constructor() { }

  ngOnInit(): void {
    this.SerialPort.list().then((ports: PortModel[]) => {
      this.ports = ports;
      console.log(ports);
    });
  }

  require(moduleName: string) {
    const req = 'require';
    const require = window[req] ? window[req] : this.fakeRequire;
    const module = require(moduleName);
    return module;
  }

  fakeRequire(moduleName: string) {
    console.warn('window.require("' + moduleName + '") won\'t work.');
  }
  
  getPortInfo(port: PortModel): string {
    const ret =
      (port.locationId ? ('location: ' + port.locationId + '\n') : '') +
      (port.manufacturer ? ('manufacturer: ' + port.manufacturer + '\n') : '') +
      (port.pnpId ? ('pnp: ' + port.pnpId + '\n') : '') +
      (port.productId ? ('product: ' + port.productId + '\n') : '') +
      (port.serialNumber ? ('serial number: ' + port.serialNumber + '\n') : '') +
      (port.vendorId ? ('vendor: ' + port.vendorId + '\n') : '');
    return ret;
  }

  showError(message: string) {
    this.error = message;
  }

  onAddWifiClick() {
    if (!this.ssid) {
      this.showError('SSID is required.');
      return;
    }
    var found = false;
    this.wifis.forEach((wifi: WifiModel) => {
      if (wifi.ssid == this.ssid) {
        found = true;
      }
    });
    if (!found) {
      this.wifis.push({
        ssid: this.ssid,
        password: this.password
      });
    } else {
      this.showError('SSID already exists, first you have to delete the exists one.');
    }
  }

  onRemoveWifiClick(ssid: string) {
    var found = -1;
    var i = 0;
    this.wifis.forEach((wifi: WifiModel) => {
      if (wifi.ssid == ssid) {
        found = i;
      }
      i++;
    });
    if (i >= 0) {
      this.wifis.splice(found, 1);
    } else {
      this.showError('SSID is not found.');
    }
  }

  serialConnect(path: string, baudrate: number, charset = 'utf8'): any {

    var port = new this.SerialPort(path, {
      baudRate: baudrate
    });

    port.on('error', (err: Error) => {
      console.error(err);
      // this.showError(err.message);
    });

    port.on('open', () => {
      console.log('port opened.');
      // var message =  this.getWifiListMessage();
      // port.write(message, charset, (err: Error) => {
      //   console.error(err);
      //   this.showError('Communication error.');
      // });
    });

    port.on('close', () => {
      console.log('port closed.');
    });

        // // Read data that is available but keep the stream in "paused mode"
    // this.port.on('readable', () => {
    //   let data = this.port.read();
    //   console.log('Data:', data);
    // });

    // // Switches the port into "flowing mode"
    // this.port.on('data', (data) => {
    //   console.log('Data:', data);
    // })


    const Readline = this.SerialPort.parsers.Readline;
    const lineStream = port.pipe(new Readline());
    lineStream.on('data', (data: any) => {
      console.log('Serial input:' + data);
    });

    return port;
  }

  serialSend(port: any, message: string, lineEnding = '\n', charset = 'utf8') {
    if (!port) {
      return this.showError('Connect first');
    }
    if (!message) { message = ''; }
    const outputs: string[] = message.split('\n');

    let end = false;
    outputs.forEach((output) => {
      if (!end) {
        port.write(output + lineEnding, charset, (err: any) => {
          if (err) {
            console.error(err);
            end = true;
          }
        });
      }
    });

    if (!end) {
      console.warn('message lost');
    }
  }

  onUploadWifiSettingsClick() {
    if (!this.port) {
      this.showError('Please select a port');
      return; 
    }

    var baudrate = 115200;
    var charset = 'utf8';
    var lineEnding = '\n';

    this.comm = this.serialConnect(this.port, baudrate, charset);

    var that = this;
    setTimeout(function() {
      //that.serialSend(that.comm, that.getWifiListMessage(), lineEnding, charset);
      that.comm.close();
    }, 1000);
    
  }

  getWifiListMessage(): string {
    var message = "WIFI LIST\n";
    this.wifis.forEach((wifi: WifiModel) => {
      message += wifi.ssid + ':' + wifi.password + '\n';
    });
    message += ".\n";
    return message;
  }

  
}
