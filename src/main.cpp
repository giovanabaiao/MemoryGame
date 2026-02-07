#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
namespace fs = std::filesystem;

constexpr int kColumns = 8;
constexpr int kRows = 4;
constexpr int kPairCount = 16;
constexpr int kCardCount = kColumns * kRows;

constexpr float kVirtualWidth = 1920.0F;
constexpr float kVirtualHeight = 1080.0F;
constexpr float kCardAspectRatio = 3.0F / 4.0F; // width / height

constexpr float kFlipDurationSeconds = 0.22F;
constexpr float kRevealDurationSeconds = 2.0F;
constexpr float kMatchRemoveDurationSeconds = 0.20F;

struct CharacterInfo
{
    std::string name;
    std::string slug;
    sf::Color fallbackColor;
};

const std::array<CharacterInfo, kPairCount> kCharacters{{
    {"Luke Skywalker", "luke_skywalker", sf::Color(226, 188, 94)},
    {"Leia Organa", "leia_organa", sf::Color(235, 152, 152)},
    {"Darth Vader", "darth_vader", sf::Color(155, 155, 170)},
    {"Anakin Skywalker", "anakin_skywalker", sf::Color(124, 172, 232)},
    {"Obi-Wan Kenobi", "obi_wan_kenobi", sf::Color(185, 147, 95)},
    {"Yoda", "yoda", sf::Color(122, 194, 122)},
    {"Han Solo", "han_solo", sf::Color(196, 170, 124)},
    {"Chewbacca", "chewbacca", sf::Color(155, 99, 74)},
    {"Emperor Palpatine", "emperor_palpatine", sf::Color(131, 120, 170)},
    {"Rey", "rey", sf::Color(224, 211, 170)},
    {"Kylo Ren", "kylo_ren", sf::Color(196, 110, 110)},
    {"R2-D2", "r2_d2", sf::Color(169, 198, 232)},
    {"C-3PO", "c_3po", sf::Color(217, 174, 75)},
    {"Lando Calrissian", "lando_calrissian", sf::Color(130, 185, 199)},
    {"Mandalorian", "mandalorian", sf::Color(171, 183, 190)},
    {"Padme Amidala", "padme_amidala", sf::Color(228, 162, 180)},
}};

enum class CardState
{
    FaceDown,
    FlippingToFront,
    FaceUp,
    FlippingToBack,
    Matched,
    Removed
};

enum class PairPhase
{
    Idle,
    WaitingForSecondFlip,
    RevealWindow,
    Resolving
};

struct Card
{
    int slotIndex = 0;
    int characterIndex = 0;
    CardState state = CardState::FaceDown;
    sf::FloatRect bounds{{0.0F, 0.0F}, {0.0F, 0.0F}};
    bool frontVisible = false;
    bool flipFaceSwapped = false;
    float flipProgress = 0.0F;
    float removeProgress = 0.0F;
};

struct Layout
{
    float scale = 1.0F;
    float outlineThickness = 2.0F;
    sf::FloatRect playArea{{0.0F, 0.0F}, {kVirtualWidth, kVirtualHeight}};
    sf::FloatRect hudArea{{0.0F, 0.0F}, {kVirtualWidth, 180.0F}};
    sf::FloatRect gridArea{{0.0F, 180.0F}, {kVirtualWidth, kVirtualHeight - 180.0F}};
    sf::FloatRect newGameButton{{0.0F, 0.0F}, {220.0F, 70.0F}};
    std::array<sf::FloatRect, kCardCount> cardBounds{};
    unsigned int titleSize = 42U;
    unsigned int statsSize = 30U;
    unsigned int buttonSize = 28U;
    unsigned int cardLabelSize = 24U;
    unsigned int overlaySize = 52U;
};

bool containsPoint(const sf::FloatRect& rect, sf::Vector2f point)
{
    return point.x >= rect.position.x &&
           point.x <= rect.position.x + rect.size.x &&
           point.y >= rect.position.y &&
           point.y <= rect.position.y + rect.size.y;
}

float clamp01(float value)
{
    return std::clamp(value, 0.0F, 1.0F);
}

class MemoryGame
{
public:
    MemoryGame();
    void run();

private:
    void processEvents();
    void update(float deltaSeconds);
    void render();
    void recomputeLayout();
    void resetGame();
    void handleLeftClick(sf::Vector2f point);

    void startFlipToFront(int index);
    void startFlipToBack(int index);
    void resolveCurrentPair();

    bool isCardClickable(const Card& card) const;
    bool areSelectedCardsStableFaceUp() const;
    bool areSelectedCardsResolved() const;
    bool selectedCardsMatch() const;

    float computeFlipScaleX(const Card& card) const;
    bool shouldRenderFrontFace(const Card& card) const;

    void drawText(const std::string& value, sf::Vector2f position, unsigned int size, sf::Color color, bool centered);
    void drawCard(const Card& card);
    std::string formatElapsedTime() const;
    std::string makeInitials(const std::string& name) const;
    const sf::Texture* textureForCharacter(int characterIndex) const;
    void loadFont();
    void loadCharacterTextures();
    void updateCardBoundsFromLayout();

    sf::RenderWindow window_;
    sf::Clock frameClock_;
    Layout layout_;

    std::vector<Card> cards_;
    PairPhase pairPhase_ = PairPhase::Idle;
    int firstSelected_ = -1;
    int secondSelected_ = -1;
    float revealRemaining_ = 0.0F;
    int moves_ = 0;
    int matchedPairs_ = 0;
    float elapsedSeconds_ = 0.0F;
    bool timerRunning_ = false;
    bool won_ = false;
    bool layoutDirty_ = true;

    sf::Font font_;
    bool fontLoaded_ = false;
    std::unordered_map<int, sf::Texture> characterTextures_;

    std::mt19937 random_{std::random_device{}()};
};

MemoryGame::MemoryGame() :
    window_(sf::VideoMode::getDesktopMode(), "Star Wars Memory Game", sf::State::Fullscreen)
{
    window_.setVerticalSyncEnabled(true);
    cards_.resize(kCardCount);

    loadFont();
    loadCharacterTextures();
    recomputeLayout();
    resetGame();
}

void MemoryGame::run()
{
    while (window_.isOpen())
    {
        processEvents();

        float dt = frameClock_.restart().asSeconds();
        dt = std::min(dt, 0.1F);

        update(dt);
        render();
    }
}

void MemoryGame::processEvents()
{
    while (const std::optional event = window_.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            window_.close();
            continue;
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
        {
            if (keyPressed->code == sf::Keyboard::Key::Escape)
            {
                window_.close();
            }
            continue;
        }

        if (const auto* resized = event->getIf<sf::Event::Resized>())
        {
            window_.setView(sf::View(sf::FloatRect(
                sf::Vector2f(0.0F, 0.0F),
                sf::Vector2f(
                    static_cast<float>(resized->size.x),
                    static_cast<float>(resized->size.y)))));
            layoutDirty_ = true;
            continue;
        }

        if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
        {
            if (mousePressed->button == sf::Mouse::Button::Left)
            {
                handleLeftClick(
                    sf::Vector2f(
                        static_cast<float>(mousePressed->position.x),
                        static_cast<float>(mousePressed->position.y)));
            }
        }
    }
}

void MemoryGame::update(float deltaSeconds)
{
    if (layoutDirty_)
    {
        recomputeLayout();
    }

    if (timerRunning_ && !won_)
    {
        elapsedSeconds_ += deltaSeconds;
    }

    for (Card& card : cards_)
    {
        switch (card.state)
        {
            case CardState::FlippingToFront:
            case CardState::FlippingToBack:
            {
                card.flipProgress += deltaSeconds / kFlipDurationSeconds;
                const float normalized = clamp01(card.flipProgress);

                if (!card.flipFaceSwapped && normalized >= 0.5F)
                {
                    card.frontVisible = (card.state == CardState::FlippingToFront);
                    card.flipFaceSwapped = true;
                }

                if (normalized >= 1.0F)
                {
                    if (card.state == CardState::FlippingToFront)
                    {
                        card.state = CardState::FaceUp;
                        card.frontVisible = true;
                    }
                    else
                    {
                        card.state = CardState::FaceDown;
                        card.frontVisible = false;
                    }
                    card.flipProgress = 1.0F;
                }
                break;
            }
            case CardState::Matched:
            {
                card.removeProgress += deltaSeconds / kMatchRemoveDurationSeconds;
                if (card.removeProgress >= 1.0F)
                {
                    card.removeProgress = 1.0F;
                    card.state = CardState::Removed;
                }
                break;
            }
            case CardState::FaceDown:
            case CardState::FaceUp:
            case CardState::Removed:
            default:
                break;
        }
    }

    if (pairPhase_ == PairPhase::WaitingForSecondFlip)
    {
        if (areSelectedCardsStableFaceUp())
        {
            revealRemaining_ = kRevealDurationSeconds;
            pairPhase_ = PairPhase::RevealWindow;
        }
    }
    else if (pairPhase_ == PairPhase::RevealWindow)
    {
        revealRemaining_ -= deltaSeconds;
        if (revealRemaining_ <= 0.0F)
        {
            resolveCurrentPair();
        }
    }
    else if (pairPhase_ == PairPhase::Resolving)
    {
        if (areSelectedCardsResolved())
        {
            if (selectedCardsMatch())
            {
                matchedPairs_ += 1;
                if (matchedPairs_ >= kPairCount)
                {
                    won_ = true;
                    timerRunning_ = false;
                }
            }

            firstSelected_ = -1;
            secondSelected_ = -1;
            pairPhase_ = PairPhase::Idle;
        }
    }
}

void MemoryGame::render()
{
    window_.clear(sf::Color(10, 13, 20));

    sf::RectangleShape playFrame(layout_.playArea.size);
    playFrame.setPosition(layout_.playArea.position);
    playFrame.setFillColor(sf::Color(18, 24, 40));
    window_.draw(playFrame);

    sf::RectangleShape hud(layout_.hudArea.size);
    hud.setPosition(layout_.hudArea.position);
    hud.setFillColor(sf::Color(26, 35, 58));
    window_.draw(hud);

    sf::RectangleShape gridBg(layout_.gridArea.size);
    gridBg.setPosition(layout_.gridArea.position);
    gridBg.setFillColor(sf::Color(20, 27, 46));
    window_.draw(gridBg);

    sf::RectangleShape newGameButton(layout_.newGameButton.size);
    newGameButton.setPosition(layout_.newGameButton.position);
    newGameButton.setFillColor(sf::Color(78, 113, 170));
    newGameButton.setOutlineThickness(layout_.outlineThickness);
    newGameButton.setOutlineColor(sf::Color(199, 216, 241));
    window_.draw(newGameButton);

    for (const Card& card : cards_)
    {
        drawCard(card);
    }

    if (fontLoaded_)
    {
        drawText(
            "Star Wars Memory",
            sf::Vector2f(layout_.hudArea.position.x + 26.0F * layout_.scale, layout_.hudArea.position.y + 24.0F * layout_.scale),
            layout_.titleSize,
            sf::Color(245, 226, 121),
            false);

        drawText(
            "Time: " + formatElapsedTime(),
            sf::Vector2f(layout_.hudArea.position.x + 30.0F * layout_.scale, layout_.hudArea.position.y + 92.0F * layout_.scale),
            layout_.statsSize,
            sf::Color(228, 234, 248),
            false);

        drawText(
            "Moves: " + std::to_string(moves_),
            sf::Vector2f(layout_.hudArea.position.x + 410.0F * layout_.scale, layout_.hudArea.position.y + 92.0F * layout_.scale),
            layout_.statsSize,
            sf::Color(228, 234, 248),
            false);

        drawText(
            "New Game",
            sf::Vector2f(
                layout_.newGameButton.position.x + layout_.newGameButton.size.x * 0.5F,
                layout_.newGameButton.position.y + layout_.newGameButton.size.y * 0.5F),
            layout_.buttonSize,
            sf::Color::White,
            true);
    }

    if (won_)
    {
        sf::RectangleShape overlay(layout_.playArea.size);
        overlay.setPosition(layout_.playArea.position);
        overlay.setFillColor(sf::Color(0, 0, 0, 125));
        window_.draw(overlay);

        if (fontLoaded_)
        {
            drawText(
                "You Won!",
                sf::Vector2f(
                    layout_.playArea.position.x + layout_.playArea.size.x * 0.5F,
                    layout_.playArea.position.y + layout_.playArea.size.y * 0.46F),
                layout_.overlaySize,
                sf::Color(255, 250, 197),
                true);

            drawText(
                "Final Time: " + formatElapsedTime() + "   Moves: " + std::to_string(moves_),
                sf::Vector2f(
                    layout_.playArea.position.x + layout_.playArea.size.x * 0.5F,
                    layout_.playArea.position.y + layout_.playArea.size.y * 0.54F),
                layout_.statsSize,
                sf::Color(236, 240, 253),
                true);
        }
    }

    window_.display();
}

void MemoryGame::recomputeLayout()
{
    const sf::Vector2u windowSize = window_.getSize();
    const float width = static_cast<float>(windowSize.x);
    const float height = static_cast<float>(windowSize.y);

    layout_.scale = std::min(width / kVirtualWidth, height / kVirtualHeight);
    if (layout_.scale <= 0.0F)
    {
        layout_.scale = 1.0F;
    }

    const sf::Vector2f playSize{kVirtualWidth * layout_.scale, kVirtualHeight * layout_.scale};
    const sf::Vector2f playPos{
        (width - playSize.x) * 0.5F,
        (height - playSize.y) * 0.5F,
    };

    layout_.playArea = sf::FloatRect(playPos, playSize);

    const float hudHeight = playSize.y * 0.18F;
    layout_.hudArea = sf::FloatRect(playPos, sf::Vector2f(playSize.x, hudHeight));

    const float outerPad = 26.0F * layout_.scale;
    const float gridY = layout_.hudArea.position.y + layout_.hudArea.size.y + outerPad;
    const float gridHeight = (layout_.playArea.position.y + layout_.playArea.size.y) - gridY - outerPad;
    layout_.gridArea = sf::FloatRect(
        sf::Vector2f(layout_.playArea.position.x + outerPad, gridY),
        sf::Vector2f(layout_.playArea.size.x - outerPad * 2.0F, gridHeight));

    const float gap = 14.0F * layout_.scale;
    const float maxWidthFromGrid = (layout_.gridArea.size.x - gap * static_cast<float>(kColumns - 1)) / static_cast<float>(kColumns);
    const float maxHeightFromGrid = (layout_.gridArea.size.y - gap * static_cast<float>(kRows - 1)) / static_cast<float>(kRows);

    float cardWidth = maxWidthFromGrid;
    float cardHeight = cardWidth / kCardAspectRatio;
    if (cardHeight > maxHeightFromGrid)
    {
        cardHeight = maxHeightFromGrid;
        cardWidth = cardHeight * kCardAspectRatio;
    }

    const float totalGridWidth = cardWidth * static_cast<float>(kColumns) + gap * static_cast<float>(kColumns - 1);
    const float totalGridHeight = cardHeight * static_cast<float>(kRows) + gap * static_cast<float>(kRows - 1);
    const float startX = layout_.gridArea.position.x + (layout_.gridArea.size.x - totalGridWidth) * 0.5F;
    const float startY = layout_.gridArea.position.y + (layout_.gridArea.size.y - totalGridHeight) * 0.5F;

    for (int row = 0; row < kRows; ++row)
    {
        for (int column = 0; column < kColumns; ++column)
        {
            const int index = row * kColumns + column;
            const sf::Vector2f cardPos{
                startX + static_cast<float>(column) * (cardWidth + gap),
                startY + static_cast<float>(row) * (cardHeight + gap),
            };
            layout_.cardBounds[static_cast<std::size_t>(index)] = sf::FloatRect(cardPos, sf::Vector2f(cardWidth, cardHeight));
        }
    }

    const sf::Vector2f buttonSize{230.0F * layout_.scale, 70.0F * layout_.scale};
    const sf::Vector2f buttonPos{
        layout_.hudArea.position.x + layout_.hudArea.size.x - buttonSize.x - 24.0F * layout_.scale,
        layout_.hudArea.position.y + 26.0F * layout_.scale,
    };
    layout_.newGameButton = sf::FloatRect(buttonPos, buttonSize);

    layout_.outlineThickness = std::max(1.0F, 2.0F * layout_.scale);
    layout_.titleSize = static_cast<unsigned int>(std::max(20.0F, std::round(48.0F * layout_.scale)));
    layout_.statsSize = static_cast<unsigned int>(std::max(14.0F, std::round(30.0F * layout_.scale)));
    layout_.buttonSize = static_cast<unsigned int>(std::max(14.0F, std::round(28.0F * layout_.scale)));
    layout_.cardLabelSize = static_cast<unsigned int>(std::max(12.0F, std::round(24.0F * layout_.scale)));
    layout_.overlaySize = static_cast<unsigned int>(std::max(22.0F, std::round(56.0F * layout_.scale)));

    updateCardBoundsFromLayout();
    layoutDirty_ = false;
}

void MemoryGame::resetGame()
{
    std::vector<int> shuffledCharacters;
    shuffledCharacters.reserve(kCardCount);

    for (int i = 0; i < kPairCount; ++i)
    {
        shuffledCharacters.push_back(i);
        shuffledCharacters.push_back(i);
    }

    std::shuffle(shuffledCharacters.begin(), shuffledCharacters.end(), random_);

    for (int cardIndex = 0; cardIndex < kCardCount; ++cardIndex)
    {
        Card& card = cards_[static_cast<std::size_t>(cardIndex)];
        card.slotIndex = cardIndex;
        card.characterIndex = shuffledCharacters[static_cast<std::size_t>(cardIndex)];
        card.state = CardState::FaceDown;
        card.bounds = layout_.cardBounds[static_cast<std::size_t>(cardIndex)];
        card.frontVisible = false;
        card.flipFaceSwapped = false;
        card.flipProgress = 0.0F;
        card.removeProgress = 0.0F;
    }

    firstSelected_ = -1;
    secondSelected_ = -1;
    pairPhase_ = PairPhase::Idle;
    revealRemaining_ = 0.0F;
    matchedPairs_ = 0;
    moves_ = 0;
    elapsedSeconds_ = 0.0F;
    timerRunning_ = false;
    won_ = false;
}

void MemoryGame::handleLeftClick(sf::Vector2f point)
{
    if (layoutDirty_)
    {
        recomputeLayout();
    }

    if (containsPoint(layout_.newGameButton, point))
    {
        resetGame();
        return;
    }

    if (won_ || pairPhase_ != PairPhase::Idle)
    {
        return;
    }

    int clickedIndex = -1;
    for (int index = 0; index < kCardCount; ++index)
    {
        const Card& card = cards_[static_cast<std::size_t>(index)];
        if (!containsPoint(card.bounds, point))
        {
            continue;
        }

        if (!isCardClickable(card))
        {
            return;
        }

        clickedIndex = index;
        break;
    }

    if (clickedIndex < 0)
    {
        return;
    }

    if (!timerRunning_)
    {
        timerRunning_ = true;
    }

    startFlipToFront(clickedIndex);

    if (firstSelected_ < 0)
    {
        firstSelected_ = clickedIndex;
        return;
    }

    if (secondSelected_ < 0 && clickedIndex != firstSelected_)
    {
        secondSelected_ = clickedIndex;
        moves_ += 1;
        pairPhase_ = PairPhase::WaitingForSecondFlip;
    }
}

void MemoryGame::startFlipToFront(int index)
{
    Card& card = cards_[static_cast<std::size_t>(index)];
    if (card.state != CardState::FaceDown)
    {
        return;
    }

    card.state = CardState::FlippingToFront;
    card.flipProgress = 0.0F;
    card.flipFaceSwapped = false;
}

void MemoryGame::startFlipToBack(int index)
{
    Card& card = cards_[static_cast<std::size_t>(index)];
    if (card.state != CardState::FaceUp)
    {
        return;
    }

    card.state = CardState::FlippingToBack;
    card.flipProgress = 0.0F;
    card.flipFaceSwapped = false;
}

void MemoryGame::resolveCurrentPair()
{
    if (firstSelected_ < 0 || secondSelected_ < 0)
    {
        return;
    }

    Card& first = cards_[static_cast<std::size_t>(firstSelected_)];
    Card& second = cards_[static_cast<std::size_t>(secondSelected_)];

    if (first.characterIndex == second.characterIndex)
    {
        first.state = CardState::Matched;
        second.state = CardState::Matched;
        first.removeProgress = 0.0F;
        second.removeProgress = 0.0F;
        first.frontVisible = true;
        second.frontVisible = true;
    }
    else
    {
        startFlipToBack(firstSelected_);
        startFlipToBack(secondSelected_);
    }

    pairPhase_ = PairPhase::Resolving;
}

bool MemoryGame::isCardClickable(const Card& card) const
{
    return card.state == CardState::FaceDown;
}

bool MemoryGame::areSelectedCardsStableFaceUp() const
{
    if (firstSelected_ < 0 || secondSelected_ < 0)
    {
        return false;
    }

    const Card& first = cards_[static_cast<std::size_t>(firstSelected_)];
    const Card& second = cards_[static_cast<std::size_t>(secondSelected_)];
    return first.state == CardState::FaceUp && second.state == CardState::FaceUp;
}

bool MemoryGame::selectedCardsMatch() const
{
    if (firstSelected_ < 0 || secondSelected_ < 0)
    {
        return false;
    }

    const Card& first = cards_[static_cast<std::size_t>(firstSelected_)];
    const Card& second = cards_[static_cast<std::size_t>(secondSelected_)];
    return first.characterIndex == second.characterIndex;
}

bool MemoryGame::areSelectedCardsResolved() const
{
    if (firstSelected_ < 0 || secondSelected_ < 0)
    {
        return false;
    }

    const Card& first = cards_[static_cast<std::size_t>(firstSelected_)];
    const Card& second = cards_[static_cast<std::size_t>(secondSelected_)];

    if (selectedCardsMatch())
    {
        return first.state == CardState::Removed && second.state == CardState::Removed;
    }

    return first.state == CardState::FaceDown && second.state == CardState::FaceDown;
}

float MemoryGame::computeFlipScaleX(const Card& card) const
{
    if (card.state != CardState::FlippingToFront && card.state != CardState::FlippingToBack)
    {
        return 1.0F;
    }

    const float progress = clamp01(card.flipProgress);
    if (progress < 0.5F)
    {
        return std::max(0.02F, 1.0F - progress * 2.0F);
    }
    return std::max(0.02F, (progress - 0.5F) * 2.0F);
}

bool MemoryGame::shouldRenderFrontFace(const Card& card) const
{
    switch (card.state)
    {
        case CardState::FaceDown:
            return false;
        case CardState::FlippingToFront:
            return card.flipProgress >= 0.5F;
        case CardState::FaceUp:
            return true;
        case CardState::FlippingToBack:
            return card.flipProgress < 0.5F;
        case CardState::Matched:
            return true;
        case CardState::Removed:
            return false;
        default:
            return false;
    }
}

void MemoryGame::drawText(const std::string& value, sf::Vector2f position, unsigned int size, sf::Color color, bool centered)
{
    if (!fontLoaded_)
    {
        return;
    }

    sf::Text text(font_, value, size);
    text.setFillColor(color);

    if (centered)
    {
        const sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(
            sf::Vector2f(
                bounds.position.x + bounds.size.x * 0.5F,
                bounds.position.y + bounds.size.y * 0.5F));
    }

    text.setPosition(position);
    window_.draw(text);
}

void MemoryGame::drawCard(const Card& card)
{
    if (card.state == CardState::Removed)
    {
        return;
    }

    float vanishScale = 1.0F;
    std::uint8_t alpha = 255U;
    if (card.state == CardState::Matched)
    {
        const float t = clamp01(card.removeProgress);
        vanishScale = 1.0F - (0.40F * t);
        alpha = static_cast<std::uint8_t>(std::lround(255.0F * (1.0F - t)));
    }

    const float flipScale = computeFlipScaleX(card);
    const float scaleX = std::max(0.02F, flipScale * vanishScale);
    const float scaleY = std::max(0.02F, vanishScale);
    const bool showFront = shouldRenderFrontFace(card);

    sf::RectangleShape body(card.bounds.size);
    body.setOrigin(sf::Vector2f(card.bounds.size.x * 0.5F, card.bounds.size.y * 0.5F));
    body.setPosition(sf::Vector2f(
        card.bounds.position.x + card.bounds.size.x * 0.5F,
        card.bounds.position.y + card.bounds.size.y * 0.5F));
    body.setScale(sf::Vector2f(scaleX, scaleY));
    body.setOutlineThickness(layout_.outlineThickness);

    if (showFront)
    {
        if (const sf::Texture* texture = textureForCharacter(card.characterIndex))
        {
            body.setTexture(texture, true);
            body.setFillColor(sf::Color(255, 255, 255, alpha));
        }
        else
        {
            body.setTexture(nullptr, true);
            sf::Color color = kCharacters[static_cast<std::size_t>(card.characterIndex)].fallbackColor;
            color.a = alpha;
            body.setFillColor(color);
        }
        body.setOutlineColor(sf::Color(20, 22, 30, alpha));
    }
    else
    {
        body.setTexture(nullptr, true);
        body.setFillColor(sf::Color(30, 49, 86, alpha));
        body.setOutlineColor(sf::Color(175, 201, 238, alpha));
    }

    window_.draw(body);

    if (showFront && !textureForCharacter(card.characterIndex) && fontLoaded_)
    {
        drawText(
            makeInitials(kCharacters[static_cast<std::size_t>(card.characterIndex)].name),
            sf::Vector2f(
                card.bounds.position.x + card.bounds.size.x * 0.5F,
                card.bounds.position.y + card.bounds.size.y * 0.5F),
            layout_.cardLabelSize,
            sf::Color(10, 12, 20, alpha),
            true);
    }
}

std::string MemoryGame::formatElapsedTime() const
{
    const int totalSeconds = static_cast<int>(std::floor(elapsedSeconds_));
    const int hours = totalSeconds / 3600;
    const int minutes = (totalSeconds % 3600) / 60;
    const int seconds = totalSeconds % 60;

    std::ostringstream stream;
    stream << std::setfill('0');
    if (hours > 0)
    {
        stream << std::setw(2) << hours << ":";
    }
    stream << std::setw(2) << minutes << ":" << std::setw(2) << seconds;
    return stream.str();
}

std::string MemoryGame::makeInitials(const std::string& name) const
{
    std::string output;
    bool takeNext = true;

    for (char c : name)
    {
        const bool separator = (c == ' ' || c == '-');
        if (takeNext && std::isalnum(static_cast<unsigned char>(c)) != 0)
        {
            output.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
            if (output.size() >= 3)
            {
                break;
            }
        }
        takeNext = separator;
    }

    if (output.empty())
    {
        return "???";
    }
    return output;
}

const sf::Texture* MemoryGame::textureForCharacter(int characterIndex) const
{
    const auto iter = characterTextures_.find(characterIndex);
    if (iter == characterTextures_.end())
    {
        return nullptr;
    }
    return &iter->second;
}

void MemoryGame::loadFont()
{
    const std::array<fs::path, 8> candidates{{
        fs::path("assets/fonts/PressStart2P-Regular.ttf"),
        fs::path("assets/fonts/VT323-Regular.ttf"),
        fs::path("assets/fonts/font.ttf"),
        fs::path("/System/Library/Fonts/Supplemental/Arial.ttf"),
        fs::path("/System/Library/Fonts/Supplemental/Arial Unicode.ttf"),
        fs::path("/Library/Fonts/Arial.ttf"),
        fs::path("C:/Windows/Fonts/arial.ttf"),
        fs::path("C:/Windows/Fonts/segoeui.ttf"),
    }};

    for (const fs::path& candidate : candidates)
    {
        if (!fs::exists(candidate))
        {
            continue;
        }
        if (font_.openFromFile(candidate.string()))
        {
            fontLoaded_ = true;
            std::cout << "Loaded font: " << candidate.string() << "\n";
            return;
        }
    }

    std::cerr << "Warning: no usable font found. UI text will not be rendered.\n";
}

void MemoryGame::loadCharacterTextures()
{
    characterTextures_.clear();

    for (int index = 0; index < kPairCount; ++index)
    {
        const CharacterInfo& info = kCharacters[static_cast<std::size_t>(index)];
        const fs::path path = fs::path("assets/processed") / (info.slug + ".png");

        if (!fs::exists(path))
        {
            continue;
        }

        sf::Texture texture;
        if (!texture.loadFromFile(path.string()))
        {
            std::cerr << "Warning: failed to load texture: " << path.string() << "\n";
            continue;
        }

        texture.setSmooth(false);
        characterTextures_.emplace(index, std::move(texture));
    }
}

void MemoryGame::updateCardBoundsFromLayout()
{
    for (int index = 0; index < kCardCount; ++index)
    {
        cards_[static_cast<std::size_t>(index)].bounds = layout_.cardBounds[static_cast<std::size_t>(index)];
    }
}

} // namespace

int main()
{
    MemoryGame game;
    game.run();
    return 0;
}
