const fs = require('fs');
const path = require('path');

//import config
const config = require('./config.json');

const { video_path, frames_path, targetRes, audioDelayFrames } = config;

//do the video conversion
const ffmpeg = require('fluent-ffmpeg');
//use .autopad(true, 'black') to preserve aspect
ffmpeg(video_path).size(targetRes.join("x")).fps(30).output(frames_path + '/%03d.png').run();

//figure out all the frames we need to import and process
let frames = fs.readdirSync(frames_path).filter(file => file.endsWith('.png'));
frames = frames.sort((a, b) => {
  const aNum = parseInt(a.split('.')[0]);
  const bNum = parseInt(b.split('.')[0]);
  return aNum - bNum;
});

//this needs to be a multiple of 8
const frameSize = targetRes[0] * targetRes[1] * 2; //16 bit color
const frameCount = frames.length;

//create a write stream for the output file
const outStream = fs.createWriteStream(path.join(__dirname, 'vid.bindat'));

//write the header

//file type identifier
outStream.write(Buffer.from([0x50, 0x49, 0x4D, 0x41, 0x4E, 0x56, 0x49, 0x44]));

//write the frame count
//Unless some lunatic tries to play an entire movie with this, all we need is 4 bytes
const frameCountBuffer = Buffer.alloc(4);
frameCountBuffer.writeUInt32LE(frameCount); //this is little endian
outStream.write(frameCountBuffer);

const frameSizeBuffer = Buffer.alloc(4);
frameSizeBuffer.writeUInt32LE(frameSize + 1);
outStream.write(frameSizeBuffer);

//read in the interpreted audio
let audio = fs.readFileSync(path.join(__dirname, 'audio.txt'), "utf-8").split("\n").map(Number);
let [bpm, fps, audioFrameCount] = audio.slice(0, 3);
audio = audio.slice(3);

const PNG = require('pngjs').PNG;

function rgbTo16Bit(r, g, b) {
  //we'll use 5 bits to represent each

  //our range goes from 0-255 to 0-31
  let rPacked = Math.floor(r / 8);
  let gPacked = Math.floor(r / 8);
  let bPacked = Math.floor(r / 8);

  let outNum = 0;
  outNum |= rPacked << 11;
  outNum |= gPacked << 6;
  outNum |= bPacked << 1;

  return outNum;
}

async function processFrames() {
  for (let i = 0; i < frameCount; i++) {
    const frame = frames[i];
    const framePath = path.join(frames_path, frame);
    console.log(`Processing frame ${i + 1}/${frameCount} (${frame})`);

    const frameData = await new Promise((resolve, reject) => {
      fs.createReadStream(framePath)
        .pipe(new PNG())
        .on('parsed', function () {
          resolve(this.data);
        });
    });

    const frameBuffer = Buffer.alloc(frameSize);

    //for every pixel
    for (let j = 0; j < frameSize / 2; j++) {
      const r = frameData[j * 4];
      const g = frameData[j * 4 + 1];
      const b = frameData[j * 4 + 2];

      const outNum = rgbTo16Bit(r, g, b);

      frameBuffer.writeUInt16LE(outNum, j * 2);
    }

    outStream.write(frameBuffer);

    //write the audio data
    const audioTick = i - audioDelayFrames;
    if (audioTick > 0 && audioTick < audioFrameCount) {
      const audioBuffer = Buffer.from([audio[audioTick]]); //default to 60 if we run out of audio data
      outStream.write(audioBuffer);
    } else {
      outStream.write(Buffer.from([60]));
    }
  }

  //close the output file
  outStream.end();

  //log that we're done
  console.log('Done!');
}

processFrames();