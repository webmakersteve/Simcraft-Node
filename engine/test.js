var simc = require('./build/Release/simc');

console.log(simc);

simc.run({
  "armory": "us,tichondrius,donpain"
}, function (dps) {
  console.log(dps);
});
