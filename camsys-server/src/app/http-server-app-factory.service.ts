import { Injectable } from '@angular/core';
import { RequireService } from './require.service';
import { DeviceListComponent } from './device-list/device-list.component';

@Injectable({
  providedIn: 'root'
})
export class HttpServerAppFactoryService {

  express = this.require.import('express');
  bodyParser = this.require.import('body-parser');

  constructor(private require: RequireService) { }

  readonly port = 3000;

  static httpServerApp: any = null;

  static lastCaller: DeviceListComponent;

  getHttpServerApp(caller: DeviceListComponent) {
    HttpServerAppFactoryService.lastCaller = caller;

    if (!HttpServerAppFactoryService.httpServerApp) {
      HttpServerAppFactoryService.httpServerApp = this.express();
      HttpServerAppFactoryService.httpServerApp.use(this.bodyParser.urlencoded({
        extended: true,
      }));

      HttpServerAppFactoryService.httpServerApp.post('/join', (req: any, res: any) => {
        console.log("POST", req, res);
        if (req.body.secret !== this.getSecret()) {
          console.error('Invalid secret');
        } else {
          let camDevice = HttpServerAppFactoryService.lastCaller.camDeviceList.joinDevice(
            req.body.client, 
            req.body.type, 
            req.body.base,
            req.body.diff_sum_max, 
            req.body.watcher,
            req.body.recording
          );
          res.send(HttpServerAppFactoryService.lastCaller.getMessageToDevice(camDevice));
        }
      });

      // this.httpServerApp.get('/join/:id', (req: any, res: any) => {
      //   caller.camDeviceList.joinDevice(req.params.id);
      //   console.log(caller.camDeviceList);
      //   caller.refreshDeviceList();

      //   res.send('ATTACHED');
      // });
      HttpServerAppFactoryService.httpServerApp.listen(this.port);
    }
    return HttpServerAppFactoryService.httpServerApp;
  }

  getSecret(): string {
    let storedDeviceSetup = JSON.parse(localStorage.getItem('deviceSetup'));
    if (storedDeviceSetup && storedDeviceSetup.secret) {
      return storedDeviceSetup.secret;
    }
    storedDeviceSetup.secret = this.getRandomToken(32);
    localStorage.setItem('deviceSetup', JSON.stringify(storedDeviceSetup));
    return storedDeviceSetup.secret;
  }

  getRandomToken(length: number): string {
    let result           = '';
    let characters       = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
    let charactersLength = characters.length;
    for ( let i = 0; i < length; i++ ) {
       result += characters.charAt(Math.floor(Math.random() * charactersLength));
    }
    return result;
  }

}
