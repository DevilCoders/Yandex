'use strict';

/* For ESLint: List any global identifiers used in this file below */
/* global Channel, Listing, require */

const catData = require('./data/cats.json');

// Channel containing hard coded cats loaded from disk.
// Subclass of Channel.
function CatsChannel() {
  Channel.call(this);
}

CatsChannel.prototype = {
  __proto__: Channel.prototype,

  getLatestListings(callback) {
    const listingArray = [];
    for (const cat of catData) {
      listingArray.push(this.listingFactory(cat.width, cat.height, cat.url, 'This is a cat!', 'catchannelswitchlabel'));
    }
    callback(listingArray);
  },
};

Object.assign(window, {
  CatsChannel,
});
