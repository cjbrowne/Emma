# Emma the emulator.

Emma is a brief, educational introduction to emulating CPU architecture made by amateur programmer Chris Browne.

Its main function is to serve as an introduction to emulation for the author, and anybody interested enough to read the code.

## Building:
```bash
$ make
```

### Building debug mode:
```bash
$ make debug
```

## Running:
```bash
$ ./emma [input file]
```

## Features
 - Generic, simplified architecture
 - 16-bit Accumulator
 - Two 16-bit registers, named reg_b and reg_c
 - Program Counter (register)
 - 16-bit FLAGS register (Zero, Carry and Error currently supported)
 - Error Number for diagnostic purposes
 - Dynamically-allocated stack and heap, default size gives roughly 1KB of total memory at startup.  There is currently no way to programmatically increase the stack/heap sizes, though a debug interrupt may be added in the future for this purpose.
 - Interrupts (non-programmable, in the future there may be programmable interrupts)
 - Output console (port 0xFFFF) that dumps hex-formatted data to stdout.  In the future, there may be more output consoles that do more interesting things added.

### License
This source is licensed under the GPLv3 license unless stated otherwise.

If you haven't got a copy of the GPLv3 license, google GPLv3 because I can't be bothered to spoon-feed you a URL or a file containing the license.  I don't really care what you do with this source, either, to be honest.
