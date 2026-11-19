#pragma once

#include <QElapsedTimer>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsView>
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
    explicit GameView(QWidget* parent = nullptr);

    const Player& player() const;

signals:
    void goldChanged(int gold);
    void livesChanged(int lives);
    void enemyReachedEnd();
    void gameOver();
    void gameWon();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void tick();

private:
    struct SlotView {
        QGraphicsEllipseItem* marker = nullptr;
        QGraphicsEllipseItem* range = nullptr;
        QGraphicsRectItem* body = nullptr;
        QGraphicsSimpleTextItem* label = nullptr;
    };

    void drawPath();
    void drawSlots();
    void showTower(std::size_t index);
    void refreshLabel(std::size_t index);
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
    std::unordered_map<const Enemy*, QGraphicsEllipseItem*> m_items;
    std::vector<SlotView> m_slotViews;
    std::vector<QGraphicsEllipseItem*> m_unitItems;
    QTimer m_timer;
    QElapsedTimer m_clock;
};

}