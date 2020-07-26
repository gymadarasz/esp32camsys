

const Handlebars = require("handlebars");
const $ = require("jquery");
const WebSocket = require('ws');

class Template {
    constructor(selector, data) {
        this.selector = selector;
        this.template = Handlebars.compile($(selector).html());
        if (data) this.show(data);
    }
    show(data) {
        $(this.selector).html(this.template(data));
        $(this.selector).show();
    }
}

// var tpl = new Template('#page', {
//     title: "Camsys", 
//     body: "Hello World!"
// });


class WebsocketServer {
    constructor(options, onConnectionCallback) {
        this.wss = new WebSocket.Server(options);
        
        this.wss.on('connection', onConnectionCallback);
    }
}

var server = new WebsocketServer({ 
    port: 8080,
    // path: '/' + pathkey, // TODO: accept connection only on this URL
    // binaryType: 'fragments',
    clientTracking: true,
}, function connection(ws) {
    console.log("client connected", ws, server);
    ws.on('message', function incoming(message) {
        console.log('received:', message);
    });

    ws.send('something');
});







