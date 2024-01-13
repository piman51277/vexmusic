const fs = require('fs');
let input = fs.readFileSync('input.txt').toString().split('\n').filter(Boolean);

//read params
const bpm = parseInt(input[0].replace('BPM=', ''));
const beatTimeMS = 60000 / bpm;
const framerate = parseInt(input[1].replace('FRAMERATE=', ''));
const frameTimeMS = 1000 / framerate;
input = input.slice(2);

//remove lines that start with //
input = input.filter(line => !line.startsWith('//'));

//read blocks
const blocks = {};
let currentBlockStart = -1;
let currentBlockName = '';
let inBlock = false;
for (let i = 0; i < input.length; i++) {
  const line = input[i];
  if (line.startsWith('#start')) {

    //blocks can't be nested
    if (inBlock) {
      //we have to +2 because we removed the first two lines
      throw new Error(`Block \"${currentBlockName}\" is not closed`)
    }

    const name = line.replace('#start ', '');


    //names must be unique
    if (blocks[name]) {
      throw new Error(`Block \"${name}\" already exists`)
    }

    currentBlockStart = i;
    currentBlockName = name;
    inBlock = true;
  }

  else if (line.startsWith('#end')) {
    if (!inBlock) {
      throw new Error(` Block was never opened`)
    }

    blocks[currentBlockName] = {
      start: currentBlockStart,
      end: i,
      lines: input.slice(currentBlockStart + 1, i)
    };
    currentBlockStart = -1;
    currentBlockName = '';
    inBlock = false;
  }
}

//check if all blocks are closed
if (inBlock) {
  throw new Error(`Block \"${currentBlockName}\" is not closed`)
}

//mark all lines with blocks for deletion
//FIXME: this is a hacky way to do it. Praise the JS overlords!
for (const blockName in blocks) {
  const block = blocks[blockName];
  const { start, end } = block;
  for (let i = start; i <= end; i++) {
    input[i] = '';
  }
}
input = input.filter(Boolean);

const validNotes = [`C`, `C#`, `Db`, `D`, `D#`, `Eb`, `E`, `F`, `F#`, `Gb`, `G`, `G#`, `Ab`, `A`, `A#`, `Bb`, `B`, `R`];
const noteMappings = {
  'C': 0,
  'C#': 1,
  'Db': 1,
  'D': 2,
  'D#': 3,
  'Eb': 3,
  'E': 4,
  'F': 5,
  'F#': 6,
  'Gb': 6,
  'G': 7,
  'G#': 8,
  'Ab': 8,
  'A': 9,
  'A#': 10,
  'Bb': 10,
  'B': 11,
  'R': 60
}

//Note lengths are relative to the beat
const noteLengths = {
  "w": 4,
  "h": 2,
  "q": 1,
  "e": 0.5,
  "s": 0.25,
  "te": 0.3333,
}

//parse notes
const noteStrings = [];

function resolveBlock(blockName, depth = 0) {
  if (depth > 20) {
    throw new Error(`Possible infinite loop detected. Check your blocks`)
  }

  if (!blocks[blockName]) throw new Error(`Block \"${blockName}\" does not exist`);

  const contents = [];
  for (const line of blocks[blockName].lines) {
    if (line.startsWith('>')) {
      const name = line.replace('>', '').trim();
      contents.push(...resolveBlock(name, depth + 1));
    } else {
      contents.push(line);
    }
  }

  return contents;
}

for (const line of input) {
  if (line.startsWith('>')) {
    const name = line.replace('>', '').trim();
    noteStrings.push(...resolveBlock(name));
  } else {
    noteStrings.push(line);
  }
}

function resolveNote(noteString) {
  try {
    const lengthStr = noteString.split(' ')[1];
    const dotted = lengthStr.slice(-1) === '.';
    let len = 1;
    if (dotted) {
      len = noteLengths[lengthStr.slice(0, -1)] * 1.5;
    } else {
      len = noteLengths[lengthStr];
    }

    const note = noteString.split(' ')[0];

    if (note == 'R') return ({
      note: noteMappings[note],
      len
    });

    const notename = note.slice(0, -1);
    if (!validNotes.includes(notename)) throw new Error(`Invalid note: ${noteString}`);
    const octave = parseInt(note.slice(-1));

    if (isNaN(octave)) throw new Error(`Invalid octave: ${noteString}`);
    if (octave < 1 || octave > 5) throw new Error(`Octave not in range: ${noteString}`);

    return {
      note: noteMappings[notename] + (octave - 1) * 12,
      len
    }
  }
  catch (e) {
    throw new Error(`Invalid note: ${noteString}`)
  }
}

const notes = noteStrings.map(resolveNote);

//calculate the time of each note
const timeline = [];
let time = 0;
for (const note of notes) {
  timeline.push({
    value: note.note,
    time: time + beatTimeMS * note.len
  });
  time += beatTimeMS * note.len;
}

//resolve the note every frame
const frames = [];
let noteptr = 0;
let note = timeline[noteptr];
let inNote = false;
let currentTime = 0;
let endTime = timeline[timeline.length - 1].time;

while (currentTime < endTime) {
  if (currentTime < note.time) {
    if (!inNote) {
      frames.push(note.value + 64); //add a bit at the start to indicate the start
      inNote = true;
    }
    else {
      frames.push(note.value);
    }
    currentTime += frameTimeMS;
  }
  else {
    noteptr++;
    note = timeline[noteptr];
    inNote = false;
  }
}

//export
const frameCount = frames.length;
const outStr = `${bpm}\n${framerate}\n${frameCount}\n${frames.join('\n')}`;
fs.writeFileSync('output.txt', outStr);