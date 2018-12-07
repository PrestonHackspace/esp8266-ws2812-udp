const fs = require('fs');
const path = require('path');
const request = require('request');

const url = 'http://esp-7bea1e.local:8080/update'
const firmwarePath = '.pioenvs/esp12e/firmware.bin';

const formData = {
  upload: fs.createReadStream(path.join(__dirname, firmwarePath)),
};

const req = request.post({ url, formData })

req.on('complete', (data) => {
  console.log('Status:', data.statusCode);
});

req.on('error', (err) => {
  console.error('Error:', err);
});
