# Anne Pro 2 Shine!

This is the custom LED firmware that goes along with the custom
QMK firmware for Anne Pro 2. 

This firmware is still in early development. And will probably,
but it provides the flexibility for creating custom effects.

# Build and Flash

**Warning: This will not work with the Obins Stock firmware**

To build both C15 and C18 versions run

`make`

to build just C15 run

`make C15`

to build just C18 run

`make C18`

The resulting `.bin` files will be in the `build` directory
named `annepro2-shine-C15.bin` and `annepro2-shine-C18.bin`
respectively


# Debugging

You can debug the chip using jlink debugger or, in a limited way using a Black
Magic Probe (BMP) debugger - which can be flashed onto a cheap and available STM
evaluation boards.

LED chip is to the left from the spacebar switch and has 5 pads to solder the
SWD port, from the left: Unused?, reset, ground, clk, io.

BMP can stop the program and investigate the current variables, but can't set
breakpoints or step.

# Contribute

Thanks to @Stanley00 on the Anne Pro Dev discord for implementing
the key mapping for the C18 revision, and @Rusty for mapping out
the physical connection.
