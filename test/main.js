var irc = require("irc");

var testName = "bloxtest";

var cli = new irc.Client("irc.openblox.org", testName, {
	userName: testName,
	realName: testName,
	port: 6697,
	autoRejoin: false,
	autoConnect: true,
	channels: ["#bloxbottest"],
	secure: true
});

cli.on("motd", function(){
	cli.say("NickServ", "IDENTIFY bloxboot " + process.env["BB_PASSWD"]);
});

cli.on("join#bloxbottest", function(nick){
	if(nick === "bloxboot"){
		//TODO: Real testing.
		cli.say("#bloxbottest", "!quit");
		process.exit(0);
	}
});
