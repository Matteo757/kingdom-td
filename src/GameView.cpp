#include "ktd/GameView.h"

#include <QAction>
#include <QFont>
#include <QGraphicsTextItem>
#include <QMenu>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPen>
#include <algorithm>
#include <cmath>
#include "ktd/Prices.h"
#include "ktd/TowerFactory.h"

namespace ktd {

namespace {

constexpr double kSlotRadius = 22.0;

constexpr TowerType kBuildable[] = {
    TowerType::Archer, TowerType::Cannon, TowerType::Mage, TowerType::Barracks, TowerType::Artillery
};

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

QColor colorFor(TowerType type)
{
    switch (type) {
    case TowerType::Archer:    return QColor(50, 100, 255);
    case TowerType::Cannon:    return QColor(60, 60, 60);
    case TowerType::Mage:      return QColor(150, 0, 200);
    case TowerType::Barracks:  return QColor(200, 120, 40);
    case TowerType::Artillery: return QColor(180, 40, 40);
    }
    return Qt::gray;
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
    drawSlots();

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

void GameView::mousePressEvent(QMouseEvent* event)
{
    if (!m_timer.isActive()) {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    const QPointF scenePos = mapToScene(event->position().toPoint());

    if (event->button() == Qt::RightButton) {
        if (m_game.castFireball({scenePos.x(), scenePos.y()})) {
            showFireball(scenePos);
        }
        return;
    }

    if (event->button() == Qt::LeftButton) {
        const std::vector<BuildSlot>& allSlots = m_game.buildSlots();
        for (std::size_t i = 0; i < allSlots.size(); ++i) {
            const Vec2& p = allSlots[i].position;
            if (std::hypot(scenePos.x() - p.x, scenePos.y() - p.y) <= kSlotRadius) {
                openMenu(i, event->globalPosition().toPoint());
                return;
            }
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void GameView::drawPath()
{
    const std::vector<Vec2>& points = m_game.path().waypoints();
    QPainterPath line(QPointF(points.front().x, points.front().y));
    for (std::size_t i = 1; i < points.size(); ++i) {
        line.lineTo(points[i].x, points[i].y);
    }
    QGraphicsPathItem* path = m_scene->addPath(line, QPen(QColor(150, 110, 60), 28, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    path->setZValue(-2);
}

void GameView::drawSlots()
{
    for (const BuildSlot& slot : m_game.buildSlots()) {
        SlotView view;
        view.marker = m_scene->addEllipse(slot.position.x - 18, slot.position.y - 18, 36, 36,
                                          QPen(QColor(255, 255, 255, 220), 2, Qt::DashLine),
                                          QBrush(QColor(0, 0, 0, 70)));
        view.marker->setZValue(1);
        m_slotViews.push_back(view);
    }
}

void GameView::showTower(std::size_t index)
{
    const Tower& tower = *m_game.buildSlots()[index].tower;
    SlotView& view = m_slotViews[index];
    const Vec2& pos = tower.position();
    const double r = tower.range();

    view.marker->setVisible(false);
    view.range = m_scene->addEllipse(pos.x - r, pos.y - r, 2 * r, 2 * r,
                                     QPen(QColor(255, 255, 255, 90)), QBrush(QColor(255, 255, 255, 30)));
    view.range->setZValue(-1);
    view.body = m_scene->addRect(pos.x - 14, pos.y - 14, 28, 28, QPen(Qt::black), QBrush(colorFor(tower.type())));
    view.body->setZValue(2);
    view.label = m_scene->addSimpleText("");
    view.label->setBrush(Qt::white);
    view.label->setPos(pos.x - 14, pos.y + 16);
    view.label->setZValue(3);
    refreshLabel(index);
}

void GameView::refreshLabel(std::size_t index)
{
    const Tower& tower = *m_game.buildSlots()[index].tower;
    m_slotViews[index].label->setText(QString("Lv %1").arg(int(tower.level())));
}

void GameView::openMenu(std::size_t index, const QPoint& globalPos)
{
    QMenu menu(this);
    const Tower* tower = m_game.buildSlots()[index].tower;

    if (tower == nullptr) {
        for (TowerType type : kBuildable) {
            const QString text = QString("%1 (%2 goud)")
                .arg(QString::fromStdString(towerName(type)))
                .arg(buildCost(type));
            QAction* action = menu.addAction(text);
            action->setEnabled(m_game.canBuild(index, type));
            connect(action, &QAction::triggered, this, [this, index, type]() { build(index, type); });
        }
    } else {
        QString text = "Maximaal level";
        if (tower->level() < Tower::kMaxLevel) {
            text = QString("Upgrade naar level %1 (%2 goud)")
                .arg(int(tower->level()) + 1)
                .arg(upgradeCost(tower->level()));
        }
        QAction* action = menu.addAction(text);
        action->setEnabled(m_game.canUpgrade(index));
        connect(action, &QAction::triggered, this, [this, index]() { upgrade(index); });
    }
    menu.exec(globalPos);
}

void GameView::build(std::size_t index, TowerType type)
{
    if (!m_game.buildTower(index, type)) {
        return;
    }
    showTower(index);
    emit goldChanged(m_game.player().gold());
}

void GameView::upgrade(std::size_t index)
{
    if (!m_game.upgradeTower(index)) {
        return;
    }
    refreshLabel(index);
    emit goldChanged(m_game.player().gold());
}

void GameView::addEnemyItem(const SpawnEvent& event)
{
    const QColor color = colorFor(event.type);
    QGraphicsEllipseItem* item = m_scene->addEllipse(-10, -10, 20, 20, QPen(Qt::black), QBrush(color));
    item->setZValue(4);
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

void GameView::showFireball(const QPointF& center)
{
    const double r = Fireball::kRadius;
    QGraphicsEllipseItem* blast = m_scene->addEllipse(center.x() - r, center.y() - r, 2 * r, 2 * r,
                                                      QPen(QColor(255, 160, 0), 3), QBrush(QColor(255, 80, 0, 110)));
    blast->setZValue(15);
    QTimer::singleShot(250, this, [blast]() { delete blast; });
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

void GameView::showEndText(const QString& text)
{
    QGraphicsTextItem* item = m_scene->addText(text, QFont("Arial", 48, QFont::Bold));
    item->setDefaultTextColor(Qt::white);
    item->setPos(300, 230);
    item->setZValue(20);
}

void GameView::tick()
{
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