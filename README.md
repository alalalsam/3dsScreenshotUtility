<<<<<<< HEAD
3dsScreenshotUtility
This program is basically just normal Luma3DS, except it takes a screenshot every couple seconds, for the purpose of building a large ml dataset. Credit for 99% of this work goes to the creators of Luma3ds, since I really only added like 2 files and 1 line of extra code in existing files.

based off of Luma3ds 13.1.1
idk if it even works yet maybe I'm getting ahead of myself editing the readme.

to use, download firmtool and the other program, add to path, and make using devkitPro.
=======
# Luma3DS

*Nintendo 3DS "Custom Firmware"*

## What it is
**Luma3DS** is a program patching and reimplementing significant parts of the software running on all models of the Nintendo 3DS family of consoles.

It aims at greatly improving the user experience and at supporting the 3DS far beyond its end-of-life. Features include:

* First class support of 3DSX homebrew
* An overlay menu called "Rosalina" (triggerable by <kbd>L+Down+Select</kbd> by default), allowing amongst many thing to take screenshots while in-game
* Removal of restrictions such as the region lock
* Per-game language settings, asset content path redirection (LayeredFS), game plugins...
* A fully-fledged GDB stub allowing to debug software (homebrew and system software alike)
* ... and much more!

Luma3DS requires a full-system persisent exploit such as [boot9strap](https://github.com/SciresM/boot9strap) to run.

## Compiling

To build Luma3DS, the following is needed:
* git
* up-to-date devkitARM and libctru
* [makerom](https://github.com/jakcron/Project_CTR) in PATH
* [firmtool](https://github.com/TuxSH/firmtool) installed

The produced `boot.firm` is meant to be copied to the root of your SD card for usage with Boot9Strap.

## Setup / Usage / Features
See https://github.com/LumaTeam/Luma3DS/wiki (needs rework)

## Credits
See https://github.com/LumaTeam/Luma3DS/wiki/Credits (needs rework)

## Licensing
This software is licensed under the terms of the GPLv3. You can find a copy of the license in the LICENSE.txt file.

Files in the GDB stub are instead triple-licensed as MIT or "GPLv2 or any later version", in which case it's specified in the file header.

By contributing to this repository, you agree to license your changes to the project's owners.
>>>>>>> cb0edd9 (updated chunkeyMunkey thread, still need to double check it works properly)
