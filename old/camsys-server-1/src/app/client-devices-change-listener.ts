import { ClientDevice } from './client-device';

export interface ClientDevicesChangeListener {
    onClientDevicesChange(clientDevices: ClientDevice[]): void;
}
