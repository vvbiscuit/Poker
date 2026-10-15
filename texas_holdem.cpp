// Texas Holdem single player simulation.
//
// The deck is shuffled and the player receives two hole cards. Then the flop
// (three cards), the turn (fourth card) and the river (fifth card) are dealt.
// After every betting round the best hand that can be made from the player's
// hole cards and the community cards is printed again.
//
// Every step is driven by the keyboard: the player presses Enter to receive
// the hole cards and to see the flop, the turn and the river, and types q to
// leave the program once the hand has been reviewed.
//
// All output text is English. Suits are shown as spade, heart, diamond and club
// symbols, ranks use digits and J, Q, K, A, and no Chinese text is printed.

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

enum class Suit { Clubs, Diamonds, Hearts, Spades };

enum class Rank {
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

struct Card {
    Rank rank;
    Suit suit;
};

// Suits are printed as symbols. The UTF-8 byte sequences are written as escape
// sequences so that the source file itself stays pure ASCII and is compiled the
// same way by every compiler: spade U+2660, heart U+2665, diamond U+2666 and
// club U+2663.
const char* suitSymbol(Suit suit) {
    switch (suit) {
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
// Ranks are printed with digits and face card letters: 2 to 10, then J, Q, K, A.
const char* rankName(Rank rank) {
    switch (rank) {
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

// A card is printed as its rank followed by its suit symbol, for example A
// followed by the spade symbol or 10 followed by the heart symbol.
std::string cardName(const Card& card) {
    return std::string(rankName(card.rank)) + suitSymbol(card.suit);
}

// Hand categories from the weakest to the strongest.
enum HandCategory {
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

const char* categoryName(int category) {
    switch (category) {
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

struct HandValue {
    int category = HighCard;
    // Ranks in comparison order, highest rank first. This resolves ties
    // between hands of the same category.
    std::vector<int> tiebreak;
};

bool isStronger(const HandValue& candidate, const HandValue& current) {
    if (candidate.category != current.category) {
        return candidate.category > current.category;
    }
    return candidate.tiebreak > current.tiebreak;
}

int rankOf(const Card& card) {
    return static_cast<int>(card.rank);
}

// Groups the cards by rank, ordered by group size and then by rank, so that
// pairs and trips end up in front of the kickers.
std::vector<std::pair<int, int>> rankGroups(const std::vector<Card>& cards) {
    std::vector<std::pair<int, int>> groups;
    for (const Card& card : cards) {
        const int rank = rankOf(card);
        auto it = std::find_if(groups.begin(), groups.end(),
                               [rank](const std::pair<int, int>& group) {
                                   return group.second == rank;
                               });
        if (it != groups.end()) {
            ++it->first;
        } else {
            groups.emplace_back(1, rank);
        }
    }
    std::sort(groups.begin(), groups.end(),
              [](const std::pair<int, int>& lhs, const std::pair<int, int>& rhs) {
                  if (lhs.first != rhs.first) {
                      return lhs.first > rhs.first;
                  }
                  return lhs.second > rhs.second;
              });
    return groups;
}

// Ranks listed with the grouped ranks first, for example K K K Q Q for a full
// house or A A K Q J for a pair of aces.
std::vector<int> comparisonRanks(const std::vector<Card>& cards) {
    std::vector<int> ranks;
    for (const std::pair<int, int>& group : rankGroups(cards)) {
        for (int i = 0; i < group.first; ++i) {
            ranks.push_back(group.second);
        }
    }
    return ranks;
}

HandValue evaluateFiveCards(const std::array<Card, 5>& hand) {
    std::vector<int> ranks;
    ranks.reserve(hand.size());
    for (const Card& card : hand) {
        ranks.push_back(rankOf(card));
    }
    std::sort(ranks.begin(), ranks.end(), std::greater<int>());

    bool isFlush = true;
    for (std::size_t i = 1; i < hand.size(); ++i) {
        if (hand[i].suit != hand[0].suit) {
            isFlush = false;
            break;
        }
    }

    std::vector<int> distinct = ranks;
    distinct.erase(std::unique(distinct.begin(), distinct.end()), distinct.end());

    bool isStraight = false;
    int straightHigh = 0;
    if (distinct.size() == 5) {
        if (distinct.front() - distinct.back() == 4) {
            isStraight = true;
            straightHigh = distinct.front();
        } else if (distinct.front() == 14 && distinct[1] == 5 && distinct[2] == 4 &&
                   distinct[3] == 3 && distinct[4] == 2) {
            // A, 5, 4, 3, 2. The ace counts as a low card.
            isStraight = true;
            straightHigh = 5;
        }
    }

    HandValue value;
    const std::vector<std::pair<int, int>> groups = rankGroups({hand.begin(), hand.end()});

    if (isStraight && isFlush) {
        value.category = (straightHigh == 14) ? RoyalFlush : StraightFlush;
        value.tiebreak = {straightHigh};
    } else if (groups.front().first == 4) {
        value.category = FourOfAKind;
        value.tiebreak = comparisonRanks({hand.begin(), hand.end()});
    } else if (groups.front().first == 3 && groups.size() > 1 && groups[1].first == 2) {
        value.category = FullHouse;
        value.tiebreak = comparisonRanks({hand.begin(), hand.end()});
    } else if (isFlush) {
        value.category = Flush;
        value.tiebreak = ranks;
    } else if (isStraight) {
        value.category = Straight;
        value.tiebreak = (straightHigh == 5) ? std::vector<int>{5, 4, 3, 2, 1} : ranks;
    } else if (groups.front().first == 3) {
        value.category = ThreeOfAKind;
        value.tiebreak = comparisonRanks({hand.begin(), hand.end()});
    } else if (groups.front().first == 2 && groups.size() > 1 && groups[1].first == 2) {
        value.category = TwoPair;
        value.tiebreak = comparisonRanks({hand.begin(), hand.end()});
    } else if (groups.front().first == 2) {
        value.category = OnePair;
        value.tiebreak = comparisonRanks({hand.begin(), hand.end()});
    } else {
        value.category = HighCard;
        value.tiebreak = ranks;
    }
    return value;
}

struct HandResult {
    HandValue value;
    std::vector<Card> cards;  // the cards that make the hand
};

// Fewer than five cards are available, so only pairs, trips and quads can be
// completed. Straights and flushes are impossible before the river.
HandResult evaluatePartialHand(const std::vector<Card>& cards) {
    HandResult result;
    const std::vector<std::pair<int, int>> groups = rankGroups(cards);

    switch (groups.front().first) {
        case 4:
            result.value.category = FourOfAKind;
            break;
        case 3:
            result.value.category = (groups.size() > 1 && groups[1].first >= 2) ? FullHouse
                                                                               : ThreeOfAKind;
            break;
        case 2:
            result.value.category =
                (groups.size() > 1 && groups[1].first == 2) ? TwoPair : OnePair;
            break;
        default:
            result.value.category = HighCard;
            break;
    }

    result.value.tiebreak = comparisonRanks(cards);
    result.cards = cards;
    return result;
}

void sortByRankDescending(std::vector<Card>& cards) {
    std::sort(cards.begin(), cards.end(), [](const Card& lhs, const Card& rhs) {
        return rankOf(lhs) > rankOf(rhs);
    });
}

// Picks the strongest five card hand out of all the cards that are in play.
HandResult findBestHand(const std::vector<Card>& cards) {
    if (cards.size() < 5) {
        HandResult result = evaluatePartialHand(cards);
        sortByRankDescending(result.cards);
        return result;
    }

    HandResult best;
    bool hasBest = false;
    const std::size_t count = cards.size();

    for (std::size_t a = 0; a + 4 < count; ++a) {
        for (std::size_t b = a + 1; b + 3 < count; ++b) {
            for (std::size_t c = b + 1; c + 2 < count; ++c) {
                for (std::size_t d = c + 1; d + 1 < count; ++d) {
                    for (std::size_t e = d + 1; e < count; ++e) {
                        const std::array<Card, 5> hand = {cards[a], cards[b], cards[c], cards[d],
                                                          cards[e]};
                        const HandValue value = evaluateFiveCards(hand);
                        if (!hasBest || isStronger(value, best.value)) {
                            best.value = value;
                            best.cards.assign(hand.begin(), hand.end());
                            hasBest = true;
                        }
                    }
                }
            }
        }
    }

    sortByRankDescending(best.cards);
    return best;
}

class Deck {
public:
    Deck() {
        for (int suit = 0; suit < 4; ++suit) {
            for (int rank = static_cast<int>(Rank::Two); rank <= static_cast<int>(Rank::Ace);
                 ++rank) {
                cards_.push_back(Card{static_cast<Rank>(rank), static_cast<Suit>(suit)});
            }
        }
    }

    void shuffle(std::mt19937& engine) {
        std::shuffle(cards_.begin(), cards_.end(), engine);
        nextCard_ = 0;
    }

    Card deal() {
        return cards_[nextCard_++];
    }

private:
    std::vector<Card> cards_;
    std::size_t nextCard_ = 0;
};

void printCardList(const std::string& title, const std::vector<Card>& cards) {
    std::cout << title << '\n';
    if (cards.empty()) {
        std::cout << "  None dealt yet\n\n";
        return;
    }
    for (const Card& card : cards) {
        // Only the two character rank 10 needs no padding, so that every suit
        // symbol lines up in the same column.
        std::cout << "  " << (rankOf(card) == static_cast<int>(Rank::Ten) ? "" : " ")
                  << cardName(card) << '\n';
    }
    std::cout << '\n';
}

void printStreet(const std::string& street, const std::vector<Card>& holeCards,
                 const std::vector<Card>& communityCards, const std::string& handTitle) {
    std::cout << street << "\n\n";

    if (!communityCards.empty()) {
        printCardList("COMMUNITY CARDS", communityCards);
    }
    printCardList("YOUR HOLE CARDS", holeCards);

    std::vector<Card> allCards = holeCards;
    allCards.insert(allCards.end(), communityCards.begin(), communityCards.end());
    const HandResult best = findBestHand(allCards);

    std::cout << handTitle << '\n';
    std::cout << "  " << categoryName(best.value.category) << '\n';
    std::cout << "  Made with ";
    for (std::size_t i = 0; i < best.cards.size(); ++i) {
        if (i > 0) {
            std::cout << ", ";
        }
        std::cout << cardName(best.cards[i]);
    }
    std::cout << "\n\n";
    std::cout.flush();
}

// The program only ends when the player types q, so the finished hand stays on
// screen for as long as it is being reviewed. Any other input asks again, and
// the end of input also ends the program.
void waitForQuit() {
    const std::string prompt = "\nType q and press Enter to exit: ";
    std::cout << prompt;
    std::cout.flush();

    std::string line;
    while (std::getline(std::cin, line)) {
        const std::size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            std::cout << prompt;
            std::cout.flush();
            continue;
        }
        const std::size_t last = line.find_last_not_of(" \t\r\n");
        const std::string trimmed = line.substr(first, last - first + 1);
        if (trimmed == "q" || trimmed == "Q") {
            std::cout << "Goodbye.\n";
            return;
        }
        std::cout << prompt;
        std::cout.flush();
    }
    std::cout << '\n';
}

// Stops until the player presses Enter, so that every step is dealt on demand.
// When the input is not interactive the end of input is treated as Enter and
// the simulation keeps running instead of blocking.
void waitForEnter(const std::string& message) {
    std::cout << message;
    std::cout.flush();

    std::string line;
    if (!std::getline(std::cin, line)) {
        std::cout << '\n';
        return;
    }
}

}  // namespace

int main() {
#ifdef _WIN32
    // The suit symbols are written as UTF-8, so the console has to use the same
    // code page for them to show up as spade, heart, diamond and club. The
    // original code page is put back when the program ends.
    const UINT previousCodePage = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::random_device device;
    std::mt19937 engine(device());

    Deck deck;
    deck.shuffle(engine);

    std::vector<Card> holeCards;
    std::vector<Card> communityCards;

    std::cout << "TEXAS HOLDEM   SINGLE PLAYER SIMULATION\n\n";
    std::cout << "One player only. The deck is shuffled and cards are dealt\n";
    std::cout << "in the usual order: preflop, flop, turn and river.\n";
    std::cout << "Press Enter to move to the next step.\n\n";

    waitForEnter("Press Enter to receive your two hole cards: ");
    holeCards.push_back(deck.deal());
    holeCards.push_back(deck.deal());
    std::cout << "\n\n";
    printStreet("PREFLOP", holeCards, communityCards, "YOUR BEST HAND");

    waitForEnter("Press Enter to see the flop: ");
    communityCards.push_back(deck.deal());
    communityCards.push_back(deck.deal());
    communityCards.push_back(deck.deal());
    std::cout << "\n\n";
    printStreet("FLOP", holeCards, communityCards, "BEST HAND SO FAR");

    waitForEnter("Press Enter to see the turn: ");
    communityCards.push_back(deck.deal());
    std::cout << "\n\n";
    printStreet("TURN", holeCards, communityCards, "BEST HAND SO FAR");

    waitForEnter("Press Enter to see the river: ");
    communityCards.push_back(deck.deal());
    std::cout << "\n\n";
    printStreet("RIVER", holeCards, communityCards, "FINAL BEST HAND");

    std::cout << "END OF HAND\n";
    std::cout << "All cards stay on screen so the hand can be reviewed.\n";

    waitForQuit();

#ifdef _WIN32
    if (previousCodePage != 0) {
        SetConsoleOutputCP(previousCodePage);
    }
#endif
    return 0;
}
