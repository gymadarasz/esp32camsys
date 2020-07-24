import { WifiCredential } from './wifi-credential';

export class WifiCredentialList {

    private wifiCredentials: WifiCredential[] = [];

    constructor(wifiCredentialsDefault: WifiCredential[] = []) {
        wifiCredentialsDefault.forEach((wifiCredentialDefault: WifiCredential) => {
            this.wifiCredentials.push(new WifiCredential(wifiCredentialDefault));
        });
    }

    addCredential(ssid: string, password: string): boolean {
        let key = this.getWifiCredentialKeyAt(ssid);
        if (key < 0) {
            this.wifiCredentials.push({
                ssid: ssid,
                password: password
            });
            return true;
        }
        return false;
    }

    private getWifiCredentialKeyAt(ssid: string): number {
        for (let key in this.wifiCredentials) {
            let wifiCredential: WifiCredential = this.wifiCredentials[key];
            if (wifiCredential.ssid === ssid) {
                return parseInt(key);
            }
        }
        return -1;
    }

    removeCredential(ssid: string): boolean {
        let key = this.getWifiCredentialKeyAt(ssid);
        if (key >= 0) {
            this.wifiCredentials.splice(key, 1);
            return true;
        }
        return false;
    }

    getCredentials(): WifiCredential[] {
        return this.wifiCredentials;
    }
}