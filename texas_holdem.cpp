// Texas Holdem two player simulation.
//
// The deck is shuffled and both players receive two hole cards. Then the flop
// (three cards), the turn (fourth card) and the river (fifth card) are dealt.
// After every street both players' best hand and the winning chances of the two
// players are printed again, and at the end the winner or a tie is shown.
//
// The winning chances are exact: every possible way of completing the board is
// played out and the results are counted, so no sampling error is involved.
//
// Every step is driven by the keyboard: pressing Enter receives the hole cards
// and shows the flop, the turn and the river. At the end of a hand an empty
// line deals the next hand and typing q ends the program.
//
// All output text is English. Suits are shown as spade, heart, diamond and club
// symbols, ranks use digits and J, Q, K, A, and no Chinese text is printed.

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace
{

enum class Suit
{
    Clubs,
    Diamonds,
    Hearts,
    Spades
};

enum class Rank
{
    Two = 2,
    Three,
    Four,
    Five,
    Six,
    Seven,
    Eight,
    Nine,
    Ten,
    Jack,
    Queen,
    King,
    Ace
};

struct Card
{
    Rank rank;
    Suit suit;
};

// Suits are printed as symbols. The UTF-8 byte sequences are written as escape
// sequences so that the source file itself stays pure ASCII and is compiled the
// same way by every compiler: spade U+2660, heart U+2665, diamond U+2666 and
// club U+2663.
const char *suitSymbol(Suit suit)
{
    switch (suit)
    {
    case Suit::Clubs:
        return "\xE2\x99\xA3";
    case Suit::Diamonds:
        return "\xE2\x99\xA6";
    case Suit::Hearts:
        return "\xE2\x99\xA5";
    case Suit::Spades:
        return "\xE2\x99\xA0";
    }
    return "?";
}

// Ranks are printed with digits and face card letters: 2 to 10, then J, Q, K,
// A.
const char *rankName(Rank rank)
{
    switch (rank)
    {
    case Rank::Two:
        return "2";
    case Rank::Three:
        return "3";
    case Rank::Four:
        return "4";
    case Rank::Five:
        return "5";
    case Rank::Six:
        return "6";
    case Rank::Seven:
        return "7";
    case Rank::Eight:
        return "8";
    case Rank::Nine:
        return "9";
    case Rank::Ten:
        return "10";
    case Rank::Jack:
        return "J";
    case Rank::Queen:
        return "Q";
    case Rank::King:
        return "K";
    case Rank::Ace:
        return "A";
    }
    return "Unknown";
}

int rankOf(const Card &card)
{
    return static_cast<int>(card.rank);
}

// A card is printed as its rank followed by its suit symbol, for example A
// followed by the spade symbol or 10 followed by the heart symbol.
std::string cardName(const Card &card)
{
    return std::string(rankName(card.rank)) + suitSymbol(card.suit);
}

// Hand categories from the weakest to the strongest.
enum HandCategory
{
    HighCard = 1,
    OnePair,
    TwoPair,
    ThreeOfAKind,
    Straight,
    Flush,
    FullHouse,
    FourOfAKind,
    StraightFlush,
    RoyalFlush
};

const char *categoryName(int category)
{
    switch (category)
    {
    case HighCard:
        return "High Card";
    case OnePair:
        return "One Pair";
    case TwoPair:
        return "Two Pair";
    case ThreeOfAKind:
        return "Three of a Kind";
    case Straight:
        return "Straight";
    case Flush:
        return "Flush";
    case FullHouse:
        return "Full House";
    case FourOfAKind:
        return "Four of a Kind";
    case StraightFlush:
        return "Straight Flush";
    case RoyalFlush:
        return "Royal Flush";
    }
    return "Unknown";
}

int categoryOf(int score)
{
    return score >> 20;
}

// Highest rank of the best straight inside the rank mask, or 0 when there is
// none. The ace counts as a low card in ace, five, four, three, two.
int highestStraight(int rankMask)
{
    for (int high = 14; high >= 6; --high)
    {
        const int run = (1 << high) | (1 << (high - 1)) | (1 << (high - 2)) |
                        (1 << (high - 3)) | (1 << (high - 4));
        if ((rankMask & run) == run)
        {
            return high;
        }
    }
    const int wheel = (1 << 14) | (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2);
    if ((rankMask & wheel) == wheel)
    {
        return 5;
    }
    return 0;
}

// Packs a hand into a single number: the category in the high bits and the
// ranks that break ties in the nibbles below, the most important rank first.
int packScore(int category, const int *ranks, int count)
{
    int score = category << 20;
    for (int i = 0; i < count && i < 5; ++i)
    {
        score |= ranks[i] << (16 - 4 * i);
    }
    return score;
}

// Scores the best five card hand that can be made from the given cards. Two
// cards up to seven cards are accepted; with fewer than five cards only pairs
// and trips are possible. One number per hand keeps comparisons cheap, which
// matters because the winning chances play out millions of boards.
int scoreHand(const Card *cards, int count)
{
    int rankCount[15] = {0};
    int suitCount[4] = {0};
    int suitRankMask[4] = {0};
    int rankMask = 0;

    for (int i = 0; i < count; ++i)
    {
        const int rank = rankOf(cards[i]);
        const int suit = static_cast<int>(cards[i].suit);
        ++rankCount[rank];
        ++suitCount[suit];
        suitRankMask[suit] |= 1 << rank;
        rankMask |= 1 << rank;
    }

    // Straight flush first, which also covers the royal flush.
    for (int suit = 0; suit < 4; ++suit)
    {
        if (suitCount[suit] < 5)
        {
            continue;
        }
        const int high = highestStraight(suitRankMask[suit]);
        if (high != 0)
        {
            const int ranks[1] = {high};
            return packScore(high == 14 ? RoyalFlush : StraightFlush, ranks, 1);
        }
    }

    // The ranks grouped by how often they occur and then by rank. Bigger groups
    // come first, so a pair or a trip always sits in front of the kickers.
    int groups[7][2] = {{0, 0}};
    int groupCount = 0;
    for (int rank = 14; rank >= 2; --rank)
    {
        if (rankCount[rank] > 0)
        {
            groups[groupCount][0] = rankCount[rank];
            groups[groupCount][1] = rank;
            ++groupCount;
        }
    }
    for (int i = 1; i < groupCount; ++i)
    {
        const int groupSize = groups[i][0];
        const int groupRank = groups[i][1];
        int j = i - 1;
        while (j >= 0 && groups[j][0] < groupSize)
        {
            groups[j + 1][0] = groups[j][0];
            groups[j + 1][1] = groups[j][1];
            --j;
        }
        groups[j + 1][0] = groupSize;
        groups[j + 1][1] = groupRank;
    }

    int ranks[5] = {0, 0, 0, 0, 0};
    for (int i = 0; i < groupCount && i < 5; ++i)
    {
        ranks[i] = groups[i][1];
    }

    if (groups[0][0] == 4)
    {
        return packScore(FourOfAKind, ranks, std::min(2, groupCount));
    }
    if (groups[0][0] == 3 && groupCount > 1 && groups[1][0] >= 2)
    {
        return packScore(FullHouse, ranks, std::min(2, groupCount));
    }
    for (int suit = 0; suit < 4; ++suit)
    {
        if (suitCount[suit] < 5)
        {
            continue;
        }
        // The five highest cards of the flush suit.
        int flushRanks[5] = {0, 0, 0, 0, 0};
        int found = 0;
        for (int rank = 14; rank >= 2 && found < 5; --rank)
        {
            if ((suitRankMask[suit] & (1 << rank)) != 0)
            {
                flushRanks[found] = rank;
                ++found;
            }
        }
        return packScore(Flush, flushRanks, found);
    }
    const int straightHigh = highestStraight(rankMask);
    if (straightHigh != 0)
    {
        const int straightRanks[1] = {straightHigh};
        return packScore(Straight, straightRanks, 1);
    }
    if (groups[0][0] == 3)
    {
        return packScore(ThreeOfAKind, ranks, std::min(3, groupCount));
    }
    if (groups[0][0] == 2 && groupCount > 1 && groups[1][0] == 2)
    {
        return packScore(TwoPair, ranks, std::min(3, groupCount));
    }
    if (groups[0][0] == 2)
    {
        return packScore(OnePair, ranks, std::min(4, groupCount));
    }
    return packScore(HighCard, ranks, std::min(5, groupCount));
}

struct HandResult
{
    int score = 0;
    std::vector<Card> cards;
};

void sortByRankDescending(std::vector<Card> &cards)
{
    std::sort(cards.begin(), cards.end(), [](const Card &lhs, const Card &rhs)
              { return rankOf(lhs) > rankOf(rhs); });
}

// Picks the strongest hand out of all the cards that are in play and reports
// the cards that make it up.
HandResult findBestHand(const std::vector<Card> &cards)
{
    HandResult result;
    result.score = scoreHand(cards.data(), static_cast<int>(cards.size()));
    result.cards = cards;

    if (cards.size() > 5)
    {
        const int total = static_cast<int>(cards.size());
        int best = -1;
        for (int a = 0; a + 4 < total; ++a)
        {
            for (int b = a + 1; b + 3 < total; ++b)
            {
                for (int c = b + 1; c + 2 < total; ++c)
                {
                    for (int d = c + 1; d + 1 < total; ++d)
                    {
                        for (int e = d + 1; e < total; ++e)
                        {
                            const Card combo[5] = {cards[a], cards[b], cards[c],
                                                   cards[d], cards[e]};
                            const int score = scoreHand(combo, 5);
                            if (score > best)
                            {
                                best = score;
                                result.cards.assign(combo, combo + 5);
                            }
                        }
                    }
                }
            }
        }
        result.score = best;
    }

    sortByRankDescending(result.cards);
    return result;
}

struct Equity
{
    long long player1Wins = 0;
    long long player2Wins = 0;
    long long ties = 0;
    long long total = 0;
};

// Works out the winning chances by playing out every possible way of completing
// the board, so the numbers are exact instead of sampled.
Equity computeEquity(const std::vector<Card> &hole1,
                     const std::vector<Card> &hole2,
                     const std::vector<Card> &community)
{
    bool known[4][15] = {{false}};
    for (const Card &card : hole1)
    {
        known[static_cast<int>(card.suit)][rankOf(card)] = true;
    }
    for (const Card &card : hole2)
    {
        known[static_cast<int>(card.suit)][rankOf(card)] = true;
    }
    for (const Card &card : community)
    {
        known[static_cast<int>(card.suit)][rankOf(card)] = true;
    }

    std::vector<Card> unknown;
    for (int suit = 0; suit < 4; ++suit)
    {
        for (int rank = 2; rank <= 14; ++rank)
        {
            if (!known[suit][rank])
            {
                unknown.push_back(
                    Card{static_cast<Rank>(rank), static_cast<Suit>(suit)});
            }
        }
    }

    const int needed = 5 - static_cast<int>(community.size());

    // Both hands hold the board plus the two hole cards of their player.
    Card hand1[7] = {};
    Card hand2[7] = {};
    for (std::size_t i = 0; i < hole1.size(); ++i)
    {
        hand1[i] = hole1[i];
        hand2[i] = hole2[i];
    }
    for (std::size_t i = 0; i < community.size(); ++i)
    {
        hand1[2 + i] = community[i];
        hand2[2 + i] = community[i];
    }
    const int boardStart = 2 + static_cast<int>(community.size());

    Equity equity;
    std::vector<int> pick(static_cast<std::size_t>(needed));
    for (int i = 0; i < needed; ++i)
    {
        pick[i] = i;
    }

    while (true)
    {
        for (int i = 0; i < needed; ++i)
        {
            const Card card = unknown[static_cast<std::size_t>(pick[i])];
            hand1[boardStart + i] = card;
            hand2[boardStart + i] = card;
        }

        const int score1 = scoreHand(hand1, 7);
        const int score2 = scoreHand(hand2, 7);
        if (score1 > score2)
        {
            ++equity.player1Wins;
        }
        else if (score2 > score1)
        {
            ++equity.player2Wins;
        }
        else
        {
            ++equity.ties;
        }
        ++equity.total;

        // Move on to the next combination of unknown cards.
        int i = needed - 1;
        while (i >= 0 &&
               pick[i] == static_cast<int>(unknown.size()) - needed + i)
        {
            --i;
        }
        if (i < 0)
        {
            break;
        }
        ++pick[i];
        for (int j = i + 1; j < needed; ++j)
        {
            pick[j] = pick[j - 1] + 1;
        }
    }

    return equity;
}

// Turns the counts into whole percentages that add up to exactly 100. Points
// left over by the rounding go to the largest remainders.
std::array<int, 3> toPercentages(const Equity &equity)
{
    const long long counts[3] = {equity.player1Wins, equity.player2Wins,
                                 equity.ties};
    std::array<int, 3> percent = {0, 0, 0};
    long long remainders[3] = {0, 0, 0};
    long long usedPoints = 0;

    for (int i = 0; i < 3; ++i)
    {
        percent[i] = static_cast<int>(counts[i] * 100 / equity.total);
        remainders[i] = counts[i] * 100 % equity.total;
        usedPoints += percent[i];
    }

    while (usedPoints < 100)
    {
        int best = 0;
        for (int i = 1; i < 3; ++i)
        {
            if (remainders[i] > remainders[best])
            {
                best = i;
            }
        }
        ++percent[best];
        remainders[best] = -1;
        ++usedPoints;
    }
    return percent;
}

class Deck
{
  public:
    Deck()
    {
        for (int suit = 0; suit < 4; ++suit)
        {
            for (int rank = static_cast<int>(Rank::Two);
                 rank <= static_cast<int>(Rank::Ace); ++rank)
            {
                cards_.push_back(
                    Card{static_cast<Rank>(rank), static_cast<Suit>(suit)});
            }
        }
    }

    void shuffle(std::mt19937 &engine)
    {
        std::shuffle(cards_.begin(), cards_.end(), engine);
        nextCard_ = 0;
    }

    Card deal()
    {
        return cards_[nextCard_++];
    }

    // Takes a card out of the cards that are still to be dealt, so that the
    // board can never repeat a card that was chosen by hand.
    void takeOut(const Card &card)
    {
        for (std::size_t i = nextCard_; i < cards_.size(); ++i)
        {
            if (cards_[i].rank == card.rank && cards_[i].suit == card.suit)
            {
                std::swap(cards_[i], cards_[nextCard_]);
                ++nextCard_;
                return;
            }
        }
    }

  private:
    std::vector<Card> cards_;
    std::size_t nextCard_ = 0;
};

void printCardList(const std::string &title, const std::vector<Card> &cards)
{
    std::cout << title << '\n';
    if (cards.empty())
    {
        std::cout << "  None dealt yet\n\n";
        return;
    }
    for (const Card &card : cards)
    {
        // Only the two character rank 10 needs no padding, so that every suit
        // symbol lines up in the same column.
        std::cout << "  "
                  << (rankOf(card) == static_cast<int>(Rank::Ten) ? "" : " ")
                  << cardName(card) << '\n';
    }
    std::cout << '\n';
}

std::string cardList(const std::vector<Card> &cards)
{
    std::string text;
    for (std::size_t i = 0; i < cards.size(); ++i)
    {
        if (i > 0)
        {
            text += ", ";
        }
        text += cardName(cards[i]);
    }
    return text;
}

std::vector<Card> handWithBoard(const std::vector<Card> &holeCards,
                                const std::vector<Card> &communityCards)
{
    std::vector<Card> all = holeCards;
    all.insert(all.end(), communityCards.begin(), communityCards.end());
    return all;
}

void printPlayer(const std::string &name, const std::vector<Card> &holeCards,
                 const std::vector<Card> &communityCards)
{
    printCardList(name + " HOLE CARDS", holeCards);

    const HandResult best =
        findBestHand(handWithBoard(holeCards, communityCards));
    std::cout << name << " BEST HAND\n";
    std::cout << "  " << categoryName(categoryOf(best.score)) << '\n';
    std::cout << "  Made with " << cardList(best.cards) << "\n\n";
}

void printWinChance(const Equity &equity)
{
    const std::array<int, 3> percent = toPercentages(equity);
    std::cout << "WIN CHANCE\n";
    std::cout << "  Player 1: " << percent[0] << "%\n";
    std::cout << "  Player 2: " << percent[1] << "%\n";
    std::cout << "  Tie: " << percent[2] << "%\n\n";
}

void printStreet(const std::string &street, const std::vector<Card> &hole1,
                 const std::vector<Card> &hole2,
                 const std::vector<Card> &communityCards)
{
    std::cout << street << "\n\n";

    if (!communityCards.empty())
    {
        printCardList("COMMUNITY CARDS", communityCards);
    }
    printPlayer("PLAYER 1", hole1, communityCards);
    printPlayer("PLAYER 2", hole2, communityCards);
    printWinChance(computeEquity(hole1, hole2, communityCards));
    std::cout.flush();
}

// The river is complete, so the winner of the showdown is known exactly.
void printResult(const std::vector<Card> &hole1, const std::vector<Card> &hole2,
                 const std::vector<Card> &communityCards)
{
    const std::vector<Card> all1 = handWithBoard(hole1, communityCards);
    const std::vector<Card> all2 = handWithBoard(hole2, communityCards);
    const int score1 = scoreHand(all1.data(), static_cast<int>(all1.size()));
    const int score2 = scoreHand(all2.data(), static_cast<int>(all2.size()));

    std::cout << "RESULT\n";
    if (score1 > score2)
    {
        std::cout << "  Player 1 wins with " << categoryName(categoryOf(score1))
                  << "\n\n";
    }
    else if (score2 > score1)
    {
        std::cout << "  Player 2 wins with " << categoryName(categoryOf(score2))
                  << "\n\n";
    }
    else
    {
        std::cout << "  It is a tie with " << categoryName(categoryOf(score1))
                  << "\n\n";
    }
}

// Turns text such as AS, 10h or td into a card. Returns false when the text is
// not a card.
bool parseCard(const std::string &text, Card &card)
{
    if (text.size() < 2)
    {
        return false;
    }

    Suit suit = Suit::Spades;
    switch (std::toupper(static_cast<unsigned char>(text.back())))
    {
    case 'S':
        suit = Suit::Spades;
        break;
    case 'H':
        suit = Suit::Hearts;
        break;
    case 'D':
        suit = Suit::Diamonds;
        break;
    case 'C':
        suit = Suit::Clubs;
        break;
    default:
        return false;
    }

    const std::string rankText = text.substr(0, text.size() - 1);
    int rank = 0;
    if (rankText == "10")
    {
        rank = 10;
    }
    else if (rankText.size() == 1 && rankText[0] >= '2' && rankText[0] <= '9')
    {
        rank = rankText[0] - '0';
    }
    else if (rankText.size() == 1)
    {
        switch (std::toupper(static_cast<unsigned char>(rankText[0])))
        {
        case 'T':
            rank = 10;
            break;
        case 'J':
            rank = 11;
            break;
        case 'Q':
            rank = 12;
            break;
        case 'K':
            rank = 13;
            break;
        case 'A':
            rank = 14;
            break;
        default:
            return false;
        }
    }
    else
    {
        return false;
    }

    card = Card{static_cast<Rank>(rank), suit};
    return true;
}

// Splits entered text into card tokens. Spaces, tabs, commas and slashes all
// separate one card from the next.
std::vector<std::string> splitCardTokens(const std::string &text)
{
    std::vector<std::string> tokens;
    std::string current;
    for (const char ch : text)
    {
        if (ch == ' ' || ch == '\t' || ch == ',' || ch == '/')
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }
        current += ch;
    }
    if (!current.empty())
    {
        tokens.push_back(current);
    }
    return tokens;
}

enum class HoleCards
{
    Random,
    Manual,
    Quit
};

// Asks for the hole cards of both players. An empty line deals random cards, q
// leaves the program, and any other input has to be four cards without a
// repeated card. The first two cards go to player 1.
HoleCards askHoleCards(std::vector<Card> &hole1, std::vector<Card> &hole2)
{
    const std::string prompt = "\nHole cards (for example AS KH 10D 2C), or "
                               "press Enter for random cards: ";

    std::string line;
    while (true)
    {
        std::cout << prompt;
        std::cout.flush();
        if (!std::getline(std::cin, line))
        {
            // The end of input is treated as pressing Enter, so a piped run
            // plays one hand with random cards and then stops at the end of the
            // hand instead of stopping here.
            std::cout << '\n';
            return HoleCards::Random;
        }

        const std::size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
        {
            return HoleCards::Random;
        }
        const std::size_t last = line.find_last_not_of(" \t\r\n");
        const std::string trimmed = line.substr(first, last - first + 1);
        if (trimmed == "q" || trimmed == "Q")
        {
            return HoleCards::Quit;
        }

        const std::vector<std::string> tokens = splitCardTokens(trimmed);
        if (tokens.size() != 4)
        {
            std::cout << "  Enter exactly four cards, two for each player.\n";
            continue;
        }

        std::vector<Card> cards;
        for (const std::string &token : tokens)
        {
            Card card{};
            if (!parseCard(token, card))
            {
                std::cout << "  Not a card: " << token
                          << "  (use a rank and a suit, for example AS KH 10D "
                             "2C)\n";
                cards.clear();
                break;
            }
            cards.push_back(card);
        }
        if (cards.size() != 4)
        {
            continue;
        }

        std::string repeated;
        for (std::size_t i = 0; i < cards.size() && repeated.empty(); ++i)
        {
            for (std::size_t j = i + 1; j < cards.size(); ++j)
            {
                if (cards[i].rank == cards[j].rank &&
                    cards[i].suit == cards[j].suit)
                {
                    repeated = cardName(cards[i]);
                    break;
                }
            }
        }
        if (!repeated.empty())
        {
            std::cout << "  " << repeated
                      << " was entered more than once. Every card can be used "
                         "only once.\n";
            continue;
        }

        hole1 = {cards[0], cards[1]};
        hole2 = {cards[2], cards[3]};
        return HoleCards::Manual;
    }
}

// Stops until the player presses Enter, so that every step is dealt on demand.
// When the input is not interactive the end of input is treated as Enter and
// the simulation keeps running instead of blocking.
void waitForEnter(const std::string &message)
{
    std::cout << message;
    std::cout.flush();

    std::string line;
    if (!std::getline(std::cin, line))
    {
        std::cout << '\n';
        return;
    }
}

// Waits at the end of a hand. An empty line deals the next hand, q ends the
// program, and any other input asks again. The end of input also ends the
// program. Returns true when another hand should be dealt.
bool askPlayAgain()
{
    const std::string prompt = "\nPress Enter to play another hand, or type q "
                               "and press Enter to exit: ";
    std::cout << prompt;
    std::cout.flush();

    std::string line;
    while (std::getline(std::cin, line))
    {
        const std::size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
        {
            std::cout << "\n\n";
            return true;
        }
        const std::size_t last = line.find_last_not_of(" \t\r\n");
        const std::string trimmed = line.substr(first, last - first + 1);
        if (trimmed == "q" || trimmed == "Q")
        {
            return false;
        }
        std::cout << prompt;
        std::cout.flush();
    }
    std::cout << '\n';
    return false;
}

} // namespace

int main()
{
#ifdef _WIN32
    // The suit symbols are written as UTF-8, so the console has to use the same
    // code page for them to show up as spade, heart, diamond and club. The
    // original code page is put back when the program ends.
    const UINT previousCodePage = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::random_device device;
    std::mt19937 engine(device());

    std::cout << "TEXAS HOLDEM   TWO PLAYER SIMULATION\n\n";
    std::cout
        << "Two players share one shuffled deck and both hole cards are\n";
    std::cout
        << "known. Every possible way of completing the board is played\n";
    std::cout << "out, so the winning chances below are exact.\n\n";
    std::cout
        << "Enter the hole cards yourself before each hand: four cards,\n";
    std::cout << "player 1 first, for example AS KH 10D 2C, using the ranks\n";
    std::cout << "2 to 10, J, Q, K, A and the suits S, H, D, C. Press Enter\n";
    std::cout << "instead to deal random cards. A repeated card is refused\n";
    std::cout << "with a message and asked for again.\n";
    std::cout << "Press Enter to move to the next step after that.\n";

    bool playAgain = true;
    while (playAgain)
    {
        Deck deck;
        deck.shuffle(engine);

        std::vector<Card> hole1;
        std::vector<Card> hole2;
        std::vector<Card> communityCards;

        const HoleCards input = askHoleCards(hole1, hole2);
        if (input == HoleCards::Quit)
        {
            break;
        }
        if (input == HoleCards::Random)
        {
            hole1.push_back(deck.deal());
            hole1.push_back(deck.deal());
            hole2.push_back(deck.deal());
            hole2.push_back(deck.deal());
        }
        else
        {
            // The chosen cards leave the deck, so the board cannot repeat them.
            for (const Card &card : hole1)
            {
                deck.takeOut(card);
            }
            for (const Card &card : hole2)
            {
                deck.takeOut(card);
            }
        }
        std::cout << "\n\n";
        printStreet("PREFLOP", hole1, hole2, communityCards);

        waitForEnter("Press Enter to see the flop: ");
        communityCards.push_back(deck.deal());
        communityCards.push_back(deck.deal());
        communityCards.push_back(deck.deal());
        std::cout << "\n\n";
        printStreet("FLOP", hole1, hole2, communityCards);

        waitForEnter("Press Enter to see the turn: ");
        communityCards.push_back(deck.deal());
        std::cout << "\n\n";
        printStreet("TURN", hole1, hole2, communityCards);

        waitForEnter("Press Enter to see the river: ");
        communityCards.push_back(deck.deal());
        std::cout << "\n\n";
        printStreet("RIVER", hole1, hole2, communityCards);

        printResult(hole1, hole2, communityCards);

        std::cout << "END OF HAND\n";
        std::cout
            << "All cards and percentages stay on screen so the hand can be\n";
        std::cout << "reviewed.\n";

        playAgain = askPlayAgain();
    }

    std::cout << "Goodbye.\n";

#ifdef _WIN32
    if (previousCodePage != 0)
    {
        SetConsoleOutputCP(previousCodePage);
    }
#endif
    return 0;
}
