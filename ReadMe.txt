Poker test program

texas_holdem.cpp
----------------
TEXAS HOLDEM single player simulation written in C++17.

The program shuffles a standard 52 card deck and deals two hole cards to the
single player. It then deals the flop (three cards), the turn (fourth card) and
the river (fifth card). After every street it shows the community cards, the
hole cards and the best hand that can be made from all of them, including the
cards that make up that hand. There are no opponents and no betting.

Hand categories, from weakest to strongest:
High Card, One Pair, Two Pair, Three of a Kind, Straight, Flush, Full House,
Four of a Kind, Straight Flush, Royal Flush.

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

  1. Press Enter to receive the two hole cards.
  2. Press Enter to see the flop (first three community cards).
  3. Press Enter to see the turn (fourth community card).
  4. Press Enter to see the river (fifth community card).
  5. Type q to exit.

After each step the community cards, the hole cards and the current best hand
are printed again. When the hand is over the program prints END OF HAND and
keeps the whole hand on screen. It then asks for q and only ends after q has
been typed, so the result can be reviewed for as long as needed. Any other
input just asks again.

If the input is not interactive (for example when the output is piped) the end
of input works like pressing Enter and the simulation runs to the end.

Build with MinGW g++:
  g++ -std=c++17 -Wall -Wextra -O2 -o texas_holdem.exe texas_holdem.cpp

Build with MSVC:
  cl /std:c++17 /W4 /EHsc /Fe:texas_holdem.exe texas_holdem.cpp

Run:
  texas_holdem.exe