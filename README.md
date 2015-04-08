Simcraft-Node
=============

Simcraft Node bindings

A port of the simcraft engine to a JavaScript API.

Low level build creates an object that takes simcraft command line arguments and returns a JSON object of relevant data. Higher level JavaScript code takes in pretty parameters and interprets them into command line arguments and passes them to the C addon.

for example:

```javascript
var simc = require('simc');
simc.run({
  'armory': 'us,illidan,john'
}, function (progress) {

}, function (err, simt) {
  if (err)
    return;

  console.log(simt); // This is an array of raw output data

});
```

That's how the low level API is done. The higher level API is done to implement things like promises and make it easier to use. simc lib is used only by the internal javascripts. Or intended, at least.

```javascript

var simcraft = require('simcraft');
simcraft.on('progress', function(progress) {
  // Do things with stuff like progress bars.
});

simcraft.simulateCharacter({
  'name': 'john',
  'realm': 'us',
  'region': 'illidan'
});

simcraft.simulateRaw({
  'class': 'warrior',
  'equipment': {
    'head': {
      'reforge': "mastery to haste",
      'id': 11111,
      'gems': ['blue', 'red']
    }
  },
  'spec': 'fury',
  'talents': [0,2,3,1,2],
  'glyphs': {
    'minor': [11123,23213,11212],
    'major': [12313,12321,21312]
  },
  'race': 'human'
}).then(function (data) {
  console.log("You did " + data.dps +". Not bad!");
}).catch(function (err) {
  console.log("Error: " + err );
});

```
