#ifndef KTD_GAMEVIEW_H
#define KTD_GAMEVIEW_H

#include <QElapsedTimer>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QPoint>
#include <QString>
#include <QTimer>
#include <array>
#include <cstddef>
#include <cstdint>
#include <QPixmap>
#include <QPointF>
#include <unordered_map>
#include <vector>
#include "ktd/Game.h"

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
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void tick();

private:
    struct SlotView {
        QGraphicsEllipseItem* marker = nullptr;
        QGraphicsEllipseItem* range = nullptr;
        QGraphicsPixmapItem* body = nullptr;
        QGraphicsSimpleTextItem* label = nullptr;
    };

    void drawMap(int level);
    void drawPath();
    void drawSlots();
    void showTower(std::size_t index);
    void refreshLabel(std::size_t index);
    void refreshSprite(std::size_t index);
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

    Game m_game;
    QGraphicsScene* m_scene;
    QGraphicsPathItem* m_pathItem = nullptr;
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
    QTimer m_timer;
    QElapsedTimer m_clock;
    std::size_t m_shownWave = 0;
};

}

#endif  // KTD_GAMEVIEW_H
