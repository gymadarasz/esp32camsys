import { Component, OnInit } from '@angular/core';

interface WifiCredential {
  ssid: string;
  password?: string;
}

interface ComPort {
  path: string;
  info: string;
}

@Component({
  selector: 'cam-device-setup',
  templateUrl: './device-setup.component.html',
  styleUrls: ['./device-setup.component.css']
})
export class DeviceSetupComponent implements OnInit {

  wifiSSID: string;
  wifiPassword: string;
  wifiCredentials: WifiCredential[];
  hostAddressOrIP: string;
  comPortPath: string;
  comPorts: ComPort[];
  uploadSettingsDisabled: boolean = false;

  constructor() { }

  ngOnInit(): void {
  }

  onAddWifiCredentialsClick() { }

  onRemoveWifiCredentialsClick(ssid: string) { }

  onUploadSettingsClick() { }

}
