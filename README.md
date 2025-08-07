# DaisyX7

An exploration of the synthesis engine of the Yamaha DX7. This project implements an FM drone synth that can run on the Electrosmith Daisy Field hardware. I made this to get to know the DX7 better.

This project was inspired by the [DX7 article series on righto.com](https://www.righto.com/search/label/dx7) where I learned how the "YM21280" DX7 sound generator IC works. Besides the YM21280, also known as "OPS", the DX7 uses a second ASIC called "EGS" which generates pitch and amplitude data for all the 96 oscillators in the OPS. This project performs a high-level simulation of 6 oscillators of the OPS, not of the EGS. Because the EGS is missing, DaisyX7 only creates a drone sound.

The functions of the Daisy Field knobs, buttons and switches are as follows:

- SW1: decrease algorithm, SW2: increase algorithm
- Button A1-A6: select operator OP1-OP6
- Button B8: toggle between ratio and fixed frequency mode for the current operator
- Knob 1: amplitude of current operator
- Knob 2: coarse frequency
- Knob 3: fine frequency
- Knob 7: feedback level
- Knob 8: overall pitch
