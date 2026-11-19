#include "ktd/GameView.h"

#include <QPainterPath>
#include <QPen>
#include "ktd/Grunt.h"

namespace ktd {

GameView::GameView(QWidget* parent)
    : QGraphicsView(parent)
    , m_path(std::vector<Vec2>{{100, 100}, {400, 100}, {400, 300}, {800, 300}, {800, 480}})
    , m_scene(new QGraphicsScene(this))
    , m_enemyItem(nullptr)
{
    m_scene->setSceneRect(0, 0, 1024, 576);
    m_scene->setBackgroundBrush(QColor(90, 140, 70));
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);

    const std::vector<Vec2>& points = m_path.waypoints();
    QPainterPath line(QPointF(points.front().x, points.front().y));
    for (std::size_t i = 1; i < points.size(); ++i) {
        line.lineTo(points[i].x, points[i].y);
    }
    m_scene->addPath(line, QPen(QColor(150, 110, 60), 28, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    m_enemyItem = m_scene->addEllipse(-10, -10, 20, 20, QPen(Qt::black), QBrush(Qt::red));
    spawnEnemy();

    connect(&m_timer, &QTimer::timeout, this, &GameView::tick);
    m_timer.start(16);
    m_clock.start();
}

void GameView::spawnEnemy()
{
    m_enemy = std::make_unique<Grunt>();
}

void GameView::tick()
{
    const double dt = m_clock.restart() / 1000.0;
    m_enemy->update(dt);
    if (m_enemy->distance() >= m_path.length()) {
        spawnEnemy();
    }
    const Vec2 p = m_path.pointAt(m_enemy->distance());
    m_enemyItem->setPos(p.x, p.y);
}

}