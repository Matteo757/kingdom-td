#ifndef KTD_ROUTEBOUNDS_H
#define KTD_ROUTEBOUNDS_H

#include "ktd/Path.h"
#include "ktd/Vec2.h"

namespace ktd {

// Halve breedte van de begaanbare weg in sceneruimte (de weg op de kaarten is ongeveer 58 px breed;
// we blijven bewust iets smaller zodat een soldaat altijd netjes op het zand staat).
constexpr double kRoadHalfWidth = 24.0;

// Vereiste 2: useful and correct abstraction (explain why)
// De rest van het spel hoeft niet te weten hoe de "onzichtbare muren" berekend worden: het vraagt alleen
// contains(), clamp() of moveWithin(). Zo kan de berekening later vervangen worden (bijvoorbeeld door
// een echt navigatiemesh) zonder dat soldaten, Barracks of de GUI aangepast hoeven te worden.
//
// De muren zijn geen losse lijnstukken maar de offsetcurve van de route zelf: alles wat hoogstens
// halfWidth van de route af ligt, is weg. Dat is de vereniging van afgeronde stukken (capsules) langs elk
// wegsegment. Daardoor volgen de muren bochten en hoeken automatisch, en kunnen er per constructie geen
// gaten of overlappingen op de hoeken ontstaan. Er worden geen tekenobjecten of botsingsitems aangemaakt,
// dus de muren zijn onzichtbaar en hebben geen invloed op vijanden, torens of kogels.
class RouteBounds {
public:
    explicit RouteBounds(const Path& path, double halfWidth = kRoadHalfWidth);

    bool contains(const Vec2& point) const;
    // Het dichtstbijzijnde punt binnen de weg (het punt zelf als het er al binnen ligt).
    Vec2 clamp(const Vec2& point) const;
    // Beweging van from naar to: de eenheid stopt of schuift langs de muur in plaats van er doorheen te gaan.
    Vec2 moveWithin(const Vec2& from, const Vec2& to) const;
    // True als het hele lijnstuk binnen de weg blijft (voor toekomstige pathfinding).
    bool segmentInside(const Vec2& from, const Vec2& to) const;

    double distanceToRoute(const Vec2& point) const;
    double halfWidth() const;

private:
    Vec2 closestOnRoute(const Vec2& point) const;

    Path m_path;
    double m_halfWidth;
};

}

#endif  // KTD_ROUTEBOUNDS_H