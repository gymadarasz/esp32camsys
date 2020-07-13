export class DeviceSetupWriter {

    private baudrate = 115200;

    write(serialPort: any, readLine: any, comPortPath: string, message: string, cb: Function, cbErr: Function) {
        let result: '';

        let port = new serialPort(comPortPath, {
            baudRate: this.baudrate
        });

        port.on('error', (err: Error) => {
            console.error(err);
            cbErr(err.message);
        });

        port.on('open', () => {
            console.log('port opened.');
        });

        port.on('close', () => {
            console.log('port closed.');
        });

        let parser = port.pipe(new readLine());
        parser.on('data', (data: any) => {
            console.log('Serial input:' + data);
            port.close();
            // check ECHO response to see if write success
            if (data === 'ECHO:' + message) {
                cb(data);
            } else {
                cbErr(data);
            }
        });

        port.write(message, (err: any) => {
            if (err) {
                cbErr(err.message);
            }
            console.log('message written');

        });

    }
}

