var simc = require('./build/Release/simc');

simc.run({
  "armory": "us,maelstrom,breyada",
  "iterations": 1000
}, function (err,data) {
  if (err) {
    console.log("Error: " + err);
    return;
  }
  console.log("You did " + data.dps + " dps!");
});

console.log(simc.version());
