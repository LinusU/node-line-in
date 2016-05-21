# Line In

Capture audio with a simple stream interface.

## Installation

```sh
npm install --save line-in
```

## Usage

```js
const LineIn = require('line-in')
const Speaker = require('speaker')

const input = new LineIn()
const output = new Speaker({ signed: true })

input.pipe(output)
```

## API

### `new LineIn()`

Returns a `Readable` stream that emits chunks of raw audio data.

Currently only 2-channel 16-bit little-endian signed integer pcm encoded data is supported.
