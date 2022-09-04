# Purpose

`!Nonogram` is a Nonogram solver for the Acorn RISC OS desktop.

Nonograms are logic puzzles originating in Japan.
A picture is encoded into numbers, and the challenge is to discover the picture using only the numbers.
Alternatively, if you're lazy, or a cheat, or simply stuck on a difficult puzzle at which you've already had a fair stab (honest, I have, Guv!), get this program to solve it for you.

For more information on Nonograms, please visit [Nonogram Solver](https://www.lancaster.ac.uk/~simpsons/nonogram/).

# Features

- Multitasking operation; can solve several puzzles at once.
  Granularity of processing is now finer than a single line, and so   responsiveness shouldn't be so bad while solving a difficult line.

- Save solutions as text or Draw files (the latter now includes puzzle numbers).

- Choice of fast, complete or hybrid algorithms.

# Requirements

This application runs fine on a RISC OS 3.11 machine.
It almost certainly won't work on RISC OS 2, as it uses Wimp SWI calls which aren't in the PRM for that version.
I've no idea if it works on anything else.
It initially requires 160K (that's what I've got the WimpSlot set to), but may take more memory as it solves more puzzles simultaneously.
