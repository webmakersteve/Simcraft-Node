var simc = require('../build/Release/simcraft.node');

simc.run({
  armory: 'us,illidan,john'
}, function() {
  console.log('hey');
});

console.log("Done");
