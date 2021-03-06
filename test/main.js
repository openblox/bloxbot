const irc = require("irc"), fs = require("fs"), spawn = require("child_process").spawn;

var testName = "bloxtest";

var cli = new irc.Client("irc.openblox.org", testName, {
	userName: testName,
	realName: testName,
	port: 6697,
	autoRejoin: false,
	autoConnect: true,
	channels: ["#bloxbottest"],
	selfSigned: true,
	secure: true
});

cli.on("motd", function(){
	cli.say("NickServ", "IDENTIFY bloxboot " + process.env["BB_PASSWD"]);
});

var bloxbootQuit = false;

cli.on("join#bloxbottest", function(nick){
	if(nick === "bloxboot"){
		setTimeout(function(){
			cli.say("#bloxbottest", "!quit");
		}, 1000);
	}
});

cli.on("quit", function(nick, reason, channels, message){
	if(nick === "bloxboot"){
		cli.disconnect("Test over");

		bloxbootQuit = true;
		
		setTimeout(function(){
			process.exit(0);
		}, 5000);
	}
});

setTimeout(function(){
	cli.say("#bloxbottest", "bloxboot failed to respond in 25 seconds. Forcing end.");

	setTimeout(function(){
		cli.disconnect("Test over");

		setTimeout(function(){
			spawn("kill", ["-9", fs.readFileSync("bloxtest.pid")]);
			process.exit(0);
		}, 2500);
	}, 1000);
}, 25000);
