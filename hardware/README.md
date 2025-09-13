# Gripper PCB Board
<img src="images\soldering_setup.jpeg" style="width:300px; height:auto;">
Using two motors, resistive position sensors, and ESP32 controller, the board interfaces with a robotic arm to grasp small objects.
The PCB board design is from https://github.com/peng-zhihui/Dummy-Robot/tree/main/1.Hardware/HandModule.

<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
    <li>
      <a href="#Mounting">Mounting PCB</a>
    </li>
    <li>
      <a href="#Areas-of-improvement">Areas of improvement</a>
    </li>
    <li>
      <a href="#Soldering-process">Soldering process</a>
    </li>
    <li>
      <a href="#Troubleshooting-process">Troubleshooting process</a>
    </li>
</details>

## Mounting PCB
Custom casing and finger designs for the gripper can be found in [3D-model folder](..\3D-model). The casing and collar clamp designs
are compatible with a 5mm D-shaft.

## Areas of improvement
- Add a LED status light to easily determine status of board
    - This reduces the need to probe different points of the PCB to measure voltage and CAN
- Switch from USB-C to USB-A for easier soldering since there are much less pins
- Redesign attachment of motor lead onto board, current design is very weak due to minimal contact surface

## Soldering process
I decided to only order the PCB board with the top layer components soldered and learn soldering by doing the bottom side on my own.
At the time, this seemed like a good idea since the bottom layer had much less components.
In hindsight, it was definitely a challenging and unnecessary step even though I did learn a lot.

### Tools I used
- Oscilloscope/Multimeter: measure resistance and voltage drop
- Soldering iron: solder motor leads
- Soldering paste & Heat gun (with variable fan speed): solder SMT
- Microscope: examine if enough paste was applied to the pads before putting on components and the soldering results afterwards
- Tweezers: apply paste and adjust placement of SMTs

### General tips
- Put the soldering paste in the fridge
    - This made the paste smoother and easier to apply to the pads
- Put fan speed on lowest setting so that it doesn't shift the placement of the SMT
- Solder components close to each other at the same time since when you use the heat gun,
even the smallest tip causes the surrounding solder and SMT to shift
- USB-C: Remove the two resistors on the top layer that are directly above the USB-C port in order to heat the PCB from the top side and solder the USB-C pins
- Motor: Layer solder on the bottom layer, hold the motor in the correct position, then melt the solder from the top layer so that the leads sink into a heap of solder

## Troubleshooting process
Troubleshooting was a very critical but time consuming process. My goal was to use a 20V power supply,
but I first checked with 5V so that even if things short circuited the whole PCB wouldn't be ruined.

### Top layer
<img src="images\top_layer.png" style="width:300px; height:auto;">

### Bottom layer
<img src="images\bottom_layer.png" style="width:300px; height:auto;">
