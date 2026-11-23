# Kingdom TD

Een tower defense game in C++20 en Qt 6, geïnspireerd op Kingdom Rush. Vijanden lopen over een pad naar je kasteel. Jij bouwt torens op de bouwplekken om ze tegen te houden, en gooit af en toe een vuurbal.

Gemaakt door Matteo Belmans (Matteo757) als eindopdracht C++.

## Inhoud

1. [Spelen](#spelen)
2. [Build-handleiding](#build-handleiding)
3. [UML-diagram](#uml-diagram)
4. [Projectstructuur](#projectstructuur)
5. [Levels aanpassen](#levels-aanpassen)
6. [Tests](#tests)
7. [Bug tickets](#bug-tickets)
8. [Credits en licenties](#credits-en-licenties)

## Spelen

| Actie | Besturing |
|---|---|
| Toren bouwen of upgraden | Linkermuisknop op een bouwplek (het bordje) opent het menu |
| Vuurbal gooien | Rechtermuisknop op het pad (heeft een cooldown) |
| Route tonen of verbergen | Toets `P` (handig om de waypoints te controleren) |

- **Torens:** boogschutter (fysieke schade), mage (magische schade), kanon, artillerie en barracks (soldaten die vijanden tegenhouden). Elke toren kan tot level 3 geüpgraded worden.
- **Vijanden:** goblin, skeleton, orc en dark mage. Ze hebben verschillende health, snelheid, armor en magic resist. Schade wordt dus per type anders verminderd.
- **Doel:** overleef alle waves. Elke vijand die het kasteel bereikt kost een leven. Bij 0 levens is het game over.
- **Levels:** level 1 en level 3 zitten erin. Het spel start in level 1.

## Build-handleiding

Het project is te bouwen op een schone pc met alleen de onderstaande programma's. Alle andere bibliotheken (nlohmann/json) zitten al in de repository onder `third_party/`.

### Wat je nodig hebt

| Programma | Versie |
|---|---|
| C++-compiler met C++20 (GCC 11+, Clang 14+ of MinGW/MSVC met C++20) | |
| CMake | 3.21 of nieuwer |
| Qt | 6.x, met de modules **Widgets** en **Test** |
| Git | om te klonen |

Qt installeer je op elk systeem op een eigen manier:

- **Windows:** download de *Qt Online Installer* van <https://www.qt.io/download-qt-installer>. Kies bij "Qt 6.x" de kit **MinGW 64-bit** en onder "Developer and Designer Tools" ook **CMake** en **MinGW**. Dit is ook de omgeving waarin het project ontwikkeld is (Qt 6.12 met MinGW 64-bit).
- **Ubuntu/Debian:**
  ```bash
  sudo apt update
  sudo apt install build-essential cmake git qt6-base-dev libgl1-mesa-dev
  ```
- **macOS:**
  ```bash
  brew install cmake qt
  ```

### Optie 1: Qt Creator (het makkelijkst)

1. Clone de repository:
   ```bash
   git clone https://github.com/Matteo757/kingdom-td.git
   ```
2. Open Qt Creator, kies *File → Open File or Project* en selecteer `CMakeLists.txt` in de map `kingdom-td`.
3. Kies een Qt 6 kit (bijvoorbeeld *Desktop Qt 6.x MinGW 64-bit*) en klik *Configure Project*.
4. Kies linksonder *Release* of *Debug* en klik op de groene pijl (*Run*). Het project wordt gebouwd en gestart.

### Optie 2: opdrachtregel

```bash
git clone https://github.com/Matteo757/kingdom-td.git
cd kingdom-td

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# programma starten
./build/kingdom-td            # Linux en macOS
build\kingdom-td.exe          # Windows
```

Vindt CMake Qt niet, geef dan de installatiemap van Qt mee. Bijvoorbeeld op Windows:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64
```

Op Linux of macOS is dat bijvoorbeeld `-DCMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/gcc_64`.

### Belangrijk om te weten

- **Start het programma vanuit de build-map.** Tijdens het bouwen worden de levelbestanden (`assets/levels/*.json`) automatisch naast het programma gezet, in een map `levels/`. Het spel leest ze bij het starten. De afbeeldingen zitten in het programma zelf (Qt-resources), dus daarvoor hoef je niets te kopiëren.
- **Ontbreekt of is een levelbestand ongeldig?** Dan meldt het spel dat in de console en valt het terug op het ingebouwde level. Het stopt dus niet.
- **Windows buiten Qt Creator:** zorg dat de `bin`-map van Qt en van MinGW in je `PATH` staat, of voer `windeployqt build\kingdom-td.exe` uit om de benodigde DLL's naast het programma te zetten.

### Veelvoorkomende problemen

| Melding | Oplossing |
|---|---|
| `Could not find a package configuration file provided by "Qt6"` | Geef `-DCMAKE_PREFIX_PATH=<pad naar Qt>` mee (zie hierboven). |
| `Qt6Test` niet gevonden (Linux) | Installeer `qt6-base-dev` (bevat Qt Test) in plaats van alleen losse runtime-pakketten. |
| Het programma start maar toont niets / geen levels | Start vanuit de build-map, of kopieer `assets/levels` naar `levels/` naast het programma. |
| Aanpassing aan een afbeelding of `resources.qrc` verschijnt niet | *Build → Clean All* en daarna opnieuw bouwen. |

## UML-diagram

De diagrammen zijn in [Mermaid](https://mermaid.js.org/) geschreven en worden op GitHub automatisch getekend. Open dit bestand dus op GitHub om ze te zien.

Het project is opgedeeld in een **kern** (`ktd_core`, een statische bibliotheek zonder Qt: alle spelregels en logica) en een **GUI-laag** (Qt: tekenen, invoer en HUD). Daardoor kan de kern zonder venster worden getest.

### Kern: spelregels

```mermaid
classDiagram
    direction LR

    class Game {
        -Path m_path
        -Player m_player
        -WaveManager m_waves
        -vector~BuildSlot~ m_slots
        +level(int) Game
        +fromLevel(LevelData) Game
        +buildTower(slot, TowerType) bool
        +upgradeTower(slot) bool
        +castFireball(Vec2) bool
        +update(double dt) FrameEvents
        +isGameOver() bool
        +isWon() bool
    }
    class Player {
        -int m_gold
        -uint8_t m_lives
        +spend(int) bool
        +addGold(int)
        +loseLife()
    }
    class Path {
        -vector~Vec2~ m_waypoints
        +pointAt(double) Vec2
        +length() double
        +closestDistance(Vec2) double
    }
    class WaveManager {
        -vector~Wave~ m_waves
        +update(double dt) vector~EnemyType~
        +finished() bool
        +currentWave() size_t
    }
    class Wave {
        +vector~EnemyType~ enemies
        +double spawnInterval
    }
    class BuildSlot {
        +Vec2 position
        +Tower* tower
    }

    class Enemy {
        <<abstract>>
        -double m_health
        -uint8_t m_armor
        -uint8_t m_magicResist
        -double m_distance
        +speed()* double
        +name()* string
        +reward()* int
        +takeDamage(Damage)
        +update(double dt)
    }
    class Goblin
    class Skeleton
    class Orc
    class DarkMage

    class Tower {
        <<abstract>>
        -Vec2 m_position
        -double m_range
        -double m_damage
        -DamageType m_type
        -uint8_t m_level
        +attack(Enemy)* void
        +type()* TowerType
        +upgrade()
        +update(dt, enemies, path) Enemy
        +findTarget(enemies, path) Enemy
    }
    class ArcherTower
    class MageTower
    class CannonTower
    class ArtilleryTower
    class BarracksTower {
        -vector~Soldier~ m_soldiers
        +soldiers() vector~Soldier~
    }
    class Soldier {
        -double m_health
        -Enemy* m_target
        +engage(Enemy*)
        +takeDamage(double)
        +alive() bool
    }

    class Spell {
        <<abstract>>
        +cast(enemies, path, target)* void
        +tryCast(...) bool
        +ready() bool
    }
    class Fireball

    class Cooldown~T~ {
        -T m_interval
        -T m_remaining
        +update(T dt)
        +ready() bool
        +reset()
    }
    class Damage {
        +double amount
        +DamageType type
    }
    class TowerFactory {
        +makeTower(TowerType, Vec2, Path) unique_ptr~Tower~
    }
    class EnemyFactory {
        +makeEnemy(EnemyType) unique_ptr~Enemy~
    }
    class WaveFactory {
        +buildWaves(int count) vector~Wave~
    }
    class Prices {
        +buildCost(TowerType) int
        +upgradeCost(uint8_t level) int
    }

    Enemy <|-- Goblin
    Enemy <|-- Skeleton
    Enemy <|-- Orc
    Enemy <|-- DarkMage

    Tower <|-- ArcherTower
    Tower <|-- MageTower
    Tower <|-- CannonTower
    Tower <|-- ArtilleryTower
    Tower <|-- BarracksTower
    BarracksTower *-- "3" Soldier : bezit

    Spell <|-- Fireball

    Game *-- Path
    Game *-- Player
    Game *-- WaveManager
    Game *-- BuildSlot
    Game *-- "0..*" Tower : unique_ptr
    Game *-- "0..*" Enemy : unique_ptr
    Game *-- Spell : unique_ptr
    BuildSlot --> Tower : wijst naar
    WaveManager o-- Wave

    Tower *-- Cooldown~double~
    Soldier *-- Cooldown~double~
    Spell *-- Cooldown~double~
    Enemy ..> Damage : takeDamage
    Tower ..> Enemy : valt aan
    Soldier --> Enemy : houdt tegen

    TowerFactory ..> Tower : maakt
    EnemyFactory ..> Enemy : maakt
    WaveFactory ..> Wave : maakt
    Game ..> TowerFactory
    Game ..> EnemyFactory
    Game ..> WaveFactory
    Game ..> Prices
```

### Level laden en GUI

```mermaid
classDiagram
    direction TB

    class MainWindow {
        -AsyncLevelLoader m_loader
        -QTimer m_pollTimer
        -checkLevelLoaded()
        -showGame(Game, int level)
    }
    class AsyncLevelLoader {
        -thread m_thread
        -mutex m_mutex
        -atomic~bool~ m_finished
        -exception_ptr m_error
        +start(path)
        +isFinished() bool
        +result() LevelData
    }
    class LevelData {
        +int number
        +int startGold
        +int startLives
        +int waveCount
        +vector~Vec2~ route
        +vector~Vec2~ slotPositions
    }
    class LevelError {
        <<exception>>
    }
    class Game
    class GameView {
        -Game m_game
        -QGraphicsScene* m_scene
        -QTimer m_timer
        +goldChanged(int)
        +livesChanged(int)
        +waveChanged(int, int)
        +gameOver()
        +gameWon()
        #mousePressEvent()
        #keyPressEvent()
        -tick()
    }
    class Hud {
        -QLabel* m_gold
        -QLabel* m_wave
        -QLabel* m_lives
        +setGold(int)
        +setLives(int)
        +setWave(int, int)
    }
    class QMainWindow
    class QGraphicsView
    class QWidget
    class runtime_error

    QMainWindow <|-- MainWindow
    QGraphicsView <|-- GameView
    QWidget <|-- Hud
    runtime_error <|-- LevelError

    MainWindow *-- AsyncLevelLoader
    MainWindow --> GameView : maakt
    MainWindow --> Hud : maakt
    AsyncLevelLoader ..> LevelData : leest JSON
    AsyncLevelLoader ..> LevelError : gooit
    MainWindow ..> Game : fromLevel(LevelData)
    GameView *-- Game
    GameView ..> Hud : signals goldChanged, livesChanged, waveChanged
```

**Hoe het samenwerkt:**
1. `MainWindow` start een `AsyncLevelLoader` die het JSON-bestand in een aparte thread leest. Een `QTimer` controleert of de thread klaar is.
2. Bij succes wordt een `Game` gebouwd uit de `LevelData`. Bij een `LevelError` valt het programma terug op het ingebouwde level.
3. `GameView` laat via een `QTimer` elke tick de `Game` een stap zetten (`update(dt)`) en tekent het resultaat (`FrameEvents`) in een `QGraphicsScene`.
4. `GameView` stuurt signals (`goldChanged`, `livesChanged`, `waveChanged`) naar de `Hud`.

## Projectstructuur

```
kingdom-td/
├── CMakeLists.txt          # bouwt ktd_core (kern), het programma en de tests
├── resources.qrc           # kaarten en gebouwafbeeldingen (zitten in het programma)
├── include/ktd/            # headers
├── src/                    # implementatie
├── tests/                  # unit tests (Qt Test)
├── assets/
│   ├── levels/             # level1.json, level3.json (route, bouwplekken, goud, levens)
│   ├── maps/               # kaartafbeeldingen
│   └── buildings/          # gebouwsprites
└── third_party/nlohmann/   # json.hpp (nlohmann/json, MIT)
```

## Levels aanpassen

Een level is een JSON-bestand in `assets/levels/`. Alle coördinaten staan in sceneruimte (1024 × 576), net als de kaartafbeelding.

```json
{
    "number": 1,
    "gold": 1000,
    "lives": 20,
    "waves": 10,
    "route": [[0, 239], [101, 241], [225, 252]],
    "slots": [[177, 307], [291, 189]]
}
```

- `number` bepaalt welke kaart (`assets/maps/levelN.png`) getoond wordt.
- `route` heeft minstens 2 punten, `lives` is 1 t/m 255.
- Een ongeldig bestand levert een duidelijke foutmelding op in plaats van een crash.

## Tests

De spelregels worden getest zonder GUI. Na het bouwen:

```bash
ctest --test-dir build --output-on-failure
```

| Testbestand | Onderwerp |
|---|---|
| `test_core` | pad, schadeberekening met armor en magic resist |
| `test_enemies` | eigenschappen van de vijandtypes en de factory |
| `test_towers` | targeting, schade en upgrades van de torens |
| `test_build` | bouwen op bouwplekken en kosten |
| `test_spell` | vuurbal en cooldown |
| `test_wave` | wavetiming en spawnen |
| `test_game` | het spel als geheel |
| `test_level` | de ingebouwde levels |
| `test_player` | goud en levens |
| `test_progression` | moeilijkheidsopbouw over de waves |
| `test_levelloader` | JSON inlezen, foutafhandeling en het laden in een aparte thread |

## Bug tickets

> **Nog in te vullen door de auteur.** Zet hier de echte links; verzin niets.

**Opgelost ticket (eigen of ander project, met pull request of commit en issue-branch):**

- Issue: `<link naar het issue>`
- Branch: `<naam van de issue-branch>`
- Pull request of commit: `<link>`

**Gemeld ticket op een ander project:**

- Project: `<naam en link>`
- Ticket: `<link naar het gemelde issue>`

## Credits en licenties

- Eigen code: MIT-licentie, zie [LICENSE](LICENSE).
- [nlohmann/json](https://github.com/nlohmann/json) (MIT) voor het lezen van de levelbestanden.
- [Qt 6](https://www.qt.io/) (LGPL) voor de GUI en de unit tests.
- Afbeeldingen (kaarten, gebouwen): `<vul hier de bron of maker in>`
