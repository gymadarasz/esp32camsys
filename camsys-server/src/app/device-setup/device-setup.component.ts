import { Component, OnInit } from '@angular/core';

interface PortModel {
  path: string;
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

  ip: string;

  writeDisabled = false;

  constructor() { }

  ngOnInit(): void {
    setInterval(() => {
      this.SerialPort.list().then((ports: PortModel[]) => {
        this.ports = ports;
        var found = false;
        for (let port of this.ports) {
          if (port.path == this.path) {
            found = true;
            break;
          }
        };
        if (!found) this.path = ports.length > 0 ? ports[0].path : this.path = '';
      });
    }, 2000);
    
    this.ip = this.getIPListMessage();
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
    this.writeDisabled = true;

    if (!this.path) {
      this.showError('Please select a port');
      return;
    }

    this.serialConnect(this.path);

    setTimeout(() => {
      var message = [
        this.getWifiListMessage(),
        this.ip,
      ].join('\n');
      console.log("Message sending: " + message);
      this.port.write(message, (err: any) => {
        if (err) {
          return console.log('Error on write: ', err.message);
        }
        console.log('message written');
        // TODO: check ECHO response to see if write success
        
        setTimeout(() => {
          this.port.close();
          this.writeDisabled = false;
        }, 2000);
      });
    }, 2000);

  }

  getWifiListMessage(): string {
    var messages = [];
    this.wifis.forEach((wifi: WifiModel) => {
      messages.push(wifi.ssid + ':' + wifi.password);
    });
    if (messages.length == 0) {
      messages.push(this.ssid + ':' + this.password);
    }
    return messages.join(';');
  }

  getIPListMessage(): string {
    
    var os = this.require('os');
    var interfaces = os.networkInterfaces();
    var addresses = [];
    for (var k in interfaces) {
      for (var k2 in interfaces[k]) {
        var address = interfaces[k][k2];
        if (address.family === 'IPv4' && !address.internal) {
          addresses.push(address.address);
        }
      }
    }

    console.log(addresses);
    return addresses.join(';');
  }


}
