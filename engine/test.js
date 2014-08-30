var simc = require('./build/Release/simc');

// print process.argv
var region, realm, name;
process.argv.forEach(function (val, index, array) {
  if (index == 0 || index == 1) return;
  switch (index) {
    case 2:
      region = val;
      break;
    case 3:
      realm = val;
      break;
    case 4:
      name = val;
      break;
  }
});

if (!region || !realm || !name) {
  process.exit();
}

console.log("here");

simc.run({
  "armory": region + "," + realm + "," + name
}, function (dps) {
  console.log(dps);
});
