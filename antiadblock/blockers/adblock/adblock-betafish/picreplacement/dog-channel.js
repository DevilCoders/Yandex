'use strict';

/* For ESLint: List any global identifiers used in this file below */
/* global Channel, Listing, require */

const dogData = require('./data/dogs.json');

// Channel containing hard coded dogs loaded from CDN
// Subclass of Channel.
function DogsChannel() {
  Channel.call(this);
}
DogsChannel.prototype = {
  __proto__: Channel.prototype,

  getLatestListings(callback) {
    const listingArray = [];
    for (const dog of dogData) {
      listingArray.push(this.listingFactory(dog.width, dog.height, dog.url, 'This is a dog!', 'dogchannelswitchlabel'));
    }
    callback(listingArray);
  },
};

Object.assign(window, {
  DogsChannel,
});
