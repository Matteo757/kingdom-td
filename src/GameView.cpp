#include "ktd/GameView.h"

#include <QAction>
#include <QDebug>
#include <utility>
#include <QFont>
#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <QMenu>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <algorithm>
#include <cmath>
#include "ktd/Prices.h"
#include "ktd/TowerFactory.h"

namespace ktd {

namespace {

constexpr double kSlotRadius = 22.0;
constexpr double kSpriteWidth = 90.0;  // standaardbreedte van een toren in sceneruimte (groter = toren groter)
constexpr double kSpriteDrop = 18.0;    // hoeveel px de onderkant van de toren onder het midden van de bouwplek ligt (groter = lager)
constexpr int kSpriteSupersample = 2;   // sprite wordt 2x zo groot voorgeschaald voor een scherper beeld

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

QString spriteName(TowerType type)
{
    switch (type) {
    case TowerType::Archer:    return "Archer-Tower";
    case TowerType::Cannon:    return "Cannon-Tower";
    case TowerType::Mage:      return "Mage-Tower";
    case TowerType::Barracks:  return "Barracks";
    case TowerType::Artillery: return "Artillery-Tower";
    }
    return QString();
}

// Schaal per toren: 1.0 = standaardbreedte. Pas een getal aan om één soort toren groter of kleiner te maken.
double spriteScale(TowerType type)
{
    switch (type) {
    case TowerType::Archer:    return 0.8;
    case TowerType::Cannon:    return 0.8;
    case TowerType::Mage:      return 0.8;
    case TowerType::Barracks:  return 0.8;
    case TowerType::Artillery: return 0.8;
    }
    return 1.0;
}

// Bestanden heten bv. Archer-TowerLVL1.png. Ontbreekt een sprite voor een hoger level,
// dan wordt de sprite van het dichtstbijzijnde lagere level gebruikt.
QPixmap towerSprite(TowerType type, int level)
{
    for (int l = level; l >= 1; --l) {
        const QPixmap pixmap(QString(":/assets/buildings/%1LVL%2.png").arg(spriteName(type)).arg(l));
        if (!pixmap.isNull()) {
            return pixmap;
        }
    }
    return QPixmap();
}

// Welke kogel hoort bij welke toren, en hoe hij vliegt. De sprites wijzen naar rechts; de kop (waar de kogel
// raakt) zit rechts. 'headInset' = pixels van de rechterrand tot het zwaartepunt van de kop (bij een pijl de punt,
// bij een bol het midden).
struct ProjectileStyle {
    int kind;                // index in m_projectileSprites
    const char* name;
    double scale;            // sprites zijn groot (ca. 200 px), de scene is 1024 px breed
    double headInset;
    double speed;            // px per seconde langs de baan (sceneruimte)
    double arc;              // boogtop als fractie van de afstand tot het doel (0 = rechte lijn)
    double minArc;           // minimale boogtop in px, zodat ook een korte worp een zichtbare boog heeft
};

// Elke kogel heeft zijn eigen natuurkunde:
//  - pijl:       gemiddelde snelheid, lage boog;
//  - kanonskogel: rechte lijn, gemiddelde snelheid, voelt zwaar;
//  - magiebol:   rechte lijn, veruit het snelst;
//  - raket:      hoge, duidelijke boog en merkbaar langzamer.
bool projectileStyle(TowerType type, ProjectileStyle& out)
{
    switch (type) {
    case TowerType::Archer:    out = {0, "Arrow",      0.17, 0.0,   325.0, 0.12, 10.0}; return true;
    case TowerType::Mage:      out = {1, "MageBall",   0.15, 65.0,  750.0, 0.0,   0.0}; return true;
    case TowerType::Cannon:    out = {2, "Cannonball", 0.20, 57.0,  190.0, 0.0,   0.0}; return true;
    case TowerType::Artillery: out = {3, "Rocket",     0.17, 0.0,   190.0, 0.45, 60.0}; return true;
    case TowerType::Barracks:  return false;   // soldaten vechten zelf, er is geen kogel
    }
    return false;
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

GameView::GameView(Game game, int level, QWidget* parent)
    : QGraphicsView(parent)
    , m_game(std::move(game))
    , m_scene(new QGraphicsScene(this))
{
    m_scene->setSceneRect(0, 0, 1024, 576);
    m_scene->setBackgroundBrush(QColor(90, 140, 70));
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    drawMap(level);
    drawPath();
    drawSlots();
    loadProjectileSprites();

    connect(&m_timer, &QTimer::timeout, this, &GameView::tick);
    m_timer.start(16);
    m_clock.start();
}

int GameView::totalWaves() const
{
    return static_cast<int>(m_game.waves().waveCount());
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

void GameView::keyPressEvent(QKeyEvent* event)
{
    // P toont of verbergt de route (handig om de waypoints met de kaart te vergelijken).
    if (event->key() == Qt::Key_P && m_pathItem != nullptr) {
        m_pathItem->setVisible(!m_pathItem->isVisible());
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

void GameView::drawMap(int level)
{
    const QString file = QString(":/assets/maps/level%1.png").arg(level);
    const QPixmap map(file);
    if (map.isNull()) {
        qWarning() << "Kaart niet gevonden:" << file;
        return;
    }
    QGraphicsPixmapItem* background = m_scene->addPixmap(map);
    background->setTransformationMode(Qt::SmoothTransformation);
    background->setScale(m_scene->width() / map.width());
    background->setZValue(-10);
}

void GameView::drawPath()
{
    const std::vector<Vec2>& points = m_game.path().waypoints();
    QPainterPath line(QPointF(points.front().x, points.front().y));
    for (std::size_t i = 1; i < points.size(); ++i) {
        line.lineTo(points[i].x, points[i].y);
    }
    m_pathItem = m_scene->addPath(line, QPen(QColor(255, 0, 255, 180), 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    m_pathItem->setZValue(-2);
    m_pathItem->setVisible(false);
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
    view.body = m_scene->addPixmap(QPixmap());
    view.body->setTransformationMode(Qt::SmoothTransformation);
    view.body->setPos(pos.x, pos.y);
    view.body->setZValue(2 + pos.y / 1000.0);  // torens lager op het scherm staan voor torens erboven
    view.label = m_scene->addSimpleText("");
    view.label->setBrush(Qt::white);
    view.label->setPos(pos.x - 14, pos.y + kSpriteDrop);
    view.label->setZValue(3);
    refreshSprite(index);
    refreshLabel(index);
}

void GameView::refreshSprite(std::size_t index)
{
    const Tower& tower = *m_game.buildSlots()[index].tower;
    QGraphicsPixmapItem* body = m_slotViews[index].body;

    QPixmap sprite = towerSprite(tower.type(), tower.level());
    if (sprite.isNull()) {
        // Geen sprite gevonden: vervang door een gekleurd vlak zodat de toren zichtbaar blijft.
        sprite = QPixmap(100, 100);
        sprite.fill(colorFor(tower.type()));
    }
    const double width = kSpriteWidth * spriteScale(tower.type());
    sprite = sprite.scaledToWidth(int(width * kSpriteSupersample), Qt::SmoothTransformation);

    // De onderkant van de sprite ligt kSpriteDrop px onder het midden van de bouwplek,
    // zodat de grasvoet de hele zandcirkel van de kaart bedekt.
    body->setPixmap(sprite);
    body->setScale(1.0 / kSpriteSupersample);
    body->setOffset(-sprite.width() / 2.0, kSpriteDrop * kSpriteSupersample - sprite.height());
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
    refreshSprite(index);
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

void GameView::loadProjectileSprites()
{
    for (TowerType type : kBuildable) {
        ProjectileStyle style;
        if (!projectileStyle(type, style)) {
            continue;
        }
        for (int stage = 0; stage < 3; ++stage) {
            m_projectileSprites[style.kind][stage] = QPixmap(QString(":/assets/projectiles/%1%2.png").arg(style.name).arg(stage + 1));
        }
    }
}

void GameView::showShot(const Shot& shot)
{
    ProjectileStyle style;
    if (!projectileStyle(shot.tower, style)) {
        return;
    }
    const int stage = std::clamp(int(shot.level), 1, 3) - 1;
    const QPixmap& pixmap = m_projectileSprites[style.kind][stage];

    FlyingShot flying;
    flying.from = QPointF(shot.from.x, shot.from.y);
    flying.to = QPointF(shot.to.x, shot.to.y);
    flying.target = shot.target;
    const double distance = std::hypot(flying.to.x() - flying.from.x(), flying.to.y() - flying.from.y());
    flying.arcHeight = style.arc > 0.0 ? std::max(style.minArc, style.arc * distance) : 0.0;
    // Een boog is langer dan de rechte lijn; de lengte van een vlakke parabool is bij benadering d * (1 + 8/3 * (h/d)^2).
    // Zo hangt de vliegtijd van de boogtop en de snelheid af: de hoge raket doet er echt langer over.
    const double ratio = distance > 1.0 ? flying.arcHeight / distance : 0.0;
    const double pathLength = distance * (1.0 + (8.0 / 3.0) * ratio * ratio);
    flying.duration = std::max(0.05, pathLength / style.speed);

    if (pixmap.isNull()) {
        // Sprite ontbreekt: val terug op een korte lijn zodat de aanval zichtbaar blijft.
        const QColor color = shot.type == DamageType::Magic ? QColor(220, 80, 255) : QColor(Qt::white);
        QGraphicsLineItem* line = m_scene->addLine(shot.from.x, shot.from.y, shot.to.x, shot.to.y, QPen(color, 3));
        line->setZValue(10);
        QTimer::singleShot(80, this, [line]() { delete line; });
        return;
    }

    QGraphicsPixmapItem* item = m_scene->addPixmap(pixmap);
    item->setTransformationMode(Qt::SmoothTransformation);
    // Het item draait en schaalt rond zijn positie; die ligt op de kop van de kogel.
    item->setOffset(-(pixmap.width() - style.headInset), -pixmap.height() / 2.0);
    item->setScale(style.scale);
    item->setZValue(10);
    item->setPos(flying.from);
    flying.item = item;
    m_flyingShots.push_back(flying);
}

void GameView::updateShots(double dt)
{
    for (auto it = m_flyingShots.begin(); it != m_flyingShots.end();) {
        it->elapsed += dt;
        // Volg de vijand zolang hij er nog is; anders vlieg door naar de laatste bekende plek.
        const auto enemyItem = m_items.find(it->target);
        if (enemyItem != m_items.end()) {
            it->to = enemyItem->second->pos();
        }
        const double t = std::min(it->elapsed / it->duration, 1.0);
        const QPointF line = it->to - it->from;

        // Positie = rechte lijn + een parabool omhoog (schermcoordinaten: omhoog = negatieve y).
        // De boog is 0 aan begin en eind en maximaal (arcHeight) halverwege: 4 * t * (1 - t).
        const double lift = 4.0 * it->arcHeight * t * (1.0 - t);
        const QPointF pos = it->from + line * t + QPointF(0.0, -lift);

        // De richting van de kogel is de afgeleide van de baan: de rechte lijn plus de helling van de parabool.
        // Daardoor kantelt een pijl of raket omhoog bij het wegschieten en omlaag bij de inslag.
        const double liftSlope = 4.0 * it->arcHeight * (1.0 - 2.0 * t);
        const QPointF velocity(line.x(), line.y() - liftSlope);
        if (std::hypot(velocity.x(), velocity.y()) > 1e-6) {
            it->item->setRotation(std::atan2(velocity.y(), velocity.x()) * 180.0 / 3.14159265358979323846);
        }
        it->item->setPos(pos);

        if (t >= 1.0) {
            delete it->item;
            it = m_flyingShots.erase(it);
        } else {
            ++it;
        }
    }
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
    updateShots(dt);

    const std::size_t wave = m_game.waves().currentWave();
    if (wave != m_shownWave) {
        m_shownWave = wave;
        emit waveChanged(static_cast<int>(wave), totalWaves());
    }

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