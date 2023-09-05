# OHRBETS (Open-source Head-fixed Rodent Behavior Experimental Training System)
<p align="center">
  <img src="./images/system_render.png" width="800">
</p>

Accompany to [Gordon-Fennell et. al. 2023 *eLife*](https://elifesciences.org/articles/86183)

## Overview
Here we make available an ecosystem of open-source hardware and software for operant and consumption experiments in head-fixed mice. For operant responding, mice rotate a wheel in one of two directions and operant responding is gated using a mechanical wheel break. For consumption, mice consume liquid solutions from a metal lick spout and access to the spout is gated using retraction and extension of the spout. The system also includes optional multi-spout hardware that allows fixed access to 1 of 5 spouts. Using the hardware, behavioral software, and analysis software included in this repository, the Gordon-Fennell et. al. 2022 reproduced operant and consummatory behaviors established in feely-moving mice. The system presented here offers a scalable option for these behavioral approaches and is compatible with head-fixed imaging techniques including 2-photon imaging.  

The hardware consists of 3d printed and other low-cost components including micro servos that can be easily assembled with our detailed step-by-step guide. The system utilizes an Arduino Mega to control hardware including servos, solenoids, and tone generators, and to record behavioral events including licks and wheel rotation. Session parameters and behavioral events are recorded using a PC connected to the Arduino via USB. 

The Arduino software includes programs written in the Arduino language. We have included multiple scripts for different behavioral approaches including operant conditioning and multi-spout brief access. We have also included scrips used to setup for behavior and trouble shoot the system.  

The Analysis software includes programs written in R and relies on user created spreadsheets. We have included a universal decoder that parses the data produced by the Arduino. We have also included scripts for analyzing data produced by the generic operant conditioning and multi-spout brief access task Arduino programs.

We will continue to update this repository and add additional hardware and software. We welcome contributions that add on to our system for different behavioral designs.  
## DOI: 
- 10.5281/zenodo.7566343

## Associated Pre-Print
- [bioRxiv](https://www.biorxiv.org/content/10.1101/2023.01.13.523828v1)

## Up to Date Resources
- [Purchase List](https://docs.google.com/spreadsheets/d/1kFgsOy1gAFHM2E8M-DSgPSi6Y7Lr8sis6cbQqSb2ZY4)
- [3d Printing File Key](https://docs.google.com/spreadsheets/d/1pzRUh2JkpAaJyb1kA9NBVjSYz8nG3GPTu3c0QVQwKCg)


## Tinkercad Models (#OHRBETS)
- [Full Respository](https://www.tinkercad.com/users/elZH8v8MJf0-adam-gordon-fennell)
- [OHRBETS_assembly](https://www.tinkercad.com/things/58vTdDu0pfz)
- [OHRBETS_components_filament](https://www.tinkercad.com/things/1Yv3k1L5hwq)
- [OHRBETS_components_resin](https://www.tinkercad.com/things/6Ztq3dLU4Uj)
- [OHRBETS_components_multispout](https://www.tinkercad.com/things/cckeigB3ydE)
- [OHRBETS_arduino_enclosure](https://www.tinkercad.com/things/ki4dcN9vabO)
- [OHRBETS_mount_syringe](https://www.tinkercad.com/things/ft7A0Lh4chg)
- [OHRBETS_mount_solenoid](https://www.tinkercad.com/things/eRxwZDRHmDL)

## Contributors
Adam Gordon-Fennell (University of Washington), Garret Stuber (University of Washington)
