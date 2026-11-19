#pragma once

#include <QElapsedTimer>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include <memory>
#include "ktd/Enemy.h"
#include "ktd/Path.h"

namespace ktd {

class GameView : public QGraphicsView {
    Q_OBJECT

public:
    explicit GameView(QWidget* parent = nullptr);

private slots:
    void tick();

private:
    void spawnEnemy();

    Path m_path;
    QGraphicsScene* m_scene;
    QGraphicsEllipseItem* m_enemyItem;
    std::unique_ptr<Enemy> m_enemy;
    QTimer m_timer;
    QElapsedTimer m_clock;
};

}