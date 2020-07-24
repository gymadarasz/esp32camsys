import { BrowserModule } from '@angular/platform-browser';
import { NgModule } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { HttpClientModule } from '@angular/common/http';

import { AppComponent } from './app.component';
import { MenuComponent } from './menu/menu.component';
import { DeviceSetupComponent } from './device-setup/device-setup.component';
import { DeviceListComponent } from './device-list/device-list.component';

@NgModule({
  declarations: [
    AppComponent,
    MenuComponent,
    DeviceSetupComponent,
    DeviceListComponent
  ],
  imports: [
    BrowserModule,
    FormsModule,
    HttpClientModule,
  ],
  providers: [],
  bootstrap: [AppComponent]
})
export class AppModule { }
