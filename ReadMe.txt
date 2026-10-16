Poker test program

texas_holdem.cpp
----------------
TEXAS HOLDEM simulation for two players, written in C++17.

The program shuffles a standard 52 card deck and deals two hole cards to each
of the two players. It then deals the flop (three cards), the turn (fourth card)
and the river (fifth card). After every street it shows the community cards,
both players' hole cards, the best hand each player can make and the winning
chances of both players. At the end the winner, or a tie, is printed. There is
no betting.

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

The program is driven by the keyboard. It waits for the player to press Enter
before each step:

  1. Press Enter to deal the two hole cards.
  2. Press Enter to see the flop (first three community cards).
  3. Press Enter to see the turn (fourth community card).
  4. Press Enter to see the river (fifth community card).

At the end of the hand:

  5. Press Enter for a new hand (a fresh shuffle and two new hole cards for each
     player) or type q to exit.

After each step the community cards, both hands, both best hands and the winning
chances are printed again. When the hand is over the program prints RESULT with
the winner or a tie, then END OF HAND, and keeps everything on screen. The end
of a hand asks for an empty line or q: an empty line deals the next hand, q ends
the program, and any other input just asks again.

If the input is not interactive (for example when the output is piped) the end
of input works like pressing Enter and the simulation runs to the end.

Build with MinGW g++:
  g++ -std=c++17 -Wall -Wextra -O2 -o texas_holdem.exe texas_holdem.cpp

Build with MSVC:
  cl /std:c++17 /W4 /EHsc /Fe:texas_holdem.exe texas_holdem.cpp

Run:
  texas_holdem.exe
