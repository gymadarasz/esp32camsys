
import { ClientDeviceService } from './client-device.service'

export class ClientDevice {
    private mac: string;
    name: string;
    private connected: boolean = false;
    
    private request: any;
    private connection: any;

    constructor(private clientDeviceService: ClientDeviceService) { }
    
    onConnected(request: any) {
        this.request = request;
        
        if (!this.originIsAllowed(request.origin)) {
            // Make sure we only accept requests from an allowed origin
            request.reject();
            console.log((new Date()) + ' Connection from origin ' + request.origin + ' rejected.');
            return;
        }

        this.connection = request.accept(); //'echo-protocol', request.origin);
        console.log((new Date()) + ' Connection accepted.');;
        this.connection.on('message', (message: any) => {
            console.log('received message object:', message);
            if (message.type === 'utf8') {
                console.log('Received Message: ' + message.utf8Data);
                this.onMessageReceived(message);
            } else if (message.type === 'binary') {
                console.log('Received Binary Message of ' + message.binaryData.length + ' bytes');
                this.onDataReceived(message);
            } else {
                console.error('Unimplemented message type: ', message.type, message);
            }
        });

        this.connection.on('close', (reasonCode, description) => {
            console.log((new Date()) + ' Peer ' + this.connection.remoteAddress + ' disconnected.');
            this.clientDeviceService.removeClientDevice(this);
            this.onClose(reasonCode, description);
        });
    }

    private originIsAllowed(origin) {
        // put logic here to detect whether the specified origin is allowed.
        return true;
    }

    private onMessageReceived(message: any) {
        this.connection.sendUTF(message.utf8Data);
    }

    private onDataReceived(data: any) {
        this.connection.sendBytes(data.binaryData);
    }

    private onClose(reasonCode: any, description: any) {
        // TODO ...
    }

    getMac(): string {
        return this.mac;
    }
}