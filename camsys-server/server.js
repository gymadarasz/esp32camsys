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

// ----------------- ReplayPage ----------------

class ReplayPage {

  constructor() {
    pages.subscribe('device-replay', this);
  }

  onPageShow() {
    // placeholder for subscription
  }

  onPageHide() {
    var cid = $('form[name="device-replay-form"] input[name="cid"]').val();
    if (cid && deviceList.devices[cid]) {
      deviceList.devices[cid].ws.send('!STREAM STOP\0');
    }
  }

  getReplayForm(cid, indexData) {
    var device = deviceList.devices[cid];
    if (!device.updates) return 'Device updates is missing';
    if (device.updates.mode != 'camera') return 'Incorrect device mode: ' + device.updates.mode + ', please select a camera recorder with SD card.';
    
    var secret = deviceSettings.secret;
    var ts = timestamp.now();
    var uri = `http://${device.ws.ip4}/replay?secret=${secret}&ts=${ts}`;

    var html = `
      <form name="device-replay-form">
        <input name="cid" type="hidden" value="${cid}">
        <input name="index-size" type="hidden" value="${indexData.size}">
        <input name="index-pos" type="hidden" value="0">

        <h3 class="center">Replay - <span class="led ${device.alert ? 'red' : ''}"></span> ${device.name}</h3>
        <br>

        <div class="device ${device.updates.mode}">
          <div class="frame center">
            <img class="stream" src="${uri}">
          </div>
        </div>

        <br class="clear">
        <div class="center">
          <input type="button" value="<" onclick="replayPage.onRewindClick('${cid}')">
          <input type="button" value=">" onclick="replayPage.onForwardClick('${cid}')">
          <br>
          <input type="button" value="Stop" onclick="replayPage.onStopClick('${cid}')">
          <input type="button" value="Delete" onclick="replayPage.onDeleteClick('${cid}')">
          <input type="button" value="Back.." onclick="replayPage.onBackClick('${cid}')">
        </div>
      </form>
    `;
    return html;
  }

  onRewindClick(cid) {
    var $pos = $('input[name="index-pos"]');
    var pos = parseInt($pos.val()) - 1;
    if (pos < 0) pos = 0;
    $pos.val(pos);

    var device = deviceList.devices[cid];
    device.ws.send(`!INDEX ${pos}\0`);
  }

  onForwardClick(cid) {
    var size = parseInt($('input[name="index-size"]').val());
    var $pos = $('input[name="index-pos"]');
    var pos = parseInt($pos.val()) + 1;
    if (pos >= size-1) pos = size-1;
    if (pos < 0) pos = 0;
    $pos.val(pos);

    var device = deviceList.devices[cid];
    device.ws.send(`!INDEX ${pos}\0`);
  }

  showReplayForm(cid) {
    deviceList.devices[cid].ws.send("?INDEX\0");
  }

  onIndexRetrieved(ws, indexData) {
      var html = this.getReplayForm(ws.cid, indexData);
      $('.page.device-replay').html(html);
  }

  onStopClick(cid) {
    deviceList.devices[cid].ws.send('!STREAM STOP\0');
  }

  onDeleteClick(cid) {
    deviceList.devices[cid].ws.send('!RECORD DELETE\0');
  }

  onBackClick(cid) {
    pages.show('device-view');
  }

}

const replayPage = new ReplayPage();

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
    var cid = $('form[name="device-view-form"] input[name="cid"]').val();
    deviceList.devices[cid].ws.send('!STREAM STOP\0');
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
      <form name="device-view-form">
        <input type="hidden" name="cid" value="${cid}">
        <h3 class="center">Motion sensor - <span class="led ${device.alert ? 'red' : ''}"></span> ${device.name}</h3>
        <br>

        <div class="device ${device.updates.mode}
            ${device.alert ? 'alert' : ''}">
          <div class="frame" onclick="deviceView.onMotionStreamClick('${cid}', this, event)">
            <img class="stream" src="${uri}">
            <div class="watcher" style="${wstyle}"></div>
          </div>
          <div class="clear"></div>
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
          <input type="button" value="Stop stream" onclick="deviceView.onStreamStopClick('${cid}')">
          <input type="button" value="Reset device" onclick="deviceView.onResetClick('${cid}')">
          <br>
          <input type="button" value="Save" onclick="deviceView.onSaveClick('${cid}')">
          <input type="button" value="Cancel" onclick="deviceView.onCancelClick('${cid}')">
          <input class="red" type="button" value="Alert" onclick="deviceView.onAlertClick('${cid}')">
        </div>
      </form>
    `;
    return html;
  }

  getDeviceCameraHtml(device) {
    var cid = device.ws.cid;
    var secret = deviceSettings.secret;
    var ts = timestamp.now();
    var uri = `http://${device.ws.ip4}/stream?secret=${secret}&ts=${ts}`;
    var html = `
      <form name="device-view-form">
        <input type="hidden" name="cid" value="${cid}">
        <h3 class="center">Camera recorder - <span class="led ${device.updates && device.updates.camera.recording ? 'red' : ''}"></span> ${device.name}</h3>
        <br>

        <div class="device ${device.updates.mode}">
          <div class="frame center">
            <img class="stream" src="${uri}">
          </div>
        </div>

        <br class="clear">
        <div class="center">
          <input type="button" value="Stop stream" onclick="deviceView.onStreamStopClick('${cid}')">
          <input type="button" value="Reset device" onclick="deviceView.onResetClick('${cid}')">
          
          <input type="button" value="Playback" onclick="deviceView.onPlaybackClick('${cid}')">        
          <input type="button" value="Cancel" onclick="deviceView.onCancelClick('${cid}')">
        </div>
      </form>
    `;
    return html;
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
      var cid = device.ws.cid;
      $('.page.device-view').html(html);
      if (device.updates && device.updates.mode == 'motion') {
        $('.page.device-view .slider.size').slider({min: 1, max: 20, value: device.updates.watcher.size, change: (event, ui) => { deviceView.onSizeSliderChange(cid, event, ui); }});
        $('.page.device-view .slider.raster').slider({min: 1, max: 20, value: device.updates.watcher.raster, change: (event, ui) => { deviceView.onRasterSliderChange(cid, event, ui); }});
        $('.page.device-view .slider.threshold').slider({min: 1, max: 2000, value: device.updates.watcher.threshold, change: (event, ui) => { deviceView.onThresholdSliderChange(cid, event, ui); }});
        $('.page.device-view .slider.value').slider({min: 1, max: 2000, value: 0, disabled: true});
      }  
      this.updateInterval = setInterval(() => {
        deviceList.devices[cid].connected ?
          $('.page.device-view').removeClass('disconnected') :
          $('.page.device-view').addClass('disconnected');
        device.ws.send('?UPDATE\0');
      }, 500);   
    });
  }

  sendUpdatedWatcher(cid, cb) {
    var x = deviceList.devices[cid].updates.watcher.x;
    var y = deviceList.devices[cid].updates.watcher.y;
    var size = deviceList.devices[cid].updates.watcher.size;
    var raster = deviceList.devices[cid].updates.watcher.raster;
    var threshold = deviceList.devices[cid].updates.watcher.threshold;
    var msg = `!WATCH ${x},${y},${size},${raster},${threshold}\0`;
    deviceList.devices[cid].ws.send(msg, cb);
  }

  showUpdatedWatcher(cid) {
    $('.page.device-view .watcher').attr('style', this.getWStyle(deviceList.devices[cid]));
    $('.page.device-view .watcher-size').html(deviceList.devices[cid].updates.watcher.size);
    $('.page.device-view .watcher-raster').html(deviceList.devices[cid].updates.watcher.raster);
    $('.page.device-view .watcher-threshold').html(deviceList.devices[cid].updates.watcher.threshold);
  }

  onMotionStreamClick(cid, that, event) {
    var offset = $(that).offset();
    var x = event.pageX - offset.left;
    var y = event.pageY - offset.top;
    x=parseInt(x/3);
    y=parseInt(y/3);
    deviceList.devices[cid].updates.watcher.x = x;
    deviceList.devices[cid].updates.watcher.y = y;
    this.showUpdatedWatcher(cid);
  }

  onSizeSliderChange(cid, elem, ui) {
    deviceList.devices[cid].updates.watcher.size = ui.value;
    this.showUpdatedWatcher(cid);
  }

  onRasterSliderChange(cid, elem, ui) {
    deviceList.devices[cid].updates.watcher.raster = ui.value;
    this.showUpdatedWatcher(cid);
  }

  onThresholdSliderChange(cid, elem, ui) {
    deviceList.devices[cid].updates.watcher.threshold = ui.value;
    this.showUpdatedWatcher(cid);
  }

  onStreamStopClick(cid) {
    deviceList.devices[cid].ws.send('!STREAM STOP\0', () => {
      pages.show('device-list');
    });
  }

  onResetClick(cid) {
    deviceList.devices[cid].ws.send('!RESET\0', () => {
      pages.show('device-list');
    });
  }

  onSaveClick(cid) {
    preloader.show();
    this.sendUpdatedWatcher(cid, () => {
      preloader.hide();
      pages.show('device-list');
    });
  }

  onCancelClick(cid) {
    deviceList.devices[cid].ws.send('!STREAM STOP\0', () => {
      pages.show('device-list');
    });
  }

  onDeviceUpdated(ws, updates) {
    if (updates.mode != 'motion') return;
    var cid = ws.cid;
    if (deviceList.devices[cid] && deviceList.devices[cid].ws.cid === ws.cid) {
      $('.page.device-view .motion-value').html(updates.watcher.diff_sum_max);
      $('.page.device-view .slider.value').slider( "option", "value", updates.watcher.diff_sum_max);
    }
  }

  onAlertClick(cid) {
    deviceList.devices[cid].alert = true;
    system.onDeviceAlert(cid, {});
    pages.show('device-list');
  }

  onPlaybackClick(cid) {
    replayPage.showReplayForm(cid);
    pages.show('device-replay');
  }

}

const deviceView = new DeviceView();

// ------------------ DeviceList ----------------------

class DeviceList {
  constructor() {
    this.devices = [];
    this.counter = new Counter('device');
    this.loadDeviceNames();
    this.streamStopInterval = setInterval(() => {
      this.sendStreamStopBroadcast();
    }, 3000);
    pages.subscribe('device-list', this);
  }

  onPageShow(classname) {
    // placeholder for pages subscription
  }

  onPageHide(classname) {
    this.sendStreamStopBroadcast();
    clearInterval(this.streamStopInterval);
  }

  sendStreamStopBroadcast() {
    this.devices.forEach((device) => {
      device.ws.send('!STREAM STOP\0');
    });
  }

  loadDeviceNames() {
    this.deviceNames = storage.getItem('device-names', {});
  }
  
  getDeviceHtml(device) {
    var mode = 'mode-unknown';
    if (device.updates && device.updates.mode) mode = 'mode-' + device.updates.mode;
    var html = `
      <div class="device ${mode} 
          ${(device.connected ? 'connected' : 'disconnected')} 
          ${(device.alert ? 'alert' : '')}">
        <div class="icon" onclick="deviceList.onDeviceClick('${device.ws.cid}')"></div>
        <div class="label">
          <span class="led 
            ${(device.alert || 
                (device.updates && device.updates.camera.recording) ? 
                  'red' : '')}"></span>
          <span class="name" onclick="deviceList.onDeviceNameClick('${device.ws.cid}')">${device.name}</span>
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
    html += `
        <br class="clear">
        <div class="center">
          <input type="button" value="Stop all stream" onclick="deviceList.onStreamStopAllClick()">
          <input type="button" value="Reset all device" onclick="deviceList.onResetAllClick()">
          <br>
          <input class="green" type="button" value="Alert OFF" onclick="deviceList.onAlertOffClick()">
        </div>
      </form>`;
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

  onStreamStopAllClick() {
    for (var cid in this.devices) {
      var device = this.devices[cid];
      device.ws.send('!STREAM STOP\0');
    }
  }

  onResetAllClick() {
    for (var cid in this.devices) {
      var device = this.devices[cid];
      device.ws.send('!RESET\0');
    }
  }

  onAlertOffClick() {
    system.alertStop();
  }
}

const deviceList = new DeviceList();

// ----------------------------------------

class System {
  
  constructor() {
    this.alert = false;
  }
  
  onDeviceAlert(ws, message) {
    var cid = ws.cid;
    this.alertStart(cid);
  }

  alertStart(cid) {
    $('body').addClass('alert');
    this.alert = true;
    if (cid && deviceList.devices[cid]) {
      deviceList.devices[cid].alert = true;
    }
    this.sendRecordStartBroadcast();
  }

  alertStop() {
    $('body').removeClass('alert');
    this.alert = false;
    for (var cid in deviceList.devices) {
      var device = deviceList.devices[cid];
      if (device.updates && device.updates.mode == 'motion') {
        deviceList.devices[cid].alert = false;
      }
    }
    this.sendRecordStopBroadcast();
    this.sendStreamStopBroadcast();
  }

  sendRecordStopBroadcast() {
    for (var cid in deviceList.devices) {
      var device = deviceList.devices[cid];
      if (device.updates && device.updates.mode == 'camera') {
        device.ws.send('!RECORD STOP\0');
      }
    }
  }

  sendRecordStartBroadcast() {
    for (var cid in deviceList.devices) {
      var device = deviceList.devices[cid];
      if (device.updates && device.updates.mode == 'camera') {
        device.ws.send('!RECORD START\0');
        // TODO pending on stream url to start streaming too!!! (or change the ESP code to record in loop function)
      }
    }
  }

  sendUpdateBroadcast() {
    for (var cid in deviceList.devices) {
      var device = deviceList.devices[cid];
        device.ws.send('?UPDATE\0');
    }
  }

  sendStreamStopBroadcast() {
    for (var cid in deviceList.devices) {
      var device = deviceList.devices[cid];
      device.ws.send('!STREAM STOP\0');
    }
  }

}

const system = new System();

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
      if (pages.is('device-list')) deviceList.onDeviceUpdated(ws, message);
      if (pages.is('device-view')) deviceView.onDeviceUpdated(ws, message);
      break;

      case 'alert':
      system.onDeviceAlert(ws, message);
      break;

      case 'index':
      if (pages.is('device-replay')) replayPage.onIndexRetrieved(ws, message);
      break;

//       case 'err':
          // TODO.....
//       break;

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
  if (!ws._socket.remoteAddress) {
    console.warn('no IP');
    ws.terminate();
    return;
  }
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
      var ok = true;
      try {
        var parsed = JSON.parse(message);        
      } catch (e) {
        //console.error('JSON parse error: ', message);
        ws.send('!RESET\0');
        ok = false;
      }
      if (ok) app.onClientMessage(ws, parsed);
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
  onReloadClick() {
    location.href = location.href;
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