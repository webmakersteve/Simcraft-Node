var simc = require('./build/Release/simc');


simc.run({
  "armory": "us,malstrom,breyada",
  "iterations": 10000
}, function (err,data) {
  if (err) {
    console.log("Error: " + err);
    return;
  }
  console.log("You did " + data.DPS + "dps!");
});
