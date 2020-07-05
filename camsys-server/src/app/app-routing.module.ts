import { NgModule } from '@angular/core';
import { RouterModule, Routes } from '@angular/router';
import { StartPageComponent } from './start-page/start-page.component';
import { DeviceSetupComponent } from './device-setup/device-setup.component';

const routes: Routes = [
  { path: '', component: StartPageComponent },
  { path: 'start-page', component: StartPageComponent },
  { path: 'device-setup', component: DeviceSetupComponent }
];

@NgModule({
  imports: [RouterModule.forRoot(routes)],
  exports: [RouterModule]
})
export class AppRoutingModule { }