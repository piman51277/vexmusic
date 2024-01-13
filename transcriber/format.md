# Transcriber

This is used to make it easier to transcribe music to the format used by the rest of this project.

Since this was written as a quick tool, it will always read from `input.txt` and write to `output.txt`.

## General

Lines starting with a `//` are comments and will be ignored.

## Header

Defines constants. This must appear at the very top of the file.

```
BPM=XXX
FRAMERATE=XX
```

## Block Notation

Defines blocks for easy repeats. Recursive blocks are allowed, but nested blocks are not.

> The order in which blocks do not matter.

#### Defining

```
#start blockname
#end
```

#### Using

```
>blockname
```

## Notes

Notes are defined by their letter and octave, and can be modified by accidentals.
Rests are defined by `R`

```
C2
C#2
Db2
```

Duration is any of the below, and can be modified by dots.

```
w (whole)
h (half)
q (quarter)
e (eighth)
s (sixteenth)
te (triplet eighth)

W. (dotted whole)
h. (dotted half)
q. (dotted quarter)
e. (dotted eighth)
...
```

Complete lines look like this

```
C2 q
Db2 e.
```
