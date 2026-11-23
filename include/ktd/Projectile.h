#ifndef KTD_PROJECTILE_H
#define KTD_PROJECTILE_H

#include <cstdint>
#include <memory>
#include <vector>
#include "ktd/Damage.h"
#include "ktd/Enemy.h"
#include "ktd/Path.h"
#include "ktd/TowerType.h"
#include "ktd/Vec2.h"

namespace ktd {

// Hoe een kogel van een bepaalde toren vliegt. De GUI tekent de kogel precies op de plek die Projectile
// hier berekent, dus de schade valt op het moment dat je de kogel ziet inslaan.
struct FlightProfile {
    double speed;    // px per seconde langs de baan
    double arc;      // boogtop als fractie van de afstand tot het doel (0 = rechte lijn)
    double minArc;   // minimale boogtop in px, zodat ook een korte worp een zichtbare boog heeft
};

//  - pijl:        lage boog;
//  - kanonskogel: rechte lijn, zwaar en traag;
//  - magiebol:    rechte lijn, veruit het snelst;
//  - raket:       hoge boog en traag.
// Alle snelheden zijn gehalveerd t.o.v. de eerste versie, zodat de kogels goed te volgen zijn.
FlightProfile flightProfile(TowerType type);

// Een kogel in de lucht. Hij brengt zijn schade pas toe bij de inslag (update() geeft dan true terug).
// De kogel volgt zijn doel zolang dat bestaat, dus hij mist nooit. Heeft splashRadius > 0 (artillerie), dan
// krijgen alle vijanden binnen die straal van het inslagpunt schade.
//
// De kogel kent zijn doel alleen via een pointer. Verdwijnt het doel (dood of aan het einde), dan roept Game
// dropTarget() aan: de kogel vliegt dan door naar de laatste bekende plek en richt daar geen schade aan,
// behalve een splash-kogel, die gewoon ontploft. Zo hangt er nooit een pointer naar een verwijderde vijand.
class Projectile {
public:
    Projectile(TowerType source, std::uint8_t level, const Vec2& from, Enemy& target, const Path& path,
               const Damage& damage, double splashRadius = 0.0);

    // Laat de kogel dt seconden vliegen. Geeft true zodra hij geland is en zijn schade heeft toegebracht.
    bool update(double dt, const std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path);
    void dropTarget(const Enemy* enemy);

    void setId(std::uint64_t id);
    std::uint64_t id() const;

    TowerType source() const;
    std::uint8_t level() const;          // torenlevel = stage (1..3) van de sprite
    const Enemy* target() const;
    bool hasTarget() const;
    double splashRadius() const;
    const Vec2& position() const;
    double angleDegrees() const;         // 0 = naar rechts, 90 = naar beneden (schermcoordinaten)
    double duration() const;             // vliegtijd in seconden
    double progress() const;             // 0..1

private:
    void land(const std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path);

    TowerType m_source;
    std::uint8_t m_level;
    Vec2 m_from;
    Vec2 m_aim;
    Vec2 m_position;
    Enemy* m_target;
    Damage m_damage;
    double m_splash;
    double m_arcHeight = 0.0;
    double m_duration = 0.05;
    double m_elapsed = 0.0;
    double m_angle = 0.0;
    std::uint64_t m_id = 0;
};

}

#endif  // KTD_PROJECTILE_H
