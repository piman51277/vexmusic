const fs = require('fs');
const path = require('path');

//import config
const config = require('./config.json');

const { video_path, frames_path, targetRes, audioDelayFrames } = config;

//do the video conversion
const ffmpeg = require('fluent-ffmpeg');
ffmpeg(video_path).size(targetRes.join("x")).autopad(true, 'black').output(frames_path + '/%03d.png').run();

//figure out all the frames we need to import and process
let frames = fs.readdirSync(frames_path).filter(file => file.endsWith('.png'));
frames = frames.sort((a, b) => {
  const aNum = parseInt(a.split('.')[0]);
  const bNum = parseInt(b.split('.')[0]);
  return aNum - bNum;
});

//this needs to be a multiple of 8
const frameSize = Math.floor((targetRes[0] * targetRes[1]) / 8) * 8;
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


//read in the interpreted audio
let audio = fs.readFileSync(path.join(__dirname, 'audio.txt'), "utf-8").split("\n").map(Number);
let [bpm, fps, audioFrameCount] = audio.slice(0, 3);
audio = audio.slice(3);

const PNG = require('pngjs').PNG;

function compressFrame(frameData) {

  //inline function to check if a pixel is black
  const isBlack = (ind) => frameData[ind * 4] < 50;

  let firstBlack = isBlack(0);
  let pixelCount = targetRes[0] * targetRes[1];

  let isCurrentBlack = firstBlack;
  let currentCount = 0;
  let runs = [];

  for (let i = 1; i < pixelCount; i++) {
    const black = isBlack(i);
    if (black === isCurrentBlack) {
      currentCount++;
    } else {

      //is runs over the 16 bit max?
      if (currentCount > 65535) {
        //split it into two
        runs.push(65535);
        runs.push(0);
        currentCount -= 65535;
      }

      runs.push(currentCount);
      currentCount = 1;
      isCurrentBlack = black;
    }
  }
  if (currentCount > 0) {
    //is runs over the 16 bit max?
    if (currentCount > 65535) {
      //split it into two
      runs.push(65535);
      runs.push(0);
      currentCount -= 65535;
    }
    runs.push(currentCount);
  }

  let buf = Buffer.alloc(runs.length * 2 + 3);

  //uint16 for number of runs
  buf.writeUInt16LE(runs.length, 0);

  //uint16 for first pixel color
  buf.writeUInt8(firstBlack ? 1 : 0, 2);

  //uint16 for each run
  for (let i = 0; i < runs.length; i++) {
    buf.writeUInt16LE(runs[i], i * 2 + 3);
  }

  return buf;
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

    const frameBuffer = compressFrame(frameData);
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