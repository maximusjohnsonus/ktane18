# KTANE alpha

Requirements:
- [Newliquidcrystal_1.3.5](https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads)
- Adafruit LED Backpack Library
- Adafruit GFX Library


## Example upload from terminal

`arduino --upload --board arduino:avr:mega --port /dev/ttyACM0 head_node/head_node.ino`

`arduino --upload --board arduino:avr:uno --port /dev/ttyACM1 module_template/module_template.ino`

or try the new Makefile (run `make` for usage)

## To create a new module

```c++
cd src
cp -r test_module <name>_module
cd <name>_module
mv test_module.ino <name>_module.ino
```

Then, edit `implementation.h` in `src/<name>_module/`. Do not edit the `.ino` file;
it its symlinked to another file.
