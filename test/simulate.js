var simc = require('../build/Release/simcraft.node');

simc.run({
  armory: 'us,maelstrom,chaosity'
}, function() {
  console.log('hey');
});

console.log("Done");
