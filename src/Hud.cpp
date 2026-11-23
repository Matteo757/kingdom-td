#include "ktd/Hud.h"

#include <QFont>
#include <QHBoxLayout>

namespace ktd {

Hud::Hud(QWidget* parent)
    : QWidget(parent)
    , m_gold(new QLabel("Goud: 0", this))
    , m_wave(new QLabel("Wave 0 / 0", this))
    , m_lives(new QLabel("Levens: 0", this))
{
    QFont f = font();
    f.setPointSize(14);
    f.setBold(true);
    setFont(f);

    auto* layout = new QHBoxLayout(this);
    layout->addWidget(m_gold);
    layout->addStretch();
    layout->addWidget(m_wave);
    layout->addStretch();
    layout->addWidget(m_lives);
}

void Hud::setGold(int gold)
{
    m_gold->setText(QString("Goud: %1").arg(gold));
}

void Hud::setLives(int lives)
{
    m_lives->setText(QString("Levens: %1").arg(lives));
}

void Hud::setWave(int current, int total)
{
    m_wave->setText(QString("Wave %1 / %2").arg(current).arg(total));
}

}