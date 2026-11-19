#pragma once

#include <QElapsedTimer>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QString>
#include <QTimer>
#include <vector>
#include <unordered_map>
#include "ktd/Game.h"

namespace ktd {

// De view tekent alleen nog en stuurt signals uit; alle spelregels zitten in Game.
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

private slots:
    void tick();

private:
    void drawPath();
    void drawTower(const Tower& tower);
    void addEnemyItem(const SpawnEvent& event);
    void showShot(const Shot& shot);
    void applyEvents(const FrameEvents& events);
    void showEndText(const QString& text);
    void updateUnits();                                   // <-- nieuwe functie

    Game m_game;
    QGraphicsScene* m_scene;
    std::unordered_map<const Enemy*, QGraphicsEllipseItem*> m_items;
    std::vector<QGraphicsEllipseItem*> m_unitItems;       // <-- nieuw lid
    QTimer m_timer;
    QElapsedTimer m_clock;
};

}