#include "ktd/GameView.h"

#include <QAction>
#include <QCursor>
#include <QDebug>
#include <utility>
#include <QFont>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <QMenu>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <algorithm>
#include <cmath>
#include <optional>
#include <unordered_set>
#include "ktd/LaunchPoints.h"
#include "ktd/Prices.h"
#include "ktd/SlotMenu.h"
#include "ktd/TowerFactory.h"

namespace ktd {

namespace {

constexpr double kSlotRadius = 22.0;
constexpr double kSpriteWidth = 90.0;  // standaardbreedte van een toren in sceneruimte (groter = toren groter)
constexpr double kSpriteDrop = 18.0;    // hoeveel px de onderkant van de toren onder het midden van de bouwplek ligt (groter = lager)
constexpr double kWaveButtonWidth = 132.0;
constexpr double kWaveButtonHeight = 50.0;
constexpr double kWaveButtonPathOffset = 70.0;   // px langs het pad vanaf het begin
constexpr double kWaveButtonLift = 55.0;         // px boven het pad, zodat de eerste vijanden zichtbaar blijven
constexpr double kFlagPoleHeight = 30.0;      // hoogte van de vlagpaal in sceneruimte
constexpr double kFlagBannerWidth = 20.0;
constexpr double kFlagGrabWidth = 22.0;       // zo dicht bij een vlag klikken pakt hem op om te verplaatsen
const QColor kPlacedFlagColor(40, 110, 235);  // blauw: de kleur van jouw kant (het blauwe kasteel)
const QColor kValidFlagColor(60, 200, 80);    // groen: hier mag de vlag staan
const QColor kInvalidFlagColor(230, 40, 40);  // rood: hier niet
constexpr int kSpriteSupersample = 2;   // sprite wordt 2x zo groot voorgeschaald voor een scherper beeld

// ---- De twee groene boogschutters (stippen) op de Archer Tower ----
// Waar de stippen staan (en dus waar de pijlen vandaan komen) staat in include/ktd/LaunchPoints.h.
constexpr double kArcherDotRadius = 3.8;   // in sceneruimte

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

// Hoe de sprite van een kogel getekend wordt. De sprites wijzen naar rechts; de kop (waar de kogel raakt) zit
// rechts. 'headInset' = pixels van de rechterrand tot het zwaartepunt van de kop (bij een pijl de punt, bij een bol
// het midden). Snelheid en boog staan niet hier maar in flightProfile() (Projectile.h), zodat de schade valt op het
// moment dat de sprite inslaat.
struct ProjectileStyle {
    int kind;                // index in m_projectileSprites
    const char* name;
    double scale;            // sprites zijn groot (ca. 200 px), de scene is 1024 px breed
    double headInset;
};

bool projectileStyle(TowerType type, ProjectileStyle& out)
{
    switch (type) {
    case TowerType::Archer:    out = {0, "Arrow",      0.17, 0.0};  return true;
    case TowerType::Mage:      out = {1, "MageBall",   0.15, 65.0}; return true;
    case TowerType::Cannon:    out = {2, "Cannonball", 0.20, 57.0}; return true;
    case TowerType::Artillery: out = {3, "Rocket",     0.17, 0.0};  return true;
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
    createWaveButton();
    createPlacementRange();
    createFlagGhost();
    setMouseTracking(true);              // de vlag moet meebewegen zonder dat er een knop ingedrukt is
    viewport()->setMouseTracking(true);

    connect(&m_timer, &QTimer::timeout, this, &GameView::tick);
    connect(this, &GameView::goldChanged, this, &GameView::refreshOpenMenu);
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

    // Tijdens het plaatsen van een vlag: links plaatst, rechts annuleert, al het andere wordt genegeerd.
    if (m_flagSlot.has_value()) {
        if (event->button() == Qt::LeftButton) {
            placeFlagAt(scenePos);
        } else if (event->button() == Qt::RightButton) {
            endFlagPlacement();
        }
        return;
    }

    if (event->button() == Qt::RightButton) {
        if (m_game.castFireball({scenePos.x(), scenePos.y()})) {
            showFireball(scenePos);
        }
        return;
    }

    if (event->button() == Qt::LeftButton) {
        if (m_waveButton != nullptr && m_waveButton->isVisible()
            && m_waveButton->contains(m_waveButton->mapFromScene(scenePos))) {
            startNextWave();
            return;
        }
        // Op een bestaande vlag klikken pakt hem op, zodat hij verplaatst kan worden.
        for (std::size_t i = 0; i < m_slotViews.size(); ++i) {
            const FlagView& flag = m_slotViews[i].flag;
            if (flag.root == nullptr) {
                continue;
            }
            const QPointF base = flag.root->pos();
            if (std::abs(scenePos.x() - base.x()) <= kFlagGrabWidth
                && scenePos.y() >= base.y() - kFlagPoleHeight - 6.0 && scenePos.y() <= base.y() + 10.0) {
                beginFlagPlacement(i);
                return;
            }
        }
        const std::optional<std::size_t> hit = slotAt(scenePos);
        if (hit.has_value()) {
            openMenu(*hit, event->globalPosition().toPoint());
            return;
        }
        // Een klik op een lege plek deselecteert: de bereikzone van de toren verdwijnt weer.
        m_ranges.clearSelection();
        applyRangeDisplay();
    }
    QGraphicsView::mousePressEvent(event);
}

void GameView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_flagSlot.has_value()) {
        updateFlagGhost(mapToScene(event->position().toPoint()));
    }
    QGraphicsView::mouseMoveEvent(event);
}

void GameView::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape && m_flagSlot.has_value()) {
        endFlagPlacement();   // Esc annuleert het plaatsen van de vlag
        return;
    }
    if (event->key() == Qt::Key_P && m_pathItem != nullptr) {
        m_pathItem->setVisible(!m_pathItem->isVisible());
        if (m_boundsItem != nullptr) {
            m_boundsItem->setVisible(m_pathItem->isVisible());   // de muren tonen we samen met de route
        }
        return;
    }
    if (event->key() == Qt::Key_Escape && m_ranges.selected().has_value()) {
        m_ranges.clearSelection();   // Esc deselecteert de toren
        applyRangeDisplay();
        return;
    }
    if (event->key() == Qt::Key_Space) {
        togglePause();
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

// Zet het spel op pauze of hervat het. Alle tijd loopt via Game::update(), dus de wavetimer staat ook stil.
void GameView::togglePause()
{
    if (!m_timer.isActive()) {
        return;   // na Game Over of Victory valt er niets meer te pauzeren
    }
    m_game.setPaused(!m_game.isPaused());

    if (m_pauseText == nullptr) {
        m_pauseText = m_scene->addSimpleText("PAUZE", QFont("Arial", 48, QFont::Bold));
        m_pauseText->setBrush(Qt::white);
        m_pauseText->setPen(QPen(Qt::black, 2));
        m_pauseText->setZValue(20);
        const QRectF box = m_pauseText->boundingRect();
        m_pauseText->setPos(m_scene->width() / 2 - box.width() / 2, m_scene->height() / 2 - box.height() / 2);
    }
    m_pauseText->setVisible(m_game.isPaused());
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
    // De onzichtbare muren zijn alleen logica (RouteBounds). Dit item tekent ze enkel voor controle:
    // een brede streep met dezelfde breedte als de begaanbare weg, standaard verborgen.
    m_boundsItem = m_scene->addPath(line, QPen(QColor(0, 200, 255, 70), 2 * kRoadHalfWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    m_boundsItem->setZValue(-3);
    m_boundsItem->setVisible(false);
    // Tijdens het plaatsen van een vlag tonen we de weg waarop hij mag staan (dezelfde breedte als de muren).
    m_roadHighlight = m_scene->addPath(line, QPen(QColor(60, 220, 90, 80), 2 * kRoadHalfWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    m_roadHighlight->setZValue(-2.5);
    m_roadHighlight->setVisible(false);
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

void GameView::createPlacementRange()
{
    // Zelfde uiterlijk als de bereikzone van een toren, maar blauw en gestippeld: het is een voorbeeld, nog geen toren.
    m_placementRange = m_scene->addEllipse(0, 0, 0, 0, QPen(QColor(120, 200, 255, 190), 2, Qt::DashLine),
                                           QBrush(QColor(120, 200, 255, 40)));
    m_placementRange->setZValue(-1);
    m_placementRange->setVisible(false);
}

// De enige plek die bereikzones laat verschijnen of verdwijnen. RangeSelection bepaalt welke zone er is (nooit meer
// dan één, met de actuele straal van de toren); hier wordt alles eerst verborgen en daarna alleen die zone getoond.
void GameView::applyRangeDisplay()
{
    for (SlotView& view : m_slotViews) {
        if (view.range != nullptr) {
            view.range->setVisible(false);
        }
    }
    m_placementRange->setVisible(false);

    const std::optional<RangeZone> zone = m_ranges.visibleZone(m_game);
    if (!zone.has_value()) {
        return;
    }
    const QRectF rect(zone->center.x - zone->radius, zone->center.y - zone->radius, 2 * zone->radius, 2 * zone->radius);
    if (zone->kind == RangeZone::Kind::Selected) {
        QGraphicsEllipseItem* range = m_slotViews[zone->slot].range;
        if (range != nullptr) {
            range->setRect(rect);   // de straal van het huidige level, ook na een upgrade
            range->setVisible(true);
        }
    } else {
        m_placementRange->setRect(rect);
        m_placementRange->setVisible(true);
    }
}

// Een bouwplek wordt aangeklikt op de plek zelf; een gebouwde toren ook op zijn sprite.
std::optional<std::size_t> GameView::slotAt(const QPointF& scenePos) const
{
    const std::vector<BuildSlot>& allSlots = m_game.buildSlots();
    for (std::size_t i = 0; i < allSlots.size(); ++i) {
        const Vec2& p = allSlots[i].position;
        if (std::hypot(scenePos.x() - p.x, scenePos.y() - p.y) <= kSlotRadius) {
            return i;
        }
    }
    for (std::size_t i = 0; i < m_slotViews.size(); ++i) {
        const QGraphicsPixmapItem* body = m_slotViews[i].body;
        if (body != nullptr && body->sceneBoundingRect().contains(scenePos)) {
            return i;
        }
    }
    return std::nullopt;
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
    view.range->setVisible(false);   // standaard verborgen: de zone verschijnt pas als de toren geselecteerd is
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
    if (tower.type() == TowerType::Barracks) {
        showFlag(index);
    }
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

    if (tower.type() == TowerType::Archer) {
        updateArchers(index);
    }
}

// Zet de twee stippen op de lanceerpunten van de toren (LaunchPoints.h), dus precies waar de pijlen beginnen.
// Het zijn kinderen van het torenitem: ze verplaatsen en schalen met de toren mee en liggen er altijd bovenop.
// Na een upgrade worden ze alleen verplaatst naar de punten van het nieuwe level, niet opnieuw gemaakt.
void GameView::updateArchers(std::size_t index)
{
    SlotView& view = m_slotViews[index];
    const std::span<const LaunchPoint> points = launchPoints(TowerType::Archer, m_game.buildSlots()[index].tower->level());

    // Het torenitem is 1/kSpriteSupersample geschaald en staat op het midden van de bouwplek, dus een punt in
    // scenepixels t.o.v. dat midden is hier kSpriteSupersample keer groter.
    const double r = kArcherDotRadius * kSpriteSupersample;
    for (std::size_t i = 0; i < points.size() && i < std::size(view.archers); ++i) {
        if (view.archers[i] == nullptr) {
            view.archers[i] = new QGraphicsEllipseItem(-r, -r, 2 * r, 2 * r, view.body);
            view.archers[i]->setBrush(QColor(70, 210, 75));
            view.archers[i]->setPen(QPen(QColor(15, 60, 20), kSpriteSupersample));
        }
        view.archers[i]->setPos(points[i].x * kSpriteSupersample, points[i].y * kSpriteSupersample);
    }
}

void GameView::refreshLabel(std::size_t index)
{
    const Tower& tower = *m_game.buildSlots()[index].tower;
    m_slotViews[index].label->setText(QString("Lv %1").arg(int(tower.level())));
}

void GameView::openMenu(std::size_t index, const QPoint& globalPos)
{
    QMenu menu(this);
    m_openMenu = &menu;
    m_openMenuSlot = index;
    m_openMenuActions.clear();

    // Een toren die staat is nu geselecteerd (zijn bereik verschijnt, een eerdere selectie verdwijnt).
    // Een lege bouwplek deselecteert; daar toont het menu het bereik van de toren die je zou bouwen.
    if (m_game.buildSlots()[index].tower != nullptr) {
        m_ranges.select(index);
    } else {
        m_ranges.clearSelection();
    }
    applyRangeDisplay();

    // De regels komen uit slotMenuEntries(): dezelfde functie die refreshOpenMenu() bij elke goudwijziging gebruikt.
    for (const MenuEntry& entry : slotMenuEntries(m_game, index)) {
        QAction* action = menu.addAction(QString::fromStdString(entry.text));
        action->setEnabled(entry.enabled);
        m_openMenuActions.push_back(action);

        if (entry.action == MenuAction::Build) {
            const TowerType type = entry.type;
            connect(action, &QAction::triggered, this, [this, index, type]() { build(index, type); });
            // Wijs je een toren in het menu aan, dan zie je meteen zijn bereik op deze bouwplek.
            connect(action, &QAction::hovered, this, [this, index, type]() {
                m_ranges.showPreview(index, type);
                applyRangeDisplay();
            });
        } else if (entry.action == MenuAction::Upgrade) {
            connect(action, &QAction::triggered, this, [this, index]() { upgrade(index); });
        } else if (entry.action == MenuAction::Sell) {
            connect(action, &QAction::triggered, this, [this, index]() { sell(index); });
        }
    }
    // De vlag van een Barracks verplaats je ook hier: het menu kiest alleen, de vlag zelf volgt daarna de muis.
    const Tower* tower = m_game.buildSlots()[index].tower;
    if (tower != nullptr && tower->type() == TowerType::Barracks) {
        menu.addSeparator();
        QAction* flagAction = menu.addAction("Vlag plaatsen / verplaatsen");
        connect(flagAction, &QAction::triggered, this, [this, index]() { beginFlagPlacement(index); });
    }
    menu.exec(globalPos);

    // Het menu is dicht, gekozen of geannuleerd: het bouwvoorbeeld verdwijnt. Een net gebouwde toren blijft geselecteerd.
    m_ranges.hidePreview();
    applyRangeDisplay();

    m_openMenuActions.clear();
    m_openMenu.clear();
}

// Wordt aangeroepen via het signal goldChanged, dus bij elke verdiende of uitgegeven euro. Het goud wordt hier niet
// bewaard: elke regel wordt opnieuw berekend uit Game, zodat het menu nooit met een oud bedrag werkt.
void GameView::refreshOpenMenu()
{
    if (m_openMenu.isNull()) {
        return;
    }
    const std::vector<MenuEntry> entries = slotMenuEntries(m_game, m_openMenuSlot);
    if (entries.size() != m_openMenuActions.size()) {
        m_openMenu->close();   // de bouwplek is veranderd (bijvoorbeeld net bebouwd): het menu klopt niet meer
        return;
    }
    for (std::size_t i = 0; i < entries.size(); ++i) {
        m_openMenuActions[i]->setText(QString::fromStdString(entries[i].text));
        m_openMenuActions[i]->setEnabled(entries[i].enabled);
    }
}

void GameView::build(std::size_t index, TowerType type)
{
    if (!m_game.buildTower(index, type)) {
        return;
    }
    showTower(index);
    m_ranges.select(index);   // de nieuwe toren blijft geselecteerd: zijn bereik vervangt het bouwvoorbeeld
    applyRangeDisplay();
    emit goldChanged(m_game.player().gold());
}

void GameView::upgrade(std::size_t index)
{
    if (!m_game.upgradeTower(index)) {
        return;
    }
    refreshSprite(index);
    refreshLabel(index);
    applyRangeDisplay();   // een geselecteerde toren toont zijn bereik van het nieuwe level
    emit goldChanged(m_game.player().gold());
}

// Verkoopt de toren op deze bouwplek. Game bepaalt of het mag en hoeveel het oplevert en schrijft het goud bij;
// hier verdwijnt alleen wat je ziet. Een tweede klik vindt geen toren meer en doet niets.
void GameView::sell(std::size_t index)
{
    if (!m_game.sellTower(index)) {
        return;
    }
    if (m_ranges.selected() == index) {
        m_ranges.clearSelection();   // een verkochte toren heeft geen bereik meer
    }
    clearTowerView(index);
    applyRangeDisplay();
    updateUnits();   // de soldaten van een verkochte Barracks verdwijnen meteen, niet pas bij de volgende tick
    emit goldChanged(m_game.player().gold());
}

void GameView::clearTowerView(std::size_t index)
{
    SlotView& view = m_slotViews[index];

    if (m_flagSlot.has_value() && *m_flagSlot == index) {
        endFlagPlacement();   // de vlag van deze Barracks werd nog geplaatst
    }

    // Een item uit de scene halen = het verwijderen: QGraphicsItem haalt zichzelf en zijn kinderen uit de scene.
    delete view.body;               // de twee boogschutters zijn kinderen van de body
    view.body = nullptr;
    view.archers[0] = nullptr;
    view.archers[1] = nullptr;
    delete view.range;
    view.range = nullptr;
    delete view.label;
    view.label = nullptr;
    delete view.flag.root;          // de ring en het rode kruis zijn kinderen van de vlag
    view.flag = FlagView{};

    view.marker->setVisible(true);  // de bouwplek is weer leeg en bouwbaar
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
    for (TowerType type : kBuildableTowers) {
        ProjectileStyle style;
        if (!projectileStyle(type, style)) {
            continue;
        }
        for (int stage = 0; stage < 3; ++stage) {
            m_projectileSprites[style.kind][stage] = QPixmap(QString(":/assets/projectiles/%1%2.png").arg(style.name).arg(stage + 1));
        }
    }
}

// Zorgt dat er precies een sprite per kogel in Game::projectiles() is, en zet die op de plek en richting
// die Projectile heeft berekend. Een kogel die geland is verdwijnt uit Game en dus ook van het scherm.
void GameView::syncProjectiles()
{
    std::unordered_set<std::uint64_t> alive;
    for (const Projectile& projectile : m_game.projectiles()) {
        alive.insert(projectile.id());

        auto it = m_projectileItems.find(projectile.id());
        if (it == m_projectileItems.end()) {
            QGraphicsItem* item = nullptr;
            ProjectileStyle style;
            if (projectileStyle(projectile.source(), style)) {
                const QPixmap& pixmap = m_projectileSprites[style.kind][projectile.level() - 1];
                if (!pixmap.isNull()) {
                    auto* sprite = m_scene->addPixmap(pixmap);
                    sprite->setTransformationMode(Qt::SmoothTransformation);
                    // Het item draait en schaalt rond zijn positie; die ligt op de kop van de kogel.
                    sprite->setOffset(-(pixmap.width() - style.headInset), -pixmap.height() / 2.0);
                    sprite->setScale(style.scale);
                    item = sprite;
                }
            }
            if (item == nullptr) {
                // Sprite ontbreekt: val terug op een stip zodat de kogel zichtbaar blijft.
                item = m_scene->addEllipse(-4, -4, 8, 8, QPen(Qt::black, 1), QBrush(Qt::white));
            }
            item->setZValue(10);
            it = m_projectileItems.emplace(projectile.id(), item).first;
        }
        it->second->setPos(projectile.position().x, projectile.position().y);
        it->second->setRotation(projectile.angleDegrees());
    }

    for (auto it = m_projectileItems.begin(); it != m_projectileItems.end();) {
        if (alive.count(it->first) == 0) {
            delete it->second;
            it = m_projectileItems.erase(it);
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

void GameView::createWaveButton()
{
    QPainterPath shape;
    shape.addRoundedRect(-kWaveButtonWidth / 2, -kWaveButtonHeight / 2, kWaveButtonWidth, kWaveButtonHeight, 10, 10);
    m_waveButton = m_scene->addPath(shape, QPen(Qt::white, 3), QBrush(QColor(46, 140, 60)));
    m_waveButton->setZValue(15);
    m_waveButton->setCursor(Qt::PointingHandCursor);

    // Bij het begin van het pad, iets erboven, binnen de scene.
    const Vec2 start = m_game.path().pointAt(kWaveButtonPathOffset);
    const QRectF bounds = m_scene->sceneRect();
    m_waveButton->setPos(
        std::clamp(start.x, bounds.left() + kWaveButtonWidth / 2 + 4, bounds.right() - kWaveButtonWidth / 2 - 4),
        std::clamp(start.y - kWaveButtonLift, bounds.top() + kWaveButtonHeight / 2 + 4, bounds.bottom() - kWaveButtonHeight / 2 - 4));

    QFont titleFont("Arial", 13, QFont::Bold);
    QFont subFont("Arial", 10, QFont::Bold);
    m_waveTitle = new QGraphicsSimpleTextItem(m_waveButton);
    m_waveTitle->setFont(titleFont);
    m_waveTitle->setBrush(Qt::white);
    m_waveSub = new QGraphicsSimpleTextItem(m_waveButton);
    m_waveSub->setFont(subFont);
    m_waveSub->setBrush(QColor(255, 225, 90));
    m_waveBar = new QGraphicsRectItem(m_waveButton);
    m_waveBar->setPen(Qt::NoPen);
    m_waveBar->setBrush(QColor(255, 225, 90));

    updateWaveButton();
}

// Wordt elke tick aangeroepen, zodat de bonus en de timer continu meelopen.
void GameView::updateWaveButton()
{
    if (m_waveButton == nullptr) {
        return;
    }
    const bool available = m_game.canStartNextWave();
    m_waveButton->setVisible(available);
    if (!available) {
        return;
    }

    const WaveManager& waves = m_game.waves();
    const bool first = waves.firstWavePending();
    m_waveTitle->setText(first ? "Start Wave" : "Next Wave");
    const QRectF title = m_waveTitle->boundingRect();

    if (first) {
        // Eerste wave: geen bonus en geen timer.
        m_waveSub->setVisible(false);
        m_waveBar->setVisible(false);
        m_waveTitle->setPos(-title.width() / 2, -title.height() / 2);
        return;
    }

    const int secondsLeft = static_cast<int>(std::floor(waves.timeUntilNextWave()));
    m_waveSub->setText(QString("+%1 goud  (%2s)").arg(waves.earlyBonus()).arg(secondsLeft));
    m_waveSub->setVisible(true);
    m_waveTitle->setPos(-title.width() / 2, -kWaveButtonHeight / 2 + 4);
    const QRectF sub = m_waveSub->boundingRect();
    m_waveSub->setPos(-sub.width() / 2, -kWaveButtonHeight / 2 + 4 + title.height() - 3);

    // Balkje onderin: vol bij 60 goud (20 s over), leeg bij 0.
    const double window = static_cast<double>(kMaxEarlyBonus) / kEarlyBonusPerSecond;
    const double fraction = std::clamp(waves.timeUntilNextWave() / window, 0.0, 1.0);
    m_waveBar->setRect(-kWaveButtonWidth / 2 + 8, kWaveButtonHeight / 2 - 9, (kWaveButtonWidth - 16) * fraction, 4);
    m_waveBar->setVisible(true);
}

void GameView::startNextWave()
{
    const int bonus = m_game.startNextWave();   // start de wave meteen; de eerste wave geeft geen bonus
    if (bonus > 0) {
        emit goldChanged(m_game.player().gold());
        showBonusText(bonus);
    }
    updateWaveButton();                          // de knop verdwijnt na het klikken
}

void GameView::showBonusText(int bonus)
{
    QGraphicsSimpleTextItem* text = m_scene->addSimpleText(QString("+%1 goud").arg(bonus), QFont("Arial", 16, QFont::Bold));
    text->setBrush(QColor(255, 225, 90));
    text->setPen(QPen(Qt::black, 1));
    text->setPos(m_waveButton->pos() - QPointF(text->boundingRect().width() / 2, 40));
    text->setZValue(20);
    QTimer::singleShot(1200, this, [text]() { delete text; });
}

void GameView::showEndText(const QString& text)
{
    QGraphicsTextItem* item = m_scene->addText(text, QFont("Arial", 48, QFont::Bold));
    item->setDefaultTextColor(Qt::white);
    item->setPos(300, 230);
    item->setZValue(20);
}

GameView::FlagView GameView::makeFlagView(const QColor& color)
{
    FlagView view;

    // De voet van de vlag ligt op (0, 0): de paal gaat omhoog en de wimpel wappert naar rechts.
    QPainterPath banner;
    banner.moveTo(0, -kFlagPoleHeight);
    banner.lineTo(kFlagBannerWidth, -kFlagPoleHeight + 7);
    banner.lineTo(0, -kFlagPoleHeight + 14);
    banner.closeSubpath();

    view.root = new QGraphicsPathItem(banner);
    view.root->setPen(QPen(Qt::white, 2));
    view.root->setZValue(7);

    auto* pole = new QGraphicsLineItem(0, 0, 0, -kFlagPoleHeight, view.root);
    pole->setPen(QPen(QColor(70, 45, 20), 3));

    // Een ring op de grond: de plek waar de soldaten verzamelen, ook goed te zien als de wimpel klein is.
    view.ring = new QGraphicsEllipseItem(-15, -7, 30, 14, view.root);
    view.ring->setZValue(-1);

    // Een rood kruis, alleen zichtbaar bij een ongeldige plek (zie updateFlagGhost).
    QPainterPath cross;
    cross.moveTo(-9, -16);
    cross.lineTo(9, 2);
    cross.moveTo(9, -16);
    cross.lineTo(-9, 2);
    view.cross = new QGraphicsPathItem(cross, view.root);
    view.cross->setPen(QPen(QColor(220, 30, 30), 4, Qt::SolidLine, Qt::RoundCap));
    view.cross->setVisible(false);

    setFlagColor(view, color);
    m_scene->addItem(view.root);
    return view;
}

void GameView::setFlagColor(const FlagView& view, const QColor& color)
{
    view.root->setBrush(color);
    view.ring->setBrush(QColor(color.red(), color.green(), color.blue(), 90));
    view.ring->setPen(QPen(color.lighter(130), 2));
}

void GameView::createFlagGhost()
{
    m_ghostFlag = makeFlagView(kValidFlagColor);
    m_ghostFlag.root->setZValue(12);
    m_ghostFlag.root->setOpacity(0.85);
    m_ghostFlag.root->setVisible(false);

    // De uitleg bij een ongeldige plek staat op een donkere achtergrond, zodat hij op elke kaart leesbaar blijft.
    m_flagHint = m_scene->addRect(0, 0, 10, 10, QPen(Qt::NoPen), QBrush(QColor(0, 0, 0, 190)));
    m_flagHint->setZValue(13);
    m_flagHintText = new QGraphicsSimpleTextItem("Alleen op de weg!", m_flagHint);
    m_flagHintText->setFont(QFont("Arial", 10, QFont::Bold));
    m_flagHintText->setBrush(QColor(255, 120, 120));
    const QRectF text = m_flagHintText->boundingRect();
    m_flagHintText->setPos(5, 3);
    m_flagHint->setRect(0, 0, text.width() + 10, text.height() + 6);
    m_flagHint->setVisible(false);
}

void GameView::showFlag(std::size_t index)
{
    const std::optional<Vec2> position = m_game.flagPosition(index);
    if (!position.has_value()) {
        return;
    }
    m_slotViews[index].flag = makeFlagView(kPlacedFlagColor);
    m_slotViews[index].flag.root->setPos(position->x, position->y);
}

void GameView::beginFlagPlacement(std::size_t slot)
{
    m_flagSlot = slot;
    m_roadHighlight->setVisible(true);
    m_ghostFlag.root->setVisible(true);
    updateFlagGhost(mapToScene(mapFromGlobal(QCursor::pos())));
}

void GameView::endFlagPlacement()
{
    if (!m_flagSlot.has_value()) {
        return;
    }
    m_flagSlot.reset();
    m_roadHighlight->setVisible(false);
    m_ghostFlag.root->setVisible(false);
    m_flagHint->setVisible(false);
    viewport()->unsetCursor();
}

void GameView::updateFlagGhost(const QPointF& scenePos)
{
    // Game beslist of de plek geldig is (weg, binnen de kaart, niet op een bouwplek); de view toont alleen het antwoord.
    const bool valid = m_game.canPlaceFlag({scenePos.x(), scenePos.y()});
    m_ghostFlag.root->setPos(scenePos);
    setFlagColor(m_ghostFlag, valid ? kValidFlagColor : kInvalidFlagColor);
    m_ghostFlag.cross->setVisible(!valid);
    m_flagHint->setVisible(!valid);
    m_flagHint->setPos(scenePos.x() + 16.0, scenePos.y() - kFlagPoleHeight - 18.0);
    viewport()->setCursor(valid ? Qt::CrossCursor : Qt::ForbiddenCursor);
}

void GameView::placeFlagAt(const QPointF& scenePos)
{
    if (!m_flagSlot.has_value()) {
        return;
    }
    const std::size_t slot = *m_flagSlot;
    if (!m_game.placeFlag(slot, {scenePos.x(), scenePos.y()})) {
        flashInvalidFlag(scenePos);   // niet geldig: er verandert niets en de speler kan het opnieuw proberen
        return;
    }
    m_slotViews[slot].flag.root->setPos(scenePos);
    endFlagPlacement();
}

void GameView::flashInvalidFlag(const QPointF& scenePos)
{
    QGraphicsEllipseItem* flash = m_scene->addEllipse(scenePos.x() - 18, scenePos.y() - 18, 36, 36,
                                                      QPen(QColor(230, 40, 40), 3), QBrush(QColor(230, 40, 40, 90)));
    flash->setZValue(14);
    QTimer::singleShot(250, this, [flash]() { delete flash; });
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
    syncProjectiles();
    updateWaveButton();

    const std::size_t wave = m_game.waves().currentWave();
    if (wave != m_shownWave) {
        m_shownWave = wave;
        emit waveChanged(static_cast<int>(wave), totalWaves());
    }

    if (m_game.isGameOver()) {
        m_timer.stop();
        endFlagPlacement();
        if (!m_openMenu.isNull()) {
            m_openMenu->close();   // na het einde van het spel mag er niet meer gebouwd worden
        }
        showEndText("GAME OVER");
        emit gameOver();
    } else if (m_game.isWon()) {
        m_timer.stop();
        endFlagPlacement();
        if (!m_openMenu.isNull()) {
            m_openMenu->close();
        }
        showEndText("VICTORY");
        emit gameWon();
    }
}

}