#pragma once

#include <QLabel>
#include <QWidget>

namespace ktd {

class Hud : public QWidget {
    Q_OBJECT

public:
    explicit Hud(QWidget* parent = nullptr);

public slots:
    void setGold(int gold);
    void setLives(int lives);
    void setWave(int current, int total);

private:
    QLabel* m_gold;
    QLabel* m_wave;
    QLabel* m_lives;
};

}