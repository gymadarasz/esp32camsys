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
  constructor() {
    this.subscribes = {};
  }
  subscribe(classname, subscriber) {
    if (!this.subscribes[classname]) this.subscribes[classname] = [];
    this.subscribes[classname].push(subscriber);
  }
  show(classname) {
    if (this.subscribes[classname]) this.subscribes[classname].forEach((subscriber) => {
      subscriber.onPageShow(classname);
    });
    $('.page').each((i, elem) => {
      if ($(elem).is(':visible')) {
        var classes = $(elem).attr('class').split(/\s+/);
        for (var i = 0; i < classes.length; i++) {
          if (this.subscribes[classes[i]]) this.subscribes[classes[i]].forEach((subscriber) => {
            subscriber.onPageHide(classes[i]);
          });
        }
      }
    });
    $('.page').hide();
    $('.page.'+classname).show();
  }
  is(classname) {
    return $('.page.'+classname).is(":visible");
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

// ------------------- ErrorPage ----------------

class ErrorPage {
  show(message, returnPage) {
    $('form[name="error-page-form"] .msg').html(message);
    this.returnPage = returnPage;
    pages.show('error-page');
  }
  onOkClick() {
    pages.show(this.returnPage);
  }
}

const errorPage = new ErrorPage();

// ----------------- DeviceView -----------------

class DeviceView {

  constructor() {
    pages.subscribe('device-view', this);
  }

  onPageShow(classname) {
    // placeholder for pages subscription
  }

  onPageHide(classname) {
    clearInterval(this.updateInterval);
    preloader.show();
    this.device.ws.send('!STREAM STOP\0', () => {
      preloader.hide();
    });
  }

  getWStyle(device) {
    var wx = device.updates.watcher.x * 3; 
    var wy = device.updates.watcher.y * 3; 
    var ws = device.updates.watcher.size * 6;
    wx = wx - (ws/2);
    wy = wy - (ws/2);
    var wstyle = `top:${wy}px; left:${wx}px; width:${ws}px; height:${ws}px;`;
    return wstyle;
  }

  getDeviceMotionHtml(device) {
    var cid = device.ws.cid;
    var secret = deviceSettings.secret;
    var ts = timestamp.now();
    var uri = `http://${device.ws.ip4}/stream?secret=${secret}&ts=${ts}`;
    var wstyle = this.getWStyle(device);
    this.device = device;
    var html = `
      <form>
        <h3 class="center">Motion sensor</h3>
        <br>

        <div class="device ${device.updates.mode}">
          <div class="frame center" onclick="deviceView.onMotionStreamClick(this, event)">
            <img class="stream" src="${uri}">
            <div class="watcher" style="${wstyle}"></div>
          </div>

          size (<span class="watcher-size">${device.updates.watcher.size}</span>)
          <div class="slider size"></div>
          raster (<span class="watcher-raster">${device.updates.watcher.raster}</span>)
          <div class="slider raster"></div>
          threshold (<span class="watcher-threshold">${device.updates.watcher.threshold}</span>)
          <div class="slider threshold"></div>
          value (<span class="motion-value">?</span>)
          <div class="slider value"></div>
        </div>

        <br class="clear">
        <div class="center">
          <input type="button" value="Save" onclick="deviceView.onSaveClick()">
          <input type="button" value="Cancel" onclick="deviceView.onCancelClick()">
        </div>
      </form>
    `;
    return html;
  }

  getDeviceCameraHtml(device) {
    console.error("needs to implement");
  }

  getDeviceHtml(device) {
    if (!device.updates || !device.updates.mode) {
      errorPage.show('Device mode is not set.', 'device-view');
    } else {
      switch(device.updates.mode) {
        case 'motion':
          return this.getDeviceMotionHtml(device);
        case 'camera':
          return this.getDeviceCameraHtml(device);
        default:
          errorPage.show('Unknown device mode: ' + device.updates.mode, 'device-view');
      }
    }
  }

  showDeviceHtml(device) {
    preloader.show();
    device.ws.send('!STREAM STOP\0', () => {
      preloader.hide();
      var html = this.getDeviceHtml(device);
      $('.page.device-view').html(html);
      $('.page.device-view .slider.size').slider({min: 1, max: 20, value: device.updates.watcher.size, change: function(event, ui) { deviceView.onSizeSliderChange(event, ui); }});
      $('.page.device-view .slider.raster').slider({min: 1, max: 20, value: device.updates.watcher.raster, change: function(event, ui) { deviceView.onRasterSliderChange(event, ui); }});
      $('.page.device-view .slider.threshold').slider({min: 1, max: 2000, value: device.updates.watcher.threshold, change: function(event, ui) { deviceView.onThresholdSliderChange(event, ui); }});
      $('.page.device-view .slider.value').slider({min: 1, max: 2000, value: 0, disabled: true});
      this.updateInterval = setInterval(() => {
        device.ws.send('?UPDATE\0');
      }, 2000);      
    });
  }

  sendUpdatedWatcher(cb) {
    var x = this.device.updates.watcher.x;
    var y = this.device.updates.watcher.y;
    var size = this.device.updates.watcher.size;
    var raster = this.device.updates.watcher.raster;
    var threshold = this.device.updates.watcher.threshold;
    var msg = `!WATCH ${x},${y},${size},${raster},${threshold}\0`;
    this.device.ws.send(msg, cb);
  }

  showUpdatedWatcher() {
    $('.page.device-view .watcher').attr('style', this.getWStyle(this.device));
    $('.page.device-view .watcher-size').html(this.device.updates.watcher.size);
    $('.page.device-view .watcher-raster').html(this.device.updates.watcher.raster);
    $('.page.device-view .watcher-threshold').html(this.device.updates.watcher.threshold);
  }

  onMotionStreamClick(that, event) {
    var offset = $(that).offset();
    var x = event.pageX - offset.left;
    var y = event.pageY - offset.top;
    x=parseInt(x/3);
    y=parseInt(y/3);
    this.device.updates.watcher.x = x;
    this.device.updates.watcher.y = y;
    this.showUpdatedWatcher();
  }

  onSizeSliderChange(elem, ui) {
    this.device.updates.watcher.size = ui.value;
    this.showUpdatedWatcher();
  }

  onRasterSliderChange(elem, ui) {
    this.device.updates.watcher.raster = ui.value;
    this.showUpdatedWatcher();
  }

  onThresholdSliderChange(elem, ui) {
    this.device.updates.watcher.threshold = ui.value;
    this.showUpdatedWatcher();
  }

  onSaveClick() {
    preloader.show();
    this.sendUpdatedWatcher(() => {
      preloader.hide();
      pages.show('device-list');
    });
  }

  onCancelClick() {
    pages.show('device-list');
  }

  onDeviceUpdated(ws, updates) {
    if (pages.is('device-view') && this.device && this.device.ws.cid === ws.cid) {
      console.log('UPDATE:', ws, updates);
      $('.page.device-view .motion-value').html(updates.watcher.diff_sum_max);
      $('.page.device-view .slider.value').slider( "option", "value", updates.watcher.diff_sum_max);
    }
  }

}

const deviceView = new DeviceView();

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
    var html = `
      <form>
        <h3 class="center">Device list</h3>
        <br>
    `;
    for (var cid in this.devices) {
      var device = this.devices[cid];
      html += this.getDeviceHtml(device);
    }
    html += '</form>';
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
    ws.send("?UPDATE\0");
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
    deviceView.showDeviceHtml(this.devices[cid]);
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
        deviceView.onDeviceUpdated(ws, message);
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
        ws.send('Forbidden\0');
        ws.terminate();
        return;
      }
    } else {
      try {
        var parsed = JSON.parse(message)
      } catch (e) {
        console.error('JSON parse error: ', message);
      }
      app.onClientMessage(ws, parsed);
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