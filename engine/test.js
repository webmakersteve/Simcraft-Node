var simc = require('./build/Release/simc');

simc.run({
  "armory": "us,maelstrom,breyada",
  "iterations": 2
}, function (err,data) {
  if (err) {
    console.log("Error: " + err);
    return;
  }
  console.log("You did " + data.dps + " dps!");
});
