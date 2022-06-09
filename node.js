const http = require('http');
const os = require('os');
console.log("Test server starting...");
var handler = function(req, res){
        res.writeHead(200);
        res.end("Container Hostname:" + os.hostname() + "\n");
};
var www = http.createServer(handler);
www.listen(8080);
