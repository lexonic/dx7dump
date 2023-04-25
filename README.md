# Yamaha DX7 Sysex Dump Analyzer

**A command-line-tool to display Yamaha DX7 sysex files as human readable text.**

This is a fork of [Ted Felix](http://tedfelix.com/yamaha-dx7/index.html) [dx7dump](https://sourceforge.net/u/tedfelix/dx7dump/ci/master/tree/) 

Based on info from
[sysex-format.txt](https://web.archive.org/web/20170707125228/https://homepages.abdn.ac.uk/mth192/pages/dx7/sysex-format.txt). Archived from the [original](http://homepages.abdn.ac.uk/mth192/pages/dx7/sysex-format.txt) on July 7, 2017.


## Features

* Voice name listings of banks in long and compact form
* Voice parameter listings in long and compact format
* Shows algorithms as ASCII-art
* Report errors of sysex files
* Can fix sysex checksum errors and convert headerless files to regular DX7 sysex files
* Show voice names in Unicode or plain ASCII
* Scan folders recursively and list all voice names or voice parameters


## Build and install
	
Linux: run `make` as regular user:

	make

this will install two programs in the user's `~/.local/bin` directory:

* dx7dump: a utility to print sound parameter of all sounds in a bank. 
* dx7dumpall: a dx7dump wrapper to print list of all sound banks recursively in the given search directory.


## Usage of dx7dump

```
Usage: dx7dump [OPTIONS] FILE

Options:
  -l, --long          long listing (show parameter values)
  -c, --compact       compact listing (can also be combined with -l)
  -p NUM, --patch NUM display patch number NUM
  --fix               try to fix corrupt files
                        creates a backup of the original file (*.ORIG)
  --no-backup         don't create backups when fixing files
                        WARNING: This option might result in data-loss!
                        make sure you already have a backup of the sysex-file
  -y, --yes           no questions asked. Answer everything with YES for '--fix'
  -e, --errors        print only files with errors
  -x, --hex           show voice names also as HEX and print single voice data in HEX
  -a, --ascii         use ASCII characters to show voice-names and algorithms (default = unicode)
  -v, --version       version info
  -h, --help          this help
```

## Usage of the dx7dump wrapper

```
dx7dump wrapper: Print lists of all sound banks recursively.

Usage: dx7dumpall [OPTIONS for dx7dump] [PATH]
    An argument WITHOUT a leading '-' or '--' is interpreted as the path where the recursive search will start.
    All other arguments are passed to the dx7dump utility.
```


## Examples

List sound names of rom1a.syx in compact form:

```
$ dx7dump -c rom1a.syx 
File: "rom1a.syx"
 1 |BRASS   1 |           9 |PIANO   2 |          17 |E.ORGAN 1 |          25 |ORCH-CHIME| 
 2 |BRASS   2 |          10 |PIANO   3 |          18 |PIPES   1 |          26 |TUB BELLS | 
 3 |BRASS   3 |          11 |E.PIANO 1 |          19 |HARPSICH 1|          27 |STEEL DRUM| 
 4 |STRINGS 1 |          12 |GUITAR  1 |          20 |CLAV    1 |          28 |TIMPANI   | 
 5 |STRINGS 2 |          13 |GUITAR  2 |          21 |VIBE    1 |          29 |REFS WHISL| 
 6 |STRINGS 3 |          14 |SYN-LEAD 1|          22 |MARIMBA   |          30 |VOICE   1 | 
 7 |ORCHESTRA |          15 |BASS    1 |          23 |KOTO      |          31 |TRAIN     | 
 8 |PIANO   1 |          16 |BASS    2 |          24 |FLUTE   1 |          32 |TAKE OFF  | 
```

Complete sound parameter list of patch number 12 of rom1a.syx in compact form:

```
$ dx7dump -cp 12 rom1a.syx 
File: "rom1a.syx"
Voice-#: 12
Name: "GUITAR  1 "

Algorithm: 8

           [6]
     ┌──┐   │
[2]  │ [4] [5]
 │   └──┤ ╱
[1]    [3]
 └──────┘

Feedback: 7
LFO
  Wave: Sine
  Speed: 35
  Delay: 0
  Pitch Mod Depth: 1
  Amplitude Mod Depth: 3
  Key Sync: Off
  Pitch Mod Sensitivity: 3
Oscillator Key Sync: Off
Pitch Envelope Generator
  Rate 1: 75    Level 1: 50
  Rate 2: 80    Level 2: 50
  Rate 3: 75    Level 3: 50
  Rate 4: 60    Level 4: 50
Transpose: 0

                       | Operator 1  | Operator 2  | Operator 3  | Operator 4  | Operator 5  | Operator 6  |
-----------------------+-------------+-------------+-------------+-------------+-------------+-------------+
Amplitude Mod Sens     |           0 |           0 |           0 |           0 |           0 |           3 |
Oscillator Mode        | Freq. Ratio | Freq. Ratio | Freq. Ratio | Freq. Ratio | Freq. Ratio | Freq. Ratio |
Frequency              |           1 |           3 |           1 |           3 |           3 |          12 |
Detune                 |          +0 |          +0 |          +0 |          +0 |          +0 |          +0 |
-----------------------+-------------+-------------+-------------+-------------+-------------+-------------+
Envelope Generator     |             |             |             |             |             |             |
  Rate 1 : Level 1     |   74 : 99   |   91 : 99   |   78 : 99   |   81 : 99   |   81 : 99   |   99 : 99   |
  Rate 2 : Level 2     |   85 : 95   |   25 : 86   |   87 : 92   |   87 : 92   |   87 : 92   |   57 : 0    |
  Rate 3 : Level 3     |   27 : 0    |   39 : 0    |   22 : 0    |   22 : 0    |   22 : 0    |   99 : 0    |
  Rate 4 : Level 4     |   70 : 0    |   60 : 0    |   75 : 0    |   75 : 0    |   75 : 0    |   75 : 0    |
-----------------------+-------------+-------------+-------------+-------------+-------------+-------------+
Keyboard Level Scaling |             |             |             |             |             |             |
  Breakpoint           |         A-1 |         A-1 |          G2 |         A-1 |         A-1 |          C3 |
  Left Curve           |        -LIN |        -LIN |        -LIN |        -LIN |        -LIN |        -LIN |
  Right Curve          |        -LIN |        -LIN |        -LIN |        -LIN |        -LIN |        -LIN |
  Left Depth           |           0 |           0 |           9 |           0 |           0 |          53 |
  Right Depth          |           0 |          65 |           0 |          14 |          15 |          20 |
-----------------------+-------------+-------------+-------------+-------------+-------------+-------------+
Keyboard Rate Scaling  |           4 |           2 |           3 |           4 |           4 |           0 |
Output Level           |          99 |          93 |          99 |          89 |          99 |          57 |
Key Velocity Sens      |           5 |           7 |           7 |           4 |           7 |           6 |
-----------------------+-------------+-------------+-------------+-------------+-------------+-------------+
```

List only files with sysex errors and single voice files:

```
$ dx7dumpall -e
File: "Ajay/dx7ii-bank-B.syx"
File too small (4103 Bytes)

File: "Single Voice/001.syx"
File is a Single Voice Dump: "GET FUNKY "

File: "unknown-data/BELLTEL.SYX"
WARNING: file seems to be a headerless dump (4096 Bytes)

File: "unknown-data/CLAUDE_3.SYX"
CHECKSUM FAILED: Should have been 0x39
```


