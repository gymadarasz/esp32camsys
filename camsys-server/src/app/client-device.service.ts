import { Injectable } from '@angular/core';
import { ClientDevice } from './client-device';
import { ClientDevicesChangeListener } from './client-devices-change-listener';

@Injectable({
  providedIn: 'root'
})
export class ClientDeviceService {

  clientDevices: ClientDevice[] = [];
  clientDevicesChangeListeners: ClientDevicesChangeListener[] = [];

  constructor() { }
  
  createClientDevice(): ClientDevice {
    let clientDevice = new ClientDevice(this);
    this.clientDevices.push(clientDevice);
    return clientDevice;
  }

  removeClientDevice(clientDevice: ClientDevice): number {
    let mac = clientDevice.getMac();
    let tempClientDevices: ClientDevice[] = [];
    let removed = 0;
    this.clientDevices.forEach((clientDeviceItem) => {
      if (mac !== clientDeviceItem.getMac()) {
        tempClientDevices.push(clientDeviceItem);
      } else {
        removed++;
      }
    });
    if (removed) {
      this.clientDevices = tempClientDevices;
      this.onClientDeviceRemoved();
    }
    return removed;
  }

  registerClientDevicesChange(clientDevicesChangeListener: ClientDevicesChangeListener) {
    this.clientDevicesChangeListeners.push(clientDevicesChangeListener);
  }

  private onClientDeviceRemoved() {
    this.clientDevicesChangeListeners.forEach((clientDevicesChangeListener) => {
      clientDevicesChangeListener.onClientDevicesChange(this.clientDevices);
    });
  }
}