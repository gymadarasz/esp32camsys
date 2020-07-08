import { NgModule } from '@angular/core';
import { RouterModule, Routes } from '@angular/router';
import { StartPageComponent } from './start-page/start-page.component';
import { DeviceSetupComponent } from './device-setup/device-setup.component';
import { CameraServerComponent } from './camera-server/camera-server.component';

const routes: Routes = [
  { path: '', component: StartPageComponent },
  { path: 'start-page', component: StartPageComponent },
  { path: 'device-setup', component: DeviceSetupComponent },
  { path: 'camera-server', component: CameraServerComponent },
];

@NgModule({
  imports: [RouterModule.forRoot(routes)],
  exports: [RouterModule]
})
export class AppRoutingModule { }