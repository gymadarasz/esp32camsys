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
  Readline = this.require('@serialport/parser-readline');

  error: string = '';

  ports: PortModel[] = [];

  ssid: string = '';
  password: string = '';

  wifis: WifiModel[] = [];

  path: string = '';

  baudrate = 115200;
  //charset = 'utf8';
  //eof = '';

  port: any;
  parser: any;

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

  serialConnect(path: string) {

    this.port = new this.SerialPort(path, {
      baudRate: this.baudrate
    });

    this.port.on('error', (err: Error) => {
      console.error(err);
      this.showError(err.message);
    });

    this.port.on('open', () => {
      console.log('port opened.');
    });

    this.port.on('close', () => {
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

    var wifiSetupStep = 1;

    this.parser = this.port.pipe(new this.Readline());
    this.parser.on('data', (data: any) => {
      console.log('Serial input:' + data);
      if (wifiSetupStep = 1) {
        if (data.trim() == 'READY TO WIFI SETUP') {
          
          console.log('write message 24');
          var wifiListMessage = this.getWifiListMessage();
          wifiSetupStep = 2;
          this.port.write(wifiListMessage /*+ this.eof, this.charset*/, function(err: any) {
            if (err) {
              return console.log('Error on write: ', err.message);
            }
            console.log('message written');
          });
          //this.serialSend(this.port, this.getWifiListMessage(), lineEnding, charset);
        }
      } else if (wifiSetupStep == 2) {
        if (data.trim() == 'ECHO:' + this.getWifiListMessage()) {
          console.log('wifi setup success');
        } else {
          console.log('wifi setup error');
          this.showError('Wifi Setup Error: ' + data);
        }
        this.port.close();
        wifiSetupStep = 1;
      }
    });

  }

  // serialSend(port: any, message: string) {
  //   if (!port) {
  //     return this.showError('Connect first');
  //   }
  //   if (!message) { message = ''; }
  //   const outputs: string[] = message.split('\n');

  //   let end = false;
  //   outputs.forEach((output) => {
  //     if (!end) {
  //       port.write(output + this.eof, this.charset, (err: any) => {
  //         if (err) {
  //           console.error(err);
  //           end = true;
  //         }
  //       });
  //     }
  //   });

  //   if (!end) {
  //     console.warn('message lost');
  //   }
  // }

  onUploadWifiSettingsClick() {
    if (!this.path) {
      this.showError('Please select a port');
      return; 
    }


    this.serialConnect(this.path);

    var that = this;
    setTimeout(function() {
      //that.serialSend(that.comm, that.getWifiListMessage(), lineEnding, charset);
      //that.port.close();
    }, 10010);
    
  }

  getWifiListMessage(): string {
    var message = "";
    this.wifis.forEach((wifi: WifiModel) => {
      message += wifi.ssid + ':' + wifi.password + ';';
    });
    return message;
  }

  
}
