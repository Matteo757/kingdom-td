#ifndef KTD_GAMEVIEW_H
#define KTD_GAMEVIEW_H

#include <QElapsedTimer>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QAction>
#include <QMenu>
#include <QPoint>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <QPixmap>
#include <QPointF>
#include <unordered_map>
#include <vector>
#include "ktd/Game.h"
#include "ktd/RouteBounds.h"

namespace ktd {

class GameView : public QGraphicsView {
    Q_OBJECT

public:
    GameView(Game game, int level, QWidget* parent = nullptr);

    const Player& player() const;
    int totalWaves() const;                       // <-- 1. nieuw

signals:
    void goldChanged(int gold);
    void livesChanged(int lives);
    void waveChanged(int current, int total);     // <-- 2. nieuw
    void enemyReachedEnd();
    void gameOver();
    void gameWon();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void tick();
    void refreshOpenMenu();   // werkt een geopend bouw- of upgrademenu bij zodra het goud verandert

private:
    // Een vlag op de kaart: de wimpel met paal (root), de ring op de grond waar de soldaten verzamelen, en een
    // rood kruis dat alleen bij een ongeldige plek getoond wordt.
    struct FlagView {
        QGraphicsPathItem* root = nullptr;
        QGraphicsEllipseItem* ring = nullptr;
        QGraphicsPathItem* cross = nullptr;
    };

    struct SlotView {
        FlagView flag;   // alleen bij een Barracks
        QGraphicsEllipseItem* marker = nullptr;
        QGraphicsEllipseItem* range = nullptr;
        QGraphicsPixmapItem* body = nullptr;
        QGraphicsEllipseItem* archers[2] = {nullptr, nullptr};   // de twee groene boogschutters op een Archer Tower
        QGraphicsSimpleTextItem* label = nullptr;
    };

    void drawMap(int level);
    void drawPath();
    void drawSlots();
    void showTower(std::size_t index);
    void refreshLabel(std::size_t index);
    void refreshSprite(std::size_t index);
    void updateArchers(std::size_t index);
    void openMenu(std::size_t index, const QPoint& globalPos);
    void build(std::size_t index, TowerType type);
    void upgrade(std::size_t index);
    void addEnemyItem(const SpawnEvent& event);
    void syncProjectiles();
    void loadProjectileSprites();
    void applyEvents(const FrameEvents& events);
    void updateUnits();
    void showEndText(const QString& text);
    void showFireball(const QPointF& center);
    void createWaveButton();
    void updateWaveButton();
    void startNextWave();
    void showBonusText(int bonus);

    // --- Vlag van een Barracks ---
    FlagView makeFlagView(const QColor& color);
    void setFlagColor(const FlagView& view, const QColor& color);
    void createFlagGhost();
    void showFlag(std::size_t index);                    // de vlag van een net gebouwde Barracks
    void beginFlagPlacement(std::size_t slot);           // de vlag volgt nu de muis tot er geklikt wordt
    void endFlagPlacement();
    void updateFlagGhost(const QPointF& scenePos);       // groen en geldig, of rood met kruis en uitleg
    void placeFlagAt(const QPointF& scenePos);
    void flashInvalidFlag(const QPointF& scenePos);

    Game m_game;
    QGraphicsScene* m_scene;
    QGraphicsPathItem* m_pathItem = nullptr;
    QGraphicsPathItem* m_roadHighlight = nullptr;   // toont de weg waarop een vlag mag staan, alleen tijdens het plaatsen
    QGraphicsPathItem* m_boundsItem = nullptr;   // onzichtbare muren langs de weg, alleen zichtbaar met P
    std::unordered_map<const Enemy*, QGraphicsEllipseItem*> m_items;
    std::vector<SlotView> m_slotViews;
    std::vector<QGraphicsEllipseItem*> m_unitItems;

    // De kogels zelf (positie, richting, schade bij inslag) leven in Game::projectiles(); de view tekent alleen
    // een sprite per kogel, gevonden via het id van de kogel.
    std::unordered_map<std::uint64_t, QGraphicsItem*> m_projectileItems;
    std::array<std::array<QPixmap, 3>, 4> m_projectileSprites;   // [soort][stage - 1]

    // Start Wave / Next Wave-knop bij het begin van het pad (een item in de scene, geen widget, zodat hij mee schaalt).
    QGraphicsPathItem* m_waveButton = nullptr;
    QGraphicsSimpleTextItem* m_waveTitle = nullptr;
    QGraphicsSimpleTextItem* m_waveSub = nullptr;
    QGraphicsRectItem* m_waveBar = nullptr;       // toont hoeveel van de bonustijd nog over is
    // Het menu dat nu openstaat (als er een is). De regels worden bij elke goudwijziging opnieuw uit Game afgeleid.
    QPointer<QMenu> m_openMenu;
    std::size_t m_openMenuSlot = 0;
    std::vector<QAction*> m_openMenuActions;

    // Plaatsen van een vlag: de bouwplek van de Barracks waarvan de vlag nu verplaatst wordt (als dat gebeurt).
    std::optional<std::size_t> m_flagSlot;
    FlagView m_ghostFlag;                         // de vlag die met de muis meebeweegt
    QGraphicsRectItem* m_flagHint = nullptr;      // "Alleen op de weg!" naast de muis bij een ongeldige plek
    QGraphicsSimpleTextItem* m_flagHintText = nullptr;

    QTimer m_timer;
    QElapsedTimer m_clock;
    std::size_t m_shownWave = 0;
};

}

#endif  // KTD_GAMEVIEW_H
