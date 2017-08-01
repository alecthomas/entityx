/**
 * This is an example of using EntityX with Qt.
 */

// std
#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <sstream>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <ctime>

// EntityX
#include "entityx/entityx.hh"

//QtCore
#include <QDebug>
#include <QStandardItemModel>
#include <QEvent>
#include <QTime>
#include <QTimer>
#include <QDateTime>

// QtGui
#include <QGuiApplication>
#include <QVector2D>
#include <QColor>

// QtQuick
#include <QQmlContext>
#include <QQuickView>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQmlComponent>

using std::cerr;
using std::cout;
using std::endl;

namespace ex = entityx;

float r(int a, float b = 0) {
    return (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * a + b;
}

// A position, direction, and rotation.
struct Body {
    Body(const QVector2D &position, const QVector2D &direction, float rotationd = 0.0)
        : position(position), direction(direction), rotationd(rotationd) {}

    QVector2D position;
    QVector2D direction;
    float rotation = 0.0;
    float rotationd;
    float alpha = 0.0;
};

// A Renderable.
struct Renderable {
    Renderable(float radius, QColor color, QPointF position) :
        radius(radius)
      , color(color)
      , position(position) {}

    float radius;
    QColor color;
    QPointF position;
};


// An Indestructible entity can collide but will not be destroyed.
struct Indestructible {};

// UI configuration
struct UIConfig {
    explicit UIConfig(QSize size)
        : size(size) {}

    QSize size;
};

// A Particle is a coloured object that fades out over a duration.
struct Particle {
    explicit Particle(QColor color, float radius, float duration)
        : color(color), radius(radius), alpha(color.alpha()), d(color.alpha() / duration) {}

    QColor color;
    float radius;
    float alpha;
    float d;
};


// A collision radius.
struct Collideable {
    explicit Collideable(float radius) : radius(radius) {}

    float radius;
};


using EntityManager = entityx::EntityX<
entityx::DefaultStorage, 0,
Body, Renderable, Particle, Collideable, Indestructible, UIConfig>;
using Entity = EntityManager::Entity;

namespace std {
template <>
struct hash<Entity> {
    std::size_t operator()(const Entity& k) const { return k.id().id(); }
};
}


struct System {
    virtual ~System() {}
    virtual void update(EntityManager &es, float dt) = 0;
    virtual void receive(QEvent *event) { Q_UNUSED(event) }
};


// CursorInputSystem processes user input and applies it to the cursor.
struct CursorInputSystem : System {
public:
    explicit CursorInputSystem(QQuickView *view, EntityManager &es) : m_view(view) {
        // Create the cursor.
        entity = es.create();

        entity.assign<Indestructible>();

        const float radius = 16.0;

        entity.assign<Collideable>(radius);

        entity.assign<Body>(
                    QVector2D(0, 0),
                    QVector2D(0, 0));

        entity.assign<Renderable>(radius,
                                  QColor(r(128, 127), r(128, 127), r(128, 127), 255),
                                  QPointF(radius, radius));
    }

    // Pulse the cursor.
    void update(EntityManager &es, float dt) override {
        const double radius = 16 + std::sin(time * 10.0) * 4;

        entity.component<Collideable>()->radius = radius;
        entity.component<Renderable>()->radius = radius;
        time += dt;
    }

    void receive(QEvent *event) override {
        if (event->type() == QEvent::MouseMove) {
            entity.component<Body>()->position.setX(static_cast<QMouseEvent*>(event)->pos().x());
            entity.component<Body>()->position.setY(static_cast<QMouseEvent*>(event)->pos().y());
        }
    }

    Entity get_cursor_entity() {
        return entity;
    }

private:
    QQuickItem *m_cursorItem;
    QQuickView *m_view;
    Entity entity;
    double time = 0;
};


// CursorPushSystem applies momentum to objects surrounding the cursor.
class CursorPushSystem : public System {
public:
    explicit CursorPushSystem(Entity cursor) : cursor(cursor) {}

    void update(EntityManager &es, float dt) override {
        assert(cursor);
        QVector2D cursor_position = cursor.component<Body>()->position;
        es.for_each<Body>([&](Entity entity, Body &body) {
            if (entity == cursor) return;
            QVector2D direction = body.position - cursor_position;
            float distance = direction.length();
            if (distance < 100.0) {
                body.direction += direction / distance * 2.0f * dt * 200.0f;
            }
        });
    }

private:
    Entity cursor;
};


// Ensure that a certain density of balls are present.
class SpawnSystem : public System {
    constexpr static const float DENSITY = 0.0001;
    constexpr static const int MINSIZE = 5;
    constexpr static const int MAXSIZE = 10;

public:
    SpawnSystem(Entity cfg) : m_cfg(cfg) {}

    void update(EntityManager &es, float dt) override {
        m_size = m_cfg.component<UIConfig>()->size;
        count = m_size.width() * m_size.height() * DENSITY;
        int c = 0;
        es.for_each<Collideable>([&](Entity, Collideable&) { c++; });

        for (int i = 0; i < count - c; i++) {
            Entity entity = es.create();

            // Mark as collideable (explosion particles will not be collideable).
            Collideable *collideable = entity.assign<Collideable>(r(MAXSIZE, MINSIZE));

            // "Physical" attributes.
            entity.assign<Body>(
                        QVector2D(r(m_size.width()), r(m_size.height())),
                        QVector2D(r(100, -50), r(100, -50)));

            entity.assign<Renderable>(collideable->radius,
                                      QColor(r(128, 127), r(128, 127), r(128, 127), 255),
                                      QPointF(collideable->radius, collideable->radius));
        }
    }

private:
    Entity m_cfg;
    QSize m_size = {0,0};
    int count = 0;
};


// Updates a body's position and rotation.
struct BodySystem : public System {
    void update(EntityManager &es, float dt) override {
        const float fdt = dt;
        es.for_each<Body>([&](Entity, Body &body) {
            body.position += body.direction * fdt;
            body.rotation += body.rotationd * dt;
            body.alpha = std::min(1.0f, body.alpha + fdt);
        });
    }
};


// Bounce bodies off the edge of the screen.
class BounceSystem : public System {
public:
    explicit BounceSystem(Entity cfg) : m_cfg(cfg) {}

    void update(EntityManager &es, float dt) override {
        QSize size = m_cfg.component<UIConfig>()->size;
        es.for_each<Body>([&](Entity, Body &body) {
            if (body.position.x() + body.direction.x() < 0 ||
                    body.position.x() + body.direction.x() >= size.width())
                body.direction.setX(-body.direction.x());
            if (body.position.y() + body.direction.y() < 0 ||
                    body.position.y() + body.direction.y() >= size.height())
                body.direction.setY(-body.direction.y());
        });
    }

private:
    Entity m_cfg;
};


// For any two colliding bodies, destroys the bodies and emits a bunch of bodgy explosion particles.
class ExplosionSystem : public System {
    constexpr static const float AREAFRACTION = 3.0;
public:
    void update(EntityManager &es, float dt) override {
        for (Entity entity : collided) {
            if (!entity.component<Indestructible>()) {
                emit_particles(es, entity);
                entity.destroy();
            }
        }
        collided.clear();
    }

    void emit_particles(EntityManager &es, Entity entity) {
        const Body &body = *entity.component<Body>();
        const Renderable &renderable = *entity.component<Renderable>();
        const Collideable &collideable = *entity.component<Collideable>();
        QColor colour = renderable.color;
        colour.setAlpha(200);

        float area = (M_PI * collideable.radius * collideable.radius) / AREAFRACTION;
        for (int i = 0; i < area; i++) {
            Entity particle = es.create();

            float rotationd = r(720, 180);
            if (std::rand() % 2 == 0) rotationd = -rotationd;

            float offset = r(collideable.radius, 1);
            float angle = r(360) * M_PI / 180.0;
            particle.assign<Body>(
                        body.position + QVector2D(offset * cos(angle), offset * sin(angle)),
                        body.direction + QVector2D(offset * 2 * cos(angle), offset * 2 * sin(angle)),
                        rotationd);

            float radius = r(3, 1);
            particle.assign<Particle>(colour, radius, radius / 2);
        }
    }

    void on_collision(Entity left, Entity right) {
        // Events are immutable, so we can't destroy the entities here. We defer
        // the work until the update loop.
        collided.insert(left);
        collided.insert(right);
    }

private:
    std::unordered_set<Entity> collided;
};



// Determines if two Collideable bodies have collided. If they have it emits a
// CollisionEvent. This is used by ExplosionSystem to create explosion
// particles, but it could be used by a SoundSystem to play an explosion
// sound, etc..
//
// Uses a fairly rudimentary 2D partition system, but performs reasonably well.
class CollisionSystem : public System {
    static const int PARTITIONS = 200;

    struct Candidate {
        QVector2D position;
        float radius;
        Entity entity;
    };

public:
    explicit CollisionSystem(ExplosionSystem *explosions, Entity cfg) : explosions(explosions), m_cfg(cfg) {}

    void update(EntityManager &es, float dt) override {
        m_size.setWidth(m_cfg.component<UIConfig>()->size.width() / PARTITIONS + 1);
        m_size.setHeight(m_cfg.component<UIConfig>()->size.height() / PARTITIONS + 1);
        reset();
        collect(es);
        collide();
    };

private:
    ExplosionSystem *explosions;
    std::vector<std::vector<Candidate>> grid;
    Entity m_cfg;
    QSize m_size;

    void reset() {
        grid.clear();
        grid.resize(m_size.width() * m_size.height());
    }

    void collect(EntityManager &es) {
        es.for_each<Body, Collideable>([&](Entity entity, Body &body, Collideable &collideable) {
            const unsigned int
                    left = static_cast<int>(body.position.x() - collideable.radius) / PARTITIONS,
                    top = static_cast<int>(body.position.y() - collideable.radius) / PARTITIONS,
                    right = static_cast<int>(body.position.x() + collideable.radius) / PARTITIONS,
                    bottom = static_cast<int>(body.position.y() + collideable.radius) / PARTITIONS;
            Candidate candidate {body.position, collideable.radius, entity};
            // Can't call these 'slots' when using Qt
            const unsigned int slts[4] = {
                clamp(left + top * m_size.width()),
                clamp(right + top * m_size.width()),
                clamp(left  + bottom * m_size.width()),
                clamp(right + bottom * m_size.width()),
            };
            grid[slts[0]].push_back(candidate);
            if (slts[0] != slts[1]) grid[slts[1]].push_back(candidate);
            if (slts[1] != slts[2]) grid[slts[2]].push_back(candidate);
            if (slts[2] != slts[3]) grid[slts[3]].push_back(candidate);
        });
    }

    inline unsigned int clamp(unsigned int i) const {
        return std::min(std::max(i, 0U), static_cast<unsigned int>(grid.size()) - 1U);
    }

    void collide() {
        for (const std::vector<Candidate> &candidates : grid) {
            for (const Candidate &left : candidates) {
                for (const Candidate &right : candidates) {
                    if (left.entity == right.entity) continue;
                    if (collided(left, right))
                        explosions->on_collision(left.entity, right.entity);
                }
            }
        }
    }

    bool collided(const Candidate &left, const Candidate &right) {
        return (left.position - right.position).length() < left.radius + right.radius;
    }
};


// Fade out and then remove particles.
class ParticleSystem : public System {
public:
    void update(EntityManager &es, float dt) override {
        es.for_each<Particle>([&](Entity entity, Particle &particle) {
            particle.alpha -= particle.d * dt;
            if (particle.alpha <= 0) {
                entity.destroy();
            } else {
                particle.color.setAlpha(particle.alpha);
            }
        });
    }
};


// Render all particles in one giant vertex array.
class ParticleRenderSystem : public System {
public:
    explicit ParticleRenderSystem(QQuickView *view) : m_view(view)
      , m_particleComponent(new QQmlComponent(m_view->engine(), QUrl("qrc:/Particle.qml")))
    {}

    void update(EntityManager &es, float dt) override {
        QQuickItem *item;
        auto keys = m_particleMap.keys();
        es.for_each<Particle, Body>([&](Entity e, Particle &particle, Body &body) {
            if(!m_particleMap.contains(e.id())) {
                item = qobject_cast<QQuickItem*>(m_particleComponent->create());
                Q_ASSERT(item != nullptr);
                QQmlEngine::setObjectOwnership(item, QQmlEngine::CppOwnership);
                m_particleMap.insert(e.id(), item);
                item->setParentItem(m_view->rootObject());
                item->setPosition(body.position.toPoint());
                item->setProperty("radius", particle.radius);
                item->setProperty("color", particle.color);
            } else {
                item = m_particleMap.value(e.id());
                keys.removeOne(e.id());
                item->setPosition(body.position.toPoint());
//                item->setProperty("color", particle.color);
                item->setOpacity(particle.alpha);
                item->setRotation(body.rotation);
            }
        });
        foreach(auto key, keys) {
            QQuickItem *i = m_particleMap.take(key);
            m_particleMap.remove(key);
            i->deleteLater();
        }
    }
private:
    QQuickView *m_view;
    QQmlComponent *m_particleComponent;
    QMap<entityx::Id, QQuickItem*> m_particleMap;
};

// Render all Renderable entities and draw some informational text.
class RenderSystem : public System {
public:
    explicit RenderSystem(QQuickView *view):
        m_view(view)
    {
        m_items.setItemRoleNames(m_roleNames);
        m_view->rootContext()->setContextProperty("items", &m_items);
    }

    void update(EntityManager &es, float dt) override {
        auto keys = m_itemMap.keys();
        QModelIndex idx;
        QStandardItem *item;
        es.for_each<Body, Renderable>([&](Entity e, Body &body, Renderable &renderable) {
            if(m_itemMap.contains(e.id())) {
                keys.removeAll(e.id());
                item = m_itemMap.value(e.id());
                idx = m_items.indexFromItem(item);
                m_items.setData(idx, body.position, Qt::UserRole + 1);
                m_items.setData(idx, renderable.color, Qt::UserRole + 2);
                m_items.setData(idx, renderable.radius, Qt::UserRole + 3);
                m_items.setData(idx, body.rotation, Qt::UserRole + 4);
            } else {
                //                qDebug() << "Creating new";
                item = new QStandardItem();
                m_itemMap.insert(e.id(), item);
                item->setData(body.position, Qt::UserRole + 1);
                item->setData(renderable.color, Qt::UserRole + 2);
                item->setData(renderable.radius, Qt::UserRole + 3);
                item->setData(body.rotation, Qt::UserRole + 4);
                m_items.appendRow(item);
            }
        });
        foreach(auto key, keys) {
            QStandardItem *item = m_itemMap.take(key);
            auto idx = m_items.indexFromItem(item);
            bool rmv = m_items.removeRow(idx.row());
            Q_ASSERT(rmv);
        }
    }
private:
    QQuickView *m_view;
    QStandardItemModel m_items;
    QHash<int, QByteArray> m_roleNames = {{Qt::UserRole + 1, "position"}
                                          ,{Qt::UserRole + 2, "color"}
                                          ,{Qt::UserRole + 3, "radius"}
                                          ,{Qt::UserRole + 4, "rotation"}};
    QMap<entityx::Id, QStandardItem*> m_itemMap;
};

class Engine : public QObject {
    Q_OBJECT

    Q_PROPERTY(size_t entityCount READ entityCount NOTIFY entityCountChanged)
    Q_PROPERTY(int fps READ fps WRITE setFps NOTIFY fpsChanged)
    Q_PROPERTY(float realFps READ realFps NOTIFY realFpsChanged)
    Q_PROPERTY(int frames READ frames NOTIFY framesChanged)
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)

public:
    explicit Engine(QQuickView *view, bool benchmark = false) : QObject(view)
      , m_cfg(m_entities.create())
      , m_view(view)
    {
        m_systems.push_back(new SpawnSystem(m_cfg));
        m_systems.push_back(new BodySystem());
        m_systems.push_back(new BounceSystem(m_cfg));
        ExplosionSystem *explosions = new ExplosionSystem();
        m_systems.push_back(new CollisionSystem(explosions, m_cfg));
        m_systems.push_back(explosions);
        m_systems.push_back(new ParticleSystem());
        CursorInputSystem *input_system = new CursorInputSystem(m_view, m_entities);
        m_systems.push_back(input_system);
        m_systems.push_back(new CursorPushSystem(input_system->get_cursor_entity()));
        m_systems.push_back(new ParticleRenderSystem(m_view));
        m_systems.push_back(new RenderSystem(m_view));
    }

    EntityManager *em() {
        return &m_entities;
    }

    void update(float dt) {
        m_frames++;
        for (System *system : m_systems) {
            system->update(m_entities, dt);
        }
        entityCountChanged();
        framesChanged();
        m_fpsWindow.prepend(1000./(m_t.elapsed() + 1));
        m_realFps = std::accumulate(m_fpsWindow.begin(), m_fpsWindow.end(), 0.0) / m_fpsWindow.size();;
        if(m_fpsWindow.size() > (m_fps/2 + 1)) {
            m_fpsWindow.resize(m_fps/2 + 1);
        }
        m_t.restart();
        realFpsChanged();
    }

    void sendEvent(QEvent *event) {
      for (auto receiver : m_systems)
        receiver->receive(event);
    }

    Entity cfgEntity() {
        return m_cfg;
    }

    bool running() {
        return m_running;
    }

    size_t entityCount() const
    {
        return m_entities.size();
    }

    int frames() const
    {
        return m_frames;
    }

    int fps() const
    {
        return m_fps;
    }

    float realFps() const
    {
        return m_realFps;
    }

public slots:
    void step(float stepSize) {
        update(stepSize);
    }

    void setFps(int fps)
    {
        if (m_fps == fps)
            return;

        m_fps = fps;
        emit fpsChanged(fps);
    }

    void setRunning(bool running)
    {
        if (m_running == running)
            return;

        m_running = running;
        emit runningChanged(running);
    }

signals:
    void entityCountChanged();
    void framesChanged();
    void realFpsChanged();
    void fpsChanged(int fps);
    void runningChanged(bool running);

protected:
    bool eventFilter(QObject *obj, QEvent *event) {
        if (event->type() == QEvent::MouseMove) {
            sendEvent(event);
        }
        return QObject::eventFilter(obj, event);
    }

private:
    bool m_running = true;
    EntityManager m_entities;
    Entity m_cfg;
    std::vector<System*> m_systems;
    int m_frames = 0;
    int m_fps = 60;
    QQuickView *m_view;
    float m_realFps = m_fps;
    QTime m_t;
    QVector<float> m_fpsWindow;
};


int main(int argc, char **argv) {
    qRegisterMetaType<size_t>("size_t");
    qsrand(QDateTime::currentMSecsSinceEpoch());

    QGuiApplication app(argc, argv);

    QQuickView view;
    Engine engine(&view);

    view.setResizeMode(QQuickView::ResizeMode::SizeRootObjectToView);
    view.rootContext()->setContextProperty("engine", &engine);
    view.setSource(QUrl("qrc:/main.qml"));

    // Must be done before the first update call
    engine.cfgEntity().assign<UIConfig>(view.size());

    QObject::connect(&view, &QQuickView::widthChanged, [&engine](int width){
        engine.cfgEntity().component<UIConfig>()->size.setWidth(width);
    });
    QObject::connect(&view, &QQuickView::heightChanged, [&engine](int height){
        engine.cfgEntity().component<UIConfig>()->size.setHeight(height);
    });

    QTimer engineTimer;
    engineTimer.setTimerType(Qt::TimerType::PreciseTimer);
    engineTimer.setInterval(1000./engine.fps());
    engineTimer.setSingleShot(false);

    QObject::connect(view.engine(), &QQmlEngine::quit, qApp, &QCoreApplication::quit);

    QObject::connect(&engine, &Engine::fpsChanged, [&engine, &engineTimer](int fps){
        engineTimer.setInterval(1000./engine.fps());
    });

    QObject::connect(&engineTimer, &QTimer::timeout, [&engine, &engineTimer](){
        engine.update(1./engine.fps());
    });

    QObject::connect(&engine, &Engine::runningChanged, [&engineTimer](bool running){
        if(running) {
            engineTimer.start();
        } else {
            engineTimer.stop();
        }
    });

    qApp->installEventFilter(&engine);

    if(engine.running()) {
        engineTimer.start();
    }
    view.show();
    return app.exec();
}

#include "example.moc"
