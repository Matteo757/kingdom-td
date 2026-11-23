#pragma once

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
#include <cstddef>
#include <unordered_map>
#include <vector>
#include "ktd/Game.h"

namespace ktd {

class GameView : public QGraphicsView {
    Q_OBJECT

public:
    explicit GameView(int level, QWidget* parent = nullptr);

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
    void showShot(const Shot& shot);
    void applyEvents(const FrameEvents& events);
    void updateUnits();
    void showEndText(const QString& text);
    void showFireball(const QPointF& center);

    Game m_game;
    QGraphicsScene* m_scene;
    QGraphicsPathItem* m_pathItem = nullptr;
    std::unordered_map<const Enemy*, QGraphicsEllipseItem*> m_items;
    std::vector<SlotView> m_slotViews;
    std::vector<QGraphicsEllipseItem*> m_unitItems;
    QTimer m_timer;
    QElapsedTimer m_clock;
    std::size_t m_shownWave = 0;
};

}