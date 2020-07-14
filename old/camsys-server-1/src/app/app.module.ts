import { BrowserModule } from '@angular/platform-browser';
import { NgModule } from '@angular/core';

import { AppComponent } from './app.component';
import { AppRoutingModule } from './app-routing.module';
import { DeviceSetupComponent } from './device-setup/device-setup.component';
import { StartPageComponent } from './start-page/start-page.component';
import { FormsModule } from '@angular/forms';
import { CameraServerComponent } from './camera-server/camera-server.component';

@NgModule({
  declarations: [
    AppComponent,
    StartPageComponent,
    DeviceSetupComponent,
    CameraServerComponent
  ],
  imports: [
    BrowserModule,
    AppRoutingModule,
    FormsModule,
  ],
  providers: [],
  bootstrap: [AppComponent]
})
export class AppModule { }
