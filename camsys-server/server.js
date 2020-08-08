// require("modules/jquery-ui/themes/black-tie/jquery-ui.css");
// require("modules/jquery-ui/themes/black-tie/jquery-ui.theme.css");

const Handlebars = require("handlebars");
const $ = require("jquery");
const WebSocket = require('ws');

const SerialPort = require('serialport')
const Readline = require('@serialport/parser-readline')

const os = require('os');

// ----------------------- Storage ------------------------------

class Storage {

  getItem(key, defaultValue) {
    var value = localStorage.getItem(key);
    var ret = value && value != '[]' && value != '{}' ? JSON.parse(value) : defaultValue;
    return ret;
  }

  setItem(key, value) {
    localStorage.setItem(key, JSON.stringify(value));
  }

}

const storage = new Storage();

// ----------------------- Counter ------------------------------

class Counter {
  constructor(namespace) {
    this.namespace = namespace ? namespace : '';
    this.next = storage.getItem(namespace + '-counter', 1);
  }
  getNext() {
    var ret = this.next;
    this.next++;
    storage.setItem(namespace + '-counter', this.next);
    return ret;
  }
}

const counter = new Counter;

// ----------------- Timestamp ---------------------

class Timestamp {
  constructor() {
    if (!Date.now) {
      Date.now = function() { return new Date().getTime(); }
    }
  }
  now() {
    return Date.now() / 1000 | 0;
  }
}

const timestamp = new Timestamp();

// ------------------- IDMaker ---------------------

class UID {
  makeid(length) {
    var result           = '';
    var characters       = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
    var charactersLength = characters.length;
    for ( var i = 0; i < length; i++ ) {
      result += characters.charAt(Math.floor(Math.random() * charactersLength));
    }
    return result;
  }
}

const uid = new UID();

// ------------------- Network ---------------------

class Network {

  getIpAddresses() {
    var ifaces = os.networkInterfaces();
    var ips = [];
    Object.keys(ifaces).forEach(function (ifname) {
      ifaces[ifname].forEach(function (iface) {
        if ('IPv4' !== iface.family || iface.internal !== false) return;
        ips.push(iface.address);
      });
    });
    return ips;
  }

}

const network = new Network();

// ----------------- FormData -------------------

class FormData {
  constructor($form) {
    var unindexed_array = $form.serializeArray();
    var data = {};

    $.map(unindexed_array, (n, i) => {
        data[n['name']] = n['value'];
    });

    this.data = data;
  }
}

// ------------------ PRELOADER ---------------------

class UI_Preloader {
  hide() {
    $('div.preloader').hide();
  }
  show() {
    $('div.preloader').show();
  }
}

const  ui_Preloader = new UI_Preloader;




// ---------------- DEVICE SETTINGS --------------------

class UI_DeviceSettings {



  getDeviceSettings() {
    var deviceSettingsDefaults = {
      wifiCredentials: [],
      host: network.getIpAddresses()[0],
      port: 4443,
      secret: uid.makeid(24),
      mode: 'motion',
    };
    var deviceSettings = storage.getItem('device-setup', deviceSettingsDefaults);
    if (!deviceSettings || JSON.stringify(deviceSettings) === '{}') deviceSettings = deviceSettingsDefaults;
    return deviceSettings;
  }
}

const ui_DeviceSettings = new UI_DeviceSettings();

// --------------- MOTION DETECTOR SETTINGS ---------

class UI_MotionDeviceSettings {

  getMotionDeviceSettingsHtml(cid, name) {
    var device = devices.getDevice(cid);
    if (device.info.mode !== 'motion') {
      return `It's not a Motion Sensor but mode is: '${device.info.mode}'`;
    }
    var deviceSettings = ui_DeviceSettings.getDeviceSettings();
    var ip4 = device.ws.ip4;
    var secret = deviceSettings.secret;
    var ts = timestamp.now();
    var html = `
      <form name="motion-device-settings" class="motion-device-settings">

        <label>Device name:</label>
        <input type="text" name="name" placeholder="Name" value="${name}">
        <br>

        <img class="motion-stream" src="http://${ip4}/stream?secret=${secret}&ts=${ts}">

        <br>
        <div class="middle">
          <input type="button" value="Save" onclick="ui_MotionDeviceSettings.onSaveClick('${cid}')">
          <input type="button" value="Cancel" onclick="ui_MotionDeviceSettings.onCancelClick('${cid}')">
        </div>
      </form>
    `;
    return html;
  }

  showMotionDeviceSettings(cid, name) {
    var device = devices.getDevice(cid); 
    ui_Preloader.show();   
    device.ws.send('!STREAM STOP\0', () => {
      var html = this.getMotionDeviceSettingsHtml(cid, name);
      $('ul.motion-device-settings').html(html);
      $('ul.motion-device-settings').show();
      ui_Preloader.hide();
    });
  }

  onSaveClick(cid) {
    var data = (new FormData($('form[name="motion-device-settings"]'))).data;
    ui_MotionDeviceList.setMotionDeviceName(cid, data.name);

    // TODO Save here motion watcher settings too

    this.onCancelClick(cid);
  }

  onCancelClick(cid) {
    var device = devices.getDevice(cid);  
    ui_Preloader.show();  
    device.ws.send('!STREAM STOP\0', () => {
      $('ul.motion-device-settings').hide();
      ui_MotionDeviceList.show();
      ui_Preloader.hide();
    });
  }

}

const ui_MotionDeviceSettings = new UI_MotionDeviceSettings();

// --------------- MOTION DETECTORS ---------------

class UI_MotionDeviceList {

  constructor() {
    this.storage = storage;
    this.counter = new Counter('device');
    this.deviceNames = this.storage.getItem('device-names', {});
  }

  getMotionDeviceName(cid) {
    if (!this.deviceNames[cid]) {
      this.setMotionDeviceName(cid, 'Device #' + this.counter.getNext());
    }
    return this.deviceNames[cid];
  }

  setMotionDeviceName(cid, name) {
    this.deviceNames[cid] = name;
    this.storage.setItem('device-names', this.deviceNames);
  }

  getMotionDeviceHtml(device) {
    console.log('!', device);    
    if (device.info && device.info.mode != 'motion') return '';
    var isAliveClass = device.ws && device.ws.isAlive ? "connected" : "disconnected";
    var cid = device.ws && device.ws.cid ? device.ws.cid : ""
    var name = this.getMotionDeviceName(device.ws.cid);
    var ip4 = device.ws.ip4;
    var html = `
      <li class="device ${isAliveClass}" data-cid="${cid}" onclick="ui_MotionDeviceList.onMotionDeviceClick('${cid}')">
        <img class="light green" src="images/light-green.png">
        <img class="light gray"src="images/light-gray.png">
        <div class="infos">
          <span class="name">${name}</span><br>
          <span class="ip4">${ip4}</span><br>
        </div>
      </li>
    `;
    return html;
  }

  getMotionDeviceListHtml(deviceList) {
    var html = '';
    for (var key in deviceList.devices) {
      var device = deviceList.devices[key];
      if (device.info && device.info.mode == 'motion') {
        html += this.getMotionDeviceHtml(device);
      }
    }
    return html;
  }


  showMotionDevices(deviceList) {
    var html = this.getMotionDeviceListHtml(deviceList);
    $('ul.motion-device-list').html(html);
  }

  showMotionDevice(device) {
    var html = this.getMotionDeviceHtml(device);
    var cid = device.ws && device.ws.cid ? device.ws.cid : '';
    if ($(`ul.motion-device-list > li[data-cid="${cid}"]`).length) {
      $(`ul.motion-device-list > li[data-cid="${cid}"]`).replaceWith(html);
    } else {
      $(`ul.motion-device-list`).append(html);
    }
    ui_Preloader.hide();
  }

  show() {
     $('ul.motion-device-list').show();
  }

  onMotionDeviceClick(cid) {
    var name = this.getMotionDeviceName(cid);
    $('ul.motion-device-list').hide();
    ui_MotionDeviceSettings.showMotionDeviceSettings(cid, name);
  }
}

const ui_MotionDeviceList = new UI_MotionDeviceList();


// ----------------- HIDDEN UI ------------------------


class Device {

  constructor(ws) {
    this.ws = ws;
  }

}


class DeviceList {

  constructor(storage, namespace) {
    this.namespace = namespace
    this.storage = storage;
    this.devices = this.storage.getItem(this.namespace, []);
  }

  store() {
    this.storage.setItem(this.namespace, this.devices);
  }

  add(device) {
    this.devices[device.ws.cid] = device;
    this.store();
  }

  remove(cid) {
    console.log('disconnected:', this.devices[cid]); // TODO
  }

  update(cid, info) {
    this.devices[cid].info = info;
  }

  getDevice(cid) {
    return this.devices[cid];
  }

}

class DeviceServer {

  constructor(app, wss, cid) {
    this.app = app;
    this.wss = wss;
    this.cid = cid;

    this.wss.on('connection', (ws, req) => {
      ws.isAlive = true;
      ws.ip4 = this.ip6to4(ws._socket.remoteAddress);
      ws.on('pong', () => {
        ws.isAlive = true;
      });
      ws.on('message', (message) => {
        if (!ws.cid || this.cid.isAuthMessage(message)) {
          if (ws.cid = cid.auth(message)) this.app.onClientConnected(ws);
          else {
            console.warn('Client auth error');
            ws.send('Forbidden');
            ws.terminate();
            return;
          }
        } else {
          console.log("message", message);
          this.app.onClientMessage(ws, JSON.parse(message));
        }
      });

    });

    this.wss.pingInterval = setInterval(() => {
      this.wss.clients.forEach((ws) => {
        if (ws.isAlive === false) {
          if (ws.cid) this.app.onClientDisconnect(ws);
          return ws.terminate();
        }

        ws.isAlive = false;
        ws.ping(() => { });
      });
    }, 3000);

    this.wss.on('close', () => {
      clearInterval(this.wss.pingInterval);
    });
  }

  ip6to4(ip6) {
    var ip4;
    ip6.split(':').forEach((split) => {
      if (/^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/.test(split)) return ip4 = split;
    });
    return ip4;
  }

}


class CId {
  constructor(sys, storage) {
    this.deviceSetup = storage.getItem('device-setup', sys.getDeviceSetupDefaults());
  }
  isAuthMessage(message) {
    return /^HELLO [a-zA-Z0-9]{24} [a-zA-Z0-9]{24}$/.test(message);
  }
  auth(message) {
    // a message could be "HELLO [secret-here] [cid-here]"
    if (this.isAuthMessage(message)) {
      var splits = message.split(' ');
      if (splits[1] == this.deviceSetup.secret) return splits[2];
    }
    return false;
  }
}


class ComPortListener {
  constructor(serial, callback) {
    this.serial = serial;
    this.callback = callback;
    this.ports = [];
    this.portsHash = JSON.stringify(this.ports);
    this.portListenInterval = setInterval(() => {
      this.serial.list((ports) => {
        var hash = JSON.stringify(ports);
        if (hash !== this.portsHash) {
          this.ports = ports;
          this.portsHash = hash;
          callback(this.ports);
        }
      });
    }, 3000);
  }
}



class App {
  constructor(devices, ui) {
    this.devices = devices;
    this.ui = ui;
  }

  onClientConnected(ws) {
    this.devices.add(new Device(ws));
    this.ui.showDeviceList(this.devices);
    console.log('Device connected via websocket:', ws);
  }
  onClientMessage(ws, message) {
    // TODO
    console.log('Message received:', message);
    var func = message.func;
    delete message.func;
    switch (func) {
      case 'update':
        this.devices.update(ws.cid, message);
        this.ui.showDeviceInfo(this.devices, ws.cid);
        break;
      default:
        console.error('Unknown websocket message function: ' + func, 'message:', message);
    }
  }
  onClientDisconnect(ws) {
    this.devices.remove(ws.cid);
    this.ui.showDeviceList(this.devices);
    console.log('Websocket disconnected:', ws);
  }

}

class SerialCom {

  constructor(sys) {
    this.sys = sys;
  }

  open(path, options) {
    const port = new SerialPort(path, options);
    const parser = new Readline();
    port.pipe(parser);
    return {
      port: port,
      parser: parser,
    }
  }

  list(callback) {
    SerialPort.list().then((ports) => {
      callback(ports);
    });
  }

  serialWrite(port, message, encode, callback) {
    console.log(message);
    return port.write(message, encode ? encode : 'utf8', callback ? callback : () => {

    });
  }

  sendSetup(path, key, deviceSetup, options) {
    var cred = 0;
    if (!options) options = {baudRate: 115200};
    if (!options.baudRate) options.baudRate = 115200;
    var serial = this.open(path, options);
    if (serial.port.isOpen) {
      this.serial.port.close();
    }
    serial.port.on('open', (err) => {
      if (err) {
        console.error(err);
        alert(err);
        location.href = location.href;
      }
      console.log('serial open..');
      setTimeout(() => {
        this.serialWrite(serial.port, "HELLO\n")
      }, 1000);
    });
    serial.parser.on('data', (data) => {
      while (data.startsWith('\n') || data.startsWith('\r')) data = data.substring(1);

      console.log(data);
      
      if (data.startsWith('100 KEY')) {
        this.serialWrite(serial.port, key + "\n");
      } else if (data.startsWith('100 SSID')) {
        while (!deviceSetup.wifiCredentials[cred] || !deviceSetup.wifiCredentials[cred].ssid) {
          cred++;
          if (cred >= 10) break;
        }
        if (cred < 10 && deviceSetup.wifiCredentials[cred] && deviceSetup.wifiCredentials[cred].ssid) {
          this.serialWrite(serial.port, deviceSetup.wifiCredentials[cred].ssid + "\n");
        } else {
          this.serialWrite(serial.port, "\n");
        }
      } else if (data.startsWith('100 PSWD')) {        
        this.serialWrite(serial.port, (deviceSetup.wifiCredentials[cred] && deviceSetup.wifiCredentials[cred].pswd ? deviceSetup.wifiCredentials[cred].pswd : '') + "\n");
        cred++;
      } else if (data.startsWith('100 HOST')) {        
        this.serialWrite(serial.port, deviceSetup.host + ":" + deviceSetup.port + "\n");
      } else if (data.startsWith('100 SECRET')) {        
        this.serialWrite(serial.port, deviceSetup.secret + "\n");
      } else if (data.startsWith('100 CID')) {        
        this.serialWrite(serial.port, this.sys.makeid(24) + "\n");
      } else if (data.startsWith('100 MODE')) {        
        this.serialWrite(serial.port, deviceSetup.mode + "\n");
      } else if (data.startsWith('300')) {
        console.error(data);
        alert(data);
        location.href = location.href;
      }
    });
    serial.port.on('error', function(err) {
      console.error(err);
      alert(err.message ? err.message : 'Unknown error');
      location.href = location.href;
    });
  }
}





class System {
  constructor() {
    this.deviceSetupDefaults = {
      wifiCredentials: [],
      host: this.getIpAddresses()[0],
      port: 4443,
      secret: this.makeid(24),
      mode: 'motion',
    };
  }
  makeid(length) {
    return uid.makeid(length);
//     var result           = '';
//     var characters       = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
//     var charactersLength = characters.length;
//     for ( var i = 0; i < length; i++ ) {
//       result += characters.charAt(Math.floor(Math.random() * charactersLength));
//     }
//     return result;
  }
  getIpAddresses() {
    return network.getIpAddresses();
//     var ifaces = os.networkInterfaces();
//     var ips = [];
//     Object.keys(ifaces).forEach(function (ifname) {
//       ifaces[ifname].forEach(function (iface) {
//         if ('IPv4' !== iface.family || iface.internal !== false) return;
//         ips.push(iface.address);
//       });
//     });
//     return ips;
  }
  
  getDeviceSetupDefaults() {
    return this.deviceSetupDefaults;
  }
  
}

class UI {
  getDeviceSetup(deviceSetupDefaults) {
    var deviceSetup = this.storage.getItem('device-setup', deviceSetupDefaults);
    if (!deviceSetup || JSON.stringify(deviceSetup) === '{}') deviceSetup = deviceSetupDefaults;
    return deviceSetup;
  }
  constructor(ser, sys, storage) {
    this.ser = ser;
    this.sys = sys;
    this.storage = storage;
    var deviceSetup = this.getDeviceSetup(sys.getDeviceSetupDefaults());
    if (!deviceSetup.wifiCredentials || !deviceSetup.wifiCredentials.length) this.addWifiCredential('', '');
    else deviceSetup.wifiCredentials.forEach((cred) => {
      this.addWifiCredential(cred.ssid, cred.pswd);
    });
    var ips = sys.getIpAddresses();
    $('#host').autocomplete({source: ips});
    $('#host').val(deviceSetup.host ? deviceSetup.host : ips[0]);
    $('#port').val(deviceSetup.port ? deviceSetup.port : sys.getDeviceSetupDefaults().port);
    $('#secret').val((deviceSetup.secret && deviceSetup.secret.length == 24) ? deviceSetup.secret : sys.getDeviceSetupDefaults().secret);
    $('#mode').val(deviceSetup.mode ? deviceSetup.mode : sys.getDeviceSetupDefaults().mode);
  }

  onSetUpClick() {
    var key = $('#key').val();

    var deviceSetupStored = {};
    deviceSetupStored.wifiCredentials = this.getWifiCredentials();
    deviceSetupStored.host = $('#host').val();
    deviceSetupStored.port = $('#port').val();
    deviceSetupStored.secret = $('#secret').val();
    deviceSetupStored.mode = $('#mode').val();
    this.storage.setItem('device-setup', deviceSetupStored);

    var deviceSetup = deviceSetupStored;
    var path = $('#com-port').children("option:selected").val();
    if (!path) alert('Select serial comm port first!');
    else this.ser.sendSetup(path, key, deviceSetup);
  }

  getWifiCredentials() {
    var ret = [];
    if (!$('#wifi-credentials li').length) return ret;
    $('#wifi-credentials li').each((i, elem) => {
      var ssid = $(elem).find('.ssid').val();
      var pswd = $(elem).find('.pswd').val();
      if (ssid) ret.push({
        ssid: ssid,
        pswd: pswd
      });
    });
    return ret;
  }

  addWifiCredential(ssid, pswd) {
    var l = $('#wifi-credentials li').length;
    for (var i=0; i<l; i++) {
      if (!$('#wifi-credential-' + i).length) {
        break;
      }
    }
    $('#wifi-credentials').append(
      `<li id="wifi-credential-${i}">
        <input id="ssid-${i}" class="ssid" type="text" placeholder="ssid" value="${ssid}">
        <input id="pswd-${i}" class="pswd" type="password" placeholder="password" value="${pswd}">
        <input type="button" value="Remove" onclick="ui.removeWifiCredential(${i})">
      </li>`);
  }

  removeWifiCredential(i) {
    $('#wifi-credential-' + i).remove();
  }



  onComPortsChanged(ports) {

    var getComPortInfo = function(port) {
      var info = '';
      for (var key in port) {
        if (port[key]) {
          info += (key + ': ' + port[key] + '\n');
        }
      }
      return info;
    }

    $('#com-port').empty();
    ports.forEach((port) => {
      $('#com-port').append($('<option>', {
          value: port.path,
          text: port.path,
          title: getComPortInfo(port),
      }));
    });

  }

  getDeviceInfoCameraAsHtml(camera) {
    var ret = `
      <b>camera:</b><br>
      recording:${camera.recording}<br>
    `;
    return ret;
  }
  getDeviceInfoMotionAsHtml(watcher) {
    var ret = `
      <b>motion:</b><br>
      watcher:<br>
      diff_sum_max: ${watcher.diff_sum_max}<br>
      raster: ${watcher.raster}<br>
      size: ${watcher.size}<br>
      threshold: ${watcher.threshold}<br>
      x: ${watcher.x}<br>
      y: ${watcher.y}<br>
    `;
    return ret;
  }

  getNow() {
    return timestamp.now();
//     if (!Date.now) {
//       Date.now = function() { return new Date().getTime(); }
//     }
//     return Date.now() / 1000 | 0;
  }

  getDeviceInfoAsHtml(info) {
    var now = this.getNow();
    var deviceModeInfo = info.mode == 'camera' ? 
      this.getDeviceInfoCameraAsHtml(info.camera) : 
      this.getDeviceInfoMotionAsHtml(info.watcher);
    var ret = `
      mode: ${info.mode}<br>
      ---------<br>
      ${deviceModeInfo}
      ---------<br>
      streaming: ${info.streaming}<br>
      Last updated: <span class="last-updated">${now}</span>
    `;
    return ret;
  }

  getDeviceAsHtml(device) {
    console.log(device);
    var status = device.ws.isAlive ? 'connected' : 'disconnected';
    return `
      <li id="device-${device.ws.cid}" class="device ${status}">
        <b>device:</b>
        cid: ${device.ws.cid}<br>
        IP: ${device.ws.ip4}<br>
        status: ${status}<br>
        <div class="device-info"></div>

        Stream: <img class="stream" src=""><br>

        <input type="button" onclick="ui.startStream(app.devices.devices['${device.ws.cid}'])" value="start stream">
        <br>
        <input type="button" onclick="ui.stopStream(app.devices.devices['${device.ws.cid}'])" value="stop stream/record">
        <br>

        Record: <img class="record" src="">
        <br>

        <input type="button" onclick="ui.startRecord(app.devices.devices['${device.ws.cid}'])" value="start record">
        <input type="button" onclick="ui.replayRecord(app.devices.devices['${device.ws.cid}'])" value="replay record">
        <input type="button" onclick="ui.deleteRecord(app.devices.devices['${device.ws.cid}'])" value="delete record">
        <br>

        <input type="button" onclick="ui.updateDevice(app.devices.devices['${device.ws.cid}'])" value="update">
        <input type="button" onclick="ui.resetDevice(app.devices.devices['${device.ws.cid}'])" value="reset">
        <br>
        
        Motion:<br> 
        <input id="watcher" type="text" value="43,43,10,5,250"> (x,y,size,raster,threshold)<br>
        <input type="button" value="set watcher" onclick="ui.updateMotion(app.devices.devices['${device.ws.cid}'], $('#watcher').val())">
        <br>
      </li>`;
  }

  startStream(device) {
    $('#device-' + device.ws.cid + ' .stream').attr('src', 'http://' + device.ws.ip4 + '/stream?secret=' + this.getDeviceSetup(sys.getDeviceSetupDefaults()).secret + '&ts=' + this.getNow());
  }

  replayRecord(device) {
    $('#device-' + device.ws.cid + ' .record').attr('src', 'http://' + device.ws.ip4 + '/replay?secret=' + this.getDeviceSetup(sys.getDeviceSetupDefaults()).secret + '&ts=' + this.getNow());
  }

  stopStream(device) {
    device.ws.send('!STREAM STOP\0');
  }

  startRecord(device) {
    device.ws.send('!RECORD START\0');
  }

  stopRecord(device) {
    device.ws.send('!RECORD STOP\0');
  }

  deleteRecord(device) {
    device.ws.send('!RECORD DELETE\0');
  }

  updateDevice(device) {
    device.ws.send('?UPDATE\0');
    return this.getDeviceAsHtml(device);
  }

  updateDevices(deviceList) {
    var deviceListHtml = '';
    for(var key in deviceList.devices) {
      var device = deviceList.devices[key];
      deviceListHtml += this.updateDevice(device);
    }
    return deviceListHtml;
  }

  updateMotion(device, valuestr) {
    var msg = '!WATCH ' + valuestr + '\0';
    console.log(msg);
    device.ws.send(msg);
  }

  resetDevice(device) {
    device.ws.send('!RESET\0');
  }


  showDeviceList(deviceList) {
    $('#device-list ul').html(this.updateDevices(deviceList));
    ui_MotionDeviceList.showMotionDevices(deviceList);
  }

  showDeviceInfo(deviceList, cid) {
    $('#device-' + cid + ' .device-info').html(
      this.getDeviceInfoAsHtml(deviceList.devices[cid].info)
    );
    ui_MotionDeviceList.showMotionDevice(deviceList.devices[cid]);
  }
}


// const storage = new Storage();
const sys = new System();
const ser = new SerialCom(sys);
const ui = new UI(ser, sys, storage);
const devices = new DeviceList(storage, 'DeviceList');
const app = new App(devices, ui);
const cid = new CId(sys, storage);
const serl = new ComPortListener(ser, ui.onComPortsChanged);
const wss = new WebSocket.Server({port: sys.getDeviceSetupDefaults().port});
const srv = new DeviceServer(app, wss, cid);