Poker test program

texas_holdem.cpp
----------------
TEXAS HOLDEM simulation for two players, written in C++17.

The program shuffles a standard 52 card deck and deals two hole cards to each
of the two players. The hole cards can also be entered by hand instead of being
dealt. It then deals the flop (three cards), the turn (fourth card) and the
river (fifth card). After every street it shows the community cards, both
players' hole cards, the best hand each player can make and the winning chances
of both players. At the end the winner, or a tie, is printed. There is no
betting.

Hand categories, from weakest to strongest:
High Card, One Pair, Two Pair, Three of a Kind, Straight, Flush, Full House,
Four of a Kind, Straight Flush, Royal Flush.

The winning chances are exact. For every street each possible way of completing
the board is played out and counted, so the numbers contain no sampling error.
The work is small enough to stay instant: 1712304 boards before the flop, 990
after the flop, 44 after the turn and a single board on the river. The numbers
shown are whole percentages and always add up to 100, so a point lost to
rounding is given to the largest remainder.

Cards are printed as a rank followed by a suit symbol. Ranks use digits and the
face card letters 2 to 10, J, Q, K, A, for example A, 10 or K. Suits are shown
as the spade, heart, diamond and club symbols. There is no Chinese text.

The suit symbols are written as UTF-8. The source file keeps them as escape
sequences, so the file itself is pure ASCII and builds the same with any
compiler. While it runs, the program switches the Windows console to code page
65001 so the symbols are displayed correctly, and restores the original code
page when it ends. Output redirected to a file is UTF-8 as well.

The program is driven by the keyboard.

Before each hand the hole cards can be chosen by hand or dealt at random:

  1. Type four cards to use them, or press Enter for random cards. The first
     two cards belong to player 1 and the last two to player 2. Cards are
     written as a rank followed by a suit: AS, KH, 10D or TD, 2c. The ranks are
     2 to 10, J, Q, K, A (10 may be written as 10 or T) and the suits are S for
     spades, H for hearts, D for diamonds and C for clubs. Spaces, commas and
     slashes separate the cards, so "AS KH 10D 2C", "as,kh,td,2c" and
     "AS/KH/10D/2C" all mean the same four cards.

     Input is checked before the hand starts. A repeated card, a card that
     cannot be read, or the wrong number of cards prints a message and asks
     again, and the cards that were chosen by hand are taken out of the deck so
     the board can never repeat them. An empty line deals random cards.

  2. Press Enter to see the flop (first three community cards).
  3. Press Enter to see the turn (fourth community card).
  4. Press Enter to see the river (fifth community card).

At the end of the hand:

  5. Press Enter for a new hand or type q to exit. Typing q at the hole card
     prompt also exits.

After each step the community cards, both hands, both best hands and the winning
chances are printed again. When the hand is over the program prints RESULT with
the winner or a tie, then END OF HAND, and keeps everything on screen.

If the input is not interactive (for example when the output is piped) the end
of input works like pressing Enter and the simulation runs to the end.

Build with the script (Windows):
  build.cmd          optimized build, writes texas_holdem.exe
  build.cmd debug    build with debug info, writes build\Debug\outDebug.exe
  build.cmd run      optimized build and then runs the program
  build.cmd clean    removes the build outputs

The script takes g++ or clang++ from PATH first and then from the usual
MinGW-w64 and LLVM folders, so it works from cmd, from PowerShell and from a
double click. A build leaves the working directory clean, and every mode reports
the file it wrote and returns a non zero exit code when the build fails. The
debug output path is the one .vscode\launch.json expects, so build.cmd debug
followed by the debugger works without further setup.

Build by hand with MinGW g++:
  g++ -std=c++17 -Wall -Wextra -O2 -o texas_holdem.exe texas_holdem.cpp

Build by hand with MSVC:
  cl /std:c++17 /W4 /EHsc /Fe:texas_holdem.exe texas_holdem.cpp

Run:
  texas_holdem.exe
