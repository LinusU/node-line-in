var Readable = require('stream').Readable
var NativeLineIn = require('./build/Release/line-in.node').LineIn

function LineIn () {
  Readable.call(this)

  var native = new NativeLineIn()

  this._read = native._read.bind(native)
  native.push = this.push.bind(this)
}

LineIn.prototype = Object.create(Readable.prototype)

module.exports = LineIn
