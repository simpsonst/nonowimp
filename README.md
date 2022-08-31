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

# Use

Run the application `!Nonogram`.
The icon appears on the icon bar.
Drag a Nonogram puzzle text file to the icon, and a window will open showing an empty grid.
Open the window's menu, and choose `Action` &rArr; `Run`, and the puzzle will begin being solved.
When grid is complete, open the menu and choose `Save` &rArr; `Text` &rArr; &hellip; or `Save` &rArr; `Draw` &rArr; &hellip; to save the solution as a text or Draw file respectively.

When saving as a Draw file, you may indicate:

- whether you want to include the cell values or just the empty grid,

- whether you want the row rules/clues printed on the left/right/both,

- whether you want the column rules/clues printed on the top/bottom/both.

## Nonogram puzzle format

The Nonogram Solver website gives details of the file format that the program accepts.
Briefly, it looks like this:

    width <width>
    height <height>

    rows
    <row data>

    columns
    <column data>

Each line of row or column data in the text file corresponds to a row or column in the puzzle, and numbers for the same line are comma-separated.

## Other facilities

From the window's menu, `File` &rArr; &hellip; brings up information on the Nonogram:
number of solutions found, and processing status (`Paused` (the initial status), `Running` or `Finished`).

`Algorithm` &rArr; &hellip; selects one of the three line-solving algorithms to use:

- `Algorithm` &rArr; `Fast` selects a quick and simple algorithm that isn't guaranteed to extract all the information present in a line.
  The solver may be forced to make orthogonal-line guesses that wouldn't be necessary using other algorithms.

- `Algorithm` &rArr; `Complete` selects a slower, more thorough algorithm that is guaranteed to extract all the information present in a line.

- `Algorithm` &rArr; `Hybrid` selects either of the fast or complete algorithms per line.
  The fast algorithm is prefered, but the complete algorithm will be used if necessary, rather than making an orthogonal-line guess.

- `Action` &rArr; `Exhaust` starts or stops the processing of the puzzle (a tick is displayed when running).
  Use this to count the total number of solutions.

`Action` &rArr; `Run` starts or stops the processing of the puzzle.
(A tick is displayed when running.)
Processing will stop automatically when a puzzle is found, and you can set it to continue to find more.

`Action` &rArr; `Once` causes a single line to be solved, and the puzzle will then be paused.

`Action` &rArr; `Clear` clears the grid so you can begin solving the puzzle again.

`Action` &rArr; `Discard` removes the puzzle from the window.

It is possible to load a puzzle over another one by dropping the puzzle file in an existing window.
The old puzzle is discarded.

Interactive help is provided, and can be viewed with a suitable application such as `!Help`.
