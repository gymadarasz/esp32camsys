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
    var ret =  value && value != '[]' && 
      value != '{}' && value != 'undefined' ? 
        JSON.parse(value) : 
        defaultValue;
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
    storage.setItem(this.namespace + '-counter', this.next);
    return ret;
  }
}

const counter = new Counter('default');

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

// --------------------- DeviceSettings -------------------

const deviceSettingsDefaults = {
  creds: [],
  host: network.getIpAddresses()[0],
  port: 4443,
  secret: uid.makeid(24),
  mode: 'motion',
};

const deviceSettings = storage.getItem('device-settings', deviceSettingsDefaults);


// ------------------ Pages -----------------

class Pages {
  show(classname) {
    $('.page').hide();
    $('.page.'+classname).show();
  }
}

const pages = new Pages();

// ----------------  DeviceNameEditForm -----------------

class DeviceNameEditForm {

  setForm(cid, name) {
    $('form[name="device-name-edit-form"] input[name="cid"]').val(cid);
    $('form[name="device-name-edit-form"] input[name="name"]').val(name);
  }

  onOkClick() {
    var cid = $('form[name="device-name-edit-form"] input[name="cid"]').val();
    var name = $('form[name="device-name-edit-form"] input[name="name"]').val();
    deviceList.setDeviceName(cid, name);
    deviceList.showDeviceListHtml();
    pages.show('device-list');
  }

  onCancelClick() {
    pages.show('device-list');
  }
}

const deviceNameEditForm = new DeviceNameEditForm();

// ------------------ DeviceList ----------------------

class DeviceList {
  constructor() {
    this.devices = [];
    this.counter = new Counter('device');
    this.loadDeviceNames();
  }

  loadDeviceNames() {
    this.deviceNames = storage.getItem('device-names', {});
  }
  
  getDeviceHtml(device) {
    var mode = 'mode-unknown';
    if (device.updates && device.updates.mode) mode = 'mode-' + device.updates.mode;
    var html = `
      <div class="device ${mode} ${(device.connected ? 'connected' : 'disconnected')}">
        <div class="icon" onclick="deviceList.onDeviceClick('${device.ws.cid}')"></div>
        <div class="label">
          <span class="led"></span><span class="name" onclick="deviceList.onDeviceNameClick('${device.ws.cid}')">${device.name}</span>
        </div>
        <div class="title hidden">Device '${device.name}' from IP:${device.ws.ip4} has mode '${mode}' and now is ${(device.connected ? 'connected' : 'disconnected')}</div>
      </div>`;
    return html;
  }

  getDeviceListHtml() {
    var html = '';
    for (var cid in this.devices) {
      var device = this.devices[cid];
      html += this.getDeviceHtml(device);
    }
    return html;
  }

  showDeviceListHtml() {
    var html = this.getDeviceListHtml();
    $('div.device-list').html(html);
  }

  onDeviceConnected(ws) {
    this.devices[ws.cid] = {
      ws: ws,
      connected: true,
    }
    this.showDeviceListHtml();
    ws.send("?UPDATE");
  }

  setDeviceName(cid, name) {
    this.deviceNames[cid] = name;
    this.devices[cid].name = name;
    storage.setItem('device-names', this.deviceNames);
  }

  getDeviceName(cid) {
    if (!this.deviceNames[cid]) {
      this.setDeviceName(cid, 'Device #' + this.counter.getNext());
    }
    return this.deviceNames[cid];
  }

  onDeviceUpdated(ws, message) {
    this.devices[ws.cid].ws = ws;
    this.devices[ws.cid].connected = true;
    this.devices[ws.cid].updates = message;
    this.devices[ws.cid].lastUpdate = timestamp.now();
    this.devices[ws.cid].name = this.getDeviceName(ws.cid);
    this.showDeviceListHtml();
  }

  onDeviceDisconnect(ws) {
    this.devices[ws.cid].ws = ws;
    this.devices[ws.cid].connected = false;
    this.showDeviceListHtml();
  }

  onDeviceClick(cid) {
    pages.show('device-view');
  }

  onDeviceNameClick(cid) {
    var device = this.devices[cid];
    deviceNameEditForm.setForm(cid, device.name);
    pages.show('device-name-edit');
  }
}

const deviceList = new DeviceList();

// ----------------------------------------

class App {

  onClientConnected(ws) {
    deviceList.onDeviceConnected(ws);
  }

  onClientMessage(ws, message) {
    var func = message.func;
    delete message.func;
    switch (func) {
      case 'update':
        deviceList.onDeviceUpdated(ws, message);
        break;
      default:
        console.error('Unknown websocket message function: ' + func, 'message:', message);
    }
  }

  onClientDisconnect(ws) {
    deviceList.onDeviceDisconnect(ws);
  }

}

const app = new App();


// ----------------------Websocket.Server --------------------

const wss = new WebSocket.Server({port: deviceSettings.port});

const ip6to4 = function(ip6) {
  var ip4;
  ip6.split(':').forEach((split) => {
    if (/^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/.test(split)) return ip4 = split;
  });
  return ip4;
}

const isAuthMessage = function(message) {
  return /^HELLO [a-zA-Z0-9]{24} [a-zA-Z0-9]{24}$/.test(message);
}

const auth = function(message) {
  // a message could be "HELLO [secret-here] [cid-here]"
  if (isAuthMessage(message)) {
    var splits = message.split(' ');
    if (splits[1] == deviceSettings.secret) return splits[2];
  }
  return false;
}

wss.on('connection', (ws, req) => {
  ws.isAlive = true;
  ws.ip4 = ip6to4(ws._socket.remoteAddress);
  ws.on('pong', () => {
    ws.isAlive = true;
  });
  ws.on('message', (message) => {
    if (!ws.cid || isAuthMessage(message)) {
      if (ws.cid = auth(message)) app.onClientConnected(ws);
      else {
        console.warn('Client auth error');
        ws.send('Forbidden');
        ws.terminate();
        return;
      }
    } else {
      app.onClientMessage(ws, JSON.parse(message));
    }
  });

});

wss.pingInterval = setInterval(() => {
  wss.clients.forEach((ws) => {
    if (ws.isAlive === false) {
      if (ws.cid) app.onClientDisconnect(ws);
      return ws.terminate();
    }

    ws.isAlive = false;
    ws.ping(() => { });
  });
}, 3000);

wss.on('close', () => {
  clearInterval(wss.pingInterval);
});


// -------------------------------------------------------
// -------------------------- UI -------------------------
// -------------------------------------------------------


class Preloader {
  show() {
    $('.preloader').show();
  }

  hide() {
    $('.preloader').hide();
  }
}

const preloader = new Preloader();

// ------------------------------------------------

class Menu {
  onDeviceSettingsClick() {
    pages.show('device-settings');
  }
  onDeviceListClick() {
    pages.show('device-list');
  }
}

const menu = new Menu();

// -----------------------------------

const getIpAddresses = function() {
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


// ------------ Device Settings Form -----------------

class DeviceSettingsForm {
  constructor() {
    this.$wifis = $('form[name="device-settings-form"] ul.wifi-credentials');
    deviceSettings.creds.forEach((cred, i) => {
      this.addWifiCredentialInputs(cred.ssid, cred.pswd);
    });    
    
    var $host = $('form[name="device-settings-form"] input[name="host"]');
    $host.autocomplete({source: getIpAddresses()});
    $host.val(deviceSettings.host);
    
    var $port = $('form[name="device-settings-form"] input[name="port"]');
    $port.val(deviceSettings.port);
    
    var $secret = $('form[name="device-settings-form"] input[name="secret"]');
    $secret.val(deviceSettings.secret);
    
    var $mode = $('form[name="device-settings-form"] select[name="mode"]');
    $mode.val(deviceSettings.mode);

    var $paths = $('form[name="device-settings-form"] select[name="path"]');
    var pathsInterval = setInterval(() => {
      SerialPort.list().then((ports) => { 
        $paths.html('');   
        ports.forEach((port) => {  
          $paths.append(`<option value="${port.path}">${port.path}</option>`);
        });
      });
    }, 2000);
  }

  addWifiCredentialInputs(ssid, pswd) {
    this.$wifis.append(`
      <li class="cred">
        <input type="text" class="ssid" value="${ssid}" placeholder="ssid"><br>
        <input type="password" class="pswd" value="${pswd}" placeholder="pswd"><br>          
        <input type="button" value="Remove" onclick="deviceSettingsForm.onWifiCredentialRemoveClick(this)">
      </li>
    `);
  }

  onWifiCredentialAddClick() {
    this.addWifiCredentialInputs('', '');
  }

  onWifiCredentialRemoveClick(elem) {
    $(elem).closest('li.cred').remove();
  }

  getWifiCredentials() {
    var creds = [];
    $('form[name="device-settings-form"] li.cred').each((i, elem) => {
      var ssid = $(elem).find('input.ssid').val();
      var pswd = $(elem).find('input.pswd').val();
      creds.push({ssid:ssid, pswd:pswd,});
    });
    return creds;
  }

  onSetupClick() {

    // store form data 

    var deviceSettings = (new FormData($('form[name="device-settings-form"]'))).data;
    deviceSettings.creds = this.getWifiCredentials();
    var key = deviceSettings.key;
    delete deviceSettings.key;
    storage.setItem('device-settings', deviceSettings);

    // open serial port and send settings..

    const port = new SerialPort(deviceSettings.path, {baudRate: 115200});
    const parser = new Readline();
    port.pipe(parser);


    if (port.isOpen) {
      port.close();
    }
    
    port.on('open', (err) => {
      if (err) {
        console.error(err);
        alert(err);
        location.href = location.href;
      }
      console.log('serial open..');
      setTimeout(() => {
        port.write("HELLO\n")
      }, 1000);
    });

    var cred = 0;
    parser.on('data', (data) => {
      while (data.startsWith('\n') || data.startsWith('\r')) data = data.substring(1);

      console.log(data);
      
      if (data.startsWith('100 KEY')) {
        port.write(key + "\n");
      } else if (data.startsWith('100 SSID')) {
        while (!deviceSettings.creds[cred] || !deviceSettings.creds[cred].ssid) {
          cred++;
          if (cred >= 10) break;
        }
        if (cred < 10 && deviceSettings.creds[cred] && deviceSettings.creds[cred].ssid) {
          port.write(deviceSettings.creds[cred].ssid + "\n");
        } else {
          port.write("\n");
        }
      } else if (data.startsWith('100 PSWD')) {        
        port.write((deviceSettings.creds[cred] && deviceSettings.creds[cred].pswd ? deviceSettings.creds[cred].pswd : '') + "\n");
        cred++;
      } else if (data.startsWith('100 HOST')) {        
        port.write(deviceSettings.host + ":" + deviceSettings.port + "\n");
      } else if (data.startsWith('100 SECRET')) {        
        port.write(deviceSettings.secret + "\n");
      } else if (data.startsWith('100 CID')) {        
        port.write(uid.makeid(24) + "\n");
      } else if (data.startsWith('100 MODE')) {        
        port.write(deviceSettings.mode + "\n");
      } else if (data.startsWith('300')) {
        console.error(data);
        alert(data);
        location.href = location.href;
      }
    });
    
    port.on('error', function(err) {
      console.error(err);
      alert(err.message ? err.message : 'Unknown error');
      location.href = location.href;
    });
  }
}

const deviceSettingsForm = new DeviceSettingsForm();

// ------------------------------------------------

menu.onDeviceListClick();
preloader.hide();