//
//	index.js for bitlash terminal shell server, suitable for Heroku deployment
//
//	Copyright 2012 Bill Roy (MIT License)
//
var opt = require('optimist');
var argv = opt.usage('Usage: $0 [flags]')
	.alias('p', 'port')
	.describe('p', 'http port (default 8080)')
	.argv;

if (argv.help) {
	opt.showHelp();
	process.exit();
}

var port;
if (process && process.env && process.env.PORT) port = process.env.PORT;
else port = argv.port || 8080;

var config = {
	'users': {
		'bitlash':'open sesame'
	},
	// 'hostname': '0.0.0.0',		// to serve on all ports instead of just localhost
	'https': {
		'key': null,
		'cert': null
	},
	'port': port,
	'shell': 'src/bin/bitlash-linux-32-heroku',
	//'shellArgs': shellargs,
	//'limitGlobal': 1,
	//'limitPerUser': 1,
	'term': {}
};

var tty = require('tty.js');
var app = tty.createServer(config);
app.listen();

module.exports = app;
