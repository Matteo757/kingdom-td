#include "ktd/GameView.h"

#include <QFont>
#include <QGraphicsTextItem>
#include <QPainterPath>
#include <QPen>
#include <algorithm>

namespace ktd {

namespace {

QColor colorFor(EnemyType type)
{
    switch (type) {
    case EnemyType::Goblin:   return QColor(80, 200, 80);
    case EnemyType::Skeleton: return QColor(230, 230, 210);
    case EnemyType::Orc:      return QColor(30, 100, 40);
    case EnemyType::DarkMage: return QColor(90, 30, 130);
    }
    return Qt::red;
}

}

GameView::GameView(QWidget* parent)
    : QGraphicsView(parent)
    , m_game(Game::defaultLevel())
    , m_scene(new QGraphicsScene(this))
{
    m_scene->setSceneRect(0, 0, 1024, 576);
    m_scene->setBackgroundBrush(QColor(90, 140, 70));
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    drawPath();
    for (const auto& tower : m_game.towers()) {
        drawTower(*tower);
    }

    connect(&m_timer, &QTimer::timeout, this, &GameView::tick);
    m_timer.start(16);
    m_clock.start();
}

const Player& GameView::player() const
{
    return m_game.player();
}

void GameView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

void GameView::drawPath()
{
    const std::vector<Vec2>& points = m_game.path().waypoints();
    QPainterPath line(QPointF(points.front().x, points.front().y));
    for (std::size_t i = 1; i < points.size(); ++i) {
        line.lineTo(points[i].x, points[i].y);
    }
    m_scene->addPath(line, QPen(QColor(150, 110, 60), 28, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
}

void GameView::drawTower(const Tower& tower)
{
    QColor color;
    switch (tower.type()) {
    case TowerType::Archer:    color = QColor(50, 100, 255); break;
    case TowerType::Cannon:    color = QColor(60, 60, 60); break;
    case TowerType::Mage:      color = QColor(150, 0, 200); break;
    case TowerType::Barracks:  color = QColor(200, 120, 40); break;
    case TowerType::Artillery: color = QColor(180, 40, 40); break;
    }
    const Vec2& pos = tower.position();
    const double r = tower.range();
    m_scene->addEllipse(pos.x - r, pos.y - r, 2 * r, 2 * r,
                        QPen(QColor(255, 255, 255, 90)), QBrush(QColor(255, 255, 255, 30)));
    m_scene->addRect(pos.x - 14, pos.y - 14, 28, 28, QPen(Qt::black), QBrush(color));
}

void GameView::updateUnits()
{
    std::vector<Vec2> positions;
    for (const auto& tower : m_game.towers()) {
        const std::vector<Vec2> units = tower->unitPositions();
        positions.insert(positions.end(), units.begin(), units.end());
    }
    while (m_unitItems.size() > positions.size()) {
        delete m_unitItems.back();
        m_unitItems.pop_back();
    }
    while (m_unitItems.size() < positions.size()) {
        QGraphicsEllipseItem* item = m_scene->addEllipse(-6, -6, 12, 12, QPen(Qt::black), QBrush(QColor(230, 170, 40)));
        item->setZValue(5);
        m_unitItems.push_back(item);
    }
    for (std::size_t i = 0; i < positions.size(); ++i) {
        m_unitItems[i]->setPos(positions[i].x, positions[i].y);
    }
}

void GameView::addEnemyItem(const SpawnEvent& event)
{
    const QColor color = colorFor(event.type);
    QGraphicsEllipseItem* item = m_scene->addEllipse(-10, -10, 20, 20, QPen(Qt::black), QBrush(color));
    const Vec2 start = m_game.path().pointAt(0);
    item->setPos(start.x, start.y);
    m_items[event.enemy] = item;
}

void GameView::showShot(const Shot& shot)
{
    const QColor color = shot.type == DamageType::Magic ? QColor(220, 80, 255) : QColor(Qt::white);
    QGraphicsLineItem* line = m_scene->addLine(shot.from.x, shot.from.y, shot.to.x, shot.to.y, QPen(color, 3));
    line->setZValue(10);
    QTimer::singleShot(80, this, [line]() { delete line; });
}

void GameView::applyEvents(const FrameEvents& events)
{
    for (const SpawnEvent& spawn : events.spawned) {
        addEnemyItem(spawn);
    }
    for (const Shot& shot : events.shots) {
        showShot(shot);
    }
    for (const Enemy* enemy : events.removed) {
        delete m_items[enemy];
        m_items.erase(enemy);
    }
    if (events.goldEarned > 0) {
        emit goldChanged(m_game.player().gold());
    }
    if (events.livesLost > 0) {
        emit livesChanged(m_game.player().lives());
        for (int i = 0; i < events.livesLost; ++i) {
            emit enemyReachedEnd();
        }
    }
}

void GameView::showEndText(const QString& text)
{
    QGraphicsTextItem* item = m_scene->addText(text, QFont("Arial", 48, QFont::Bold));
    item->setDefaultTextColor(Qt::white);
    item->setPos(300, 230);
    item->setZValue(20);
}

void GameView::tick()
{
    // Begrens dt, zodat het spel na het slepen van het venster niet in één keer doorspringt.
    const double dt = std::min(m_clock.restart() / 1000.0, 0.1);

    applyEvents(m_game.update(dt));

    for (const auto& enemy : m_game.enemies()) {
        const Vec2 p = m_game.path().pointAt(enemy->distance());
        m_items.at(enemy.get())->setPos(p.x, p.y);
    }
    updateUnits();

    if (m_game.isGameOver()) {
        m_timer.stop();
        showEndText("GAME OVER");
        emit gameOver();
    } else if (m_game.isWon()) {
        m_timer.stop();
        showEndText("VICTORY");
        emit gameWon();
    }
}

}