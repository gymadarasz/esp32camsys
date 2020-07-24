export class WifiCredential {
    ssid: string;
    password?: string;
    constructor(wifiCredentialDefault: WifiCredential) {
        this.ssid = wifiCredentialDefault.ssid;
        this.password = wifiCredentialDefault.password;
    }
}
