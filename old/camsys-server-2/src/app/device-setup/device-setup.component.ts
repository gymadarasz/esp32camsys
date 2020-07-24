import { Component, OnInit } from '@angular/core';
import { RequireService } from '../require.service';
import { ComPort } from '../com-port';
import { WifiCredentialList } from '../wifi-credential-list';
import { WifiCredential } from '../wifi-credential';
import { DeviceSetupWriter } from '../device-setup-writer';
import { HttpServerAppFactoryService } from '../http-server-app-factory.service';

@Component({
  selector: 'cam-device-setup',
  templateUrl: './device-setup.component.html',
  styleUrls: ['./device-setup.component.css']
})
export class DeviceSetupComponent implements OnInit {

  wifiSSID: string;
  wifiPassword: string;
  wifiCredentials: WifiCredentialList;
  hostAddressOrIP: string = this.getHostAddressOrIP();
  comPortPath: string = '';
  comPorts: ComPort[] = [];
  uploadSettingsDisabled: boolean = false;
  serialPort = this.require.import('serialport');
  serialDeviceWriter = new DeviceSetupWriter();
  readline = this.require.import('@serialport/parser-readline');
  
  constructor(private require: RequireService, private httpServerAppFactory: HttpServerAppFactoryService) {
    let deviceSetupData = JSON.parse(localStorage.getItem('deviceSetup')) ?? {
      wifiCredentials: [],
      secret: this.httpServerAppFactory.getSecret(),
    };
    this.wifiCredentials = new WifiCredentialList(
      deviceSetupData.wifiCredentials ?? []
    );

    setInterval(() => {
      this.serialPort.list().then((comPorts: ComPort[]) => {
        this.comPorts = comPorts;
        let found = false;
        for (let port of this.comPorts) {
          if (port.path == this.comPortPath) {
            found = true;
            break;
          }
        };
        if (!found) this.comPortPath = comPorts.length > 0 ? comPorts[0].path : this.comPortPath = '';
      });
    }, 2000);
  }

  ngOnInit(): void {
  }

  private showError(message: string) {
    console.error(message);
  }

  private showMessage(message: string) {
    console.log(message);
  }

  onAddWifiCredentialsClick() {
    if (!this.wifiSSID) {
      this.showError('SSID is required.');
    } else {
      if (!this.wifiCredentials.addCredential(this.wifiSSID, this.wifiPassword)) {
        this.showError('Unable to add credentials.');
      }
    }
  }

  onRemoveWifiCredentialsClick(ssid: string) {
    if (!this.wifiCredentials.removeCredential(ssid)) {
      this.showError('Unable to remove credentials.')
    }
  }

  getComPortInfo(comPort: ComPort): string {
    const info =
      (comPort.locationId ? ('location: ' + comPort.locationId + '\n') : '') +
      (comPort.manufacturer ? ('manufacturer: ' + comPort.manufacturer + '\n') : '') +
      (comPort.pnpId ? ('pnp: ' + comPort.pnpId + '\n') : '') +
      (comPort.productId ? ('product: ' + comPort.productId + '\n') : '') +
      (comPort.serialNumber ? ('serial number: ' + comPort.serialNumber + '\n') : '') +
      (comPort.vendorId ? ('vendor: ' + comPort.vendorId + '\n') : '');
    return info;
  }

  onUploadSettingsClick() {
    this.uploadSettingsDisabled = true;

    localStorage.setItem('deviceSetup', JSON.stringify({
      wifiCredentials: this.wifiCredentials.getCredentials(),
      secret: this.httpServerAppFactory.getSecret(),
    }));
    

    if (!this.comPortPath) {
      this.showError('Please select a port');
      return;
    }

    let message = 
      'UID STRING:\n' + this.httpServerAppFactory.getRandomToken(32)  + '\n' +
      'WIFI CREDENTIALS:\n' + this.getWifiCredentialsMessage() + '\n' +
      'HOST ADDRESS OR IP:\n' + this.getHostAddressOrIP() + '\n' +
      'SECRET:\n' + this.httpServerAppFactory.getSecret() + '\n' +
      'HTTP PREFIX:\n' + 'http'  + '\n' +
      'HTTP PORT:\n' + this.httpServerAppFactory.port  + '\n' +
      'COMMIT\n';

    // TODO: secret key send to client devices to identify each cameras in system

    // TODO: send message to selected port
    let port = this.serialDeviceWriter.write(this.serialPort, this.readline, this.comPortPath, message, (result: string) => {
      this.showMessage(result);
      port.close();
      this.uploadSettingsDisabled = false;
    }, (err: string) => {
      this.showError(err);
      console.log('PORT ERROR', err, port);
      this.uploadSettingsDisabled = false;
    });

  }

  private getWifiCredentialsMessage(): string {
    var messages = [];
    this.wifiCredentials.getCredentials().forEach((wifiCredential: WifiCredential) => {
      messages.push(wifiCredential.ssid + ':' + wifiCredential.password);
    });
    if (messages.length == 0) {
      messages.push(this.wifiSSID + ':' + this.wifiPassword);
    }
    return messages.join(';');
  }


  private getHostAddressOrIP(): string {
    let os = this.require.import('os');
    let interfaces = os.networkInterfaces();
    let addresses = [];
    for (let k in interfaces) {
      for (let k2 in interfaces[k]) {
        let address = interfaces[k][k2];
        if (address.family === 'IPv4' && !address.internal) {
          addresses.push(address.address);
        }
      }
    }
    return addresses.join(';');
  }

}
