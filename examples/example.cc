/**
 * This is an example of using EntityX.
 *
 * It is an SFML2 application that spawns 100 random circles on a 2D plane
 * moving in random directions. If two circles collide they will explode and
 * emit particles.
 *
 * This illustrates a bunch of EC/EntityX concepts:
 *
 * - Separation of data via components.
 * - Separation of logic via systems.
 * - Use of events (colliding bodies trigger a CollisionEvent).
 *
 * Compile with:
 *
 *    c++ -I.. -O3 -std=c++11 -Wall -lsfml-system -lsfml-window -lsfml-graphics example.cc -o example
 */
#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <sstream>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include "SFML/Window.hpp"
#include "SFML/Graphics.hpp"
#include "entityx/entityx.hh"

using std::cerr;
using std::cout;
using std::endl;

namespace ex = entityx;


float length(const sf::Vector2f &v) {
  return std::sqrt(v.x * v.x + v.y * v.y);
}

float r(int a, float b = 0) {
  return static_cast<float>(std::rand() % (a * 1000) + b * 1000) / 1000.0;
}


// A position, direction, and rotation.
struct Body {
  Body(const sf::Vector2f &position, const sf::Vector2f &direction, float rotationd = 0.0)
    : position(position), direction(direction), rotationd(rotationd) {}

  sf::Vector2f position;
  sf::Vector2f direction;
  float rotation = 0.0, rotationd, alpha = 0.0;
};


// An sf::Shape.
using Renderable = std::shared_ptr<sf::Shape>;


// An Indestructible entity can collide but will not be destroyed.
struct Indestructible {};


// A Particle is a coloured object that fades out over a duration.
struct Particle {
  explicit Particle(sf::Color colour, float radius, float duration)
      : colour(colour), radius(radius), alpha(colour.a), d(colour.a / duration) {}

  sf::Color colour;
  float radius, alpha, d;
};


// A collision radius.
struct Collideable {
  explicit Collideable(float radius) : radius(radius) {}

  float radius;
};


using EntityManager = entityx::EntityX<
  entityx::DefaultStorage, 0,
  Body, Renderable, Particle, Collideable, Indestructible>;
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
  virtual void receive(sf::Event event) {}
};


// CursorInputSystem processes user input and applies it to the cursor.
struct CursorInputSystem : System {
public:
  explicit CursorInputSystem(sf::RenderTarget &target, EntityManager &es) {
    // Create the cursor.
    entity = es.create();

    entity.assign<Indestructible>();

    const float radius = 16.0;

    entity.assign<Collideable>(radius);

    entity.assign<Body>(
      sf::Vector2f(sf::Mouse::getPosition()),
      sf::Vector2f(0, 0));

    sf::Shape *shape = new sf::CircleShape(radius);
    shape->setFillColor(sf::Color(255, 255, 255, 128));
    shape->setOrigin(radius, radius);
    entity.assign<Renderable>(shape);
  }

  // Pulse the cursor.
  void update(EntityManager &es, float dt) override {
    const double radius = 16 + std::sin(time * 10.0) * 4;

    sf::CircleShape &circle = static_cast<sf::CircleShape&>(**entity.component<Renderable>());
    entity.component<Collideable>()->radius = radius;
    circle.setRadius(radius);
    circle.setOrigin(radius, radius);
    time += dt;
  }

  void receive(sf::Event event) override {
    if (event.type == sf::Event::MouseMoved) {
      entity.component<Body>()->position = sf::Vector2f(event.mouseMove.x, event.mouseMove.y);
    }
  }

  Entity get_cursor_entity() {
    return entity;
  }

private:
  Entity entity;
  double time = 0;
};


// CursorPushSystem applies momentum to objects surrounding the cursor.
class CursorPushSystem : public System {
public:
  explicit CursorPushSystem(Entity cursor) : cursor(cursor) {}

  void update(EntityManager &es, float dt) override {
    assert(cursor);
    sf::Vector2f cursor_position = cursor.component<Body>()->position;
    es.for_each<Body>([&](Entity entity, Body &body) {
      if (entity == cursor) return;
      sf::Vector2f direction = body.position - cursor_position;
      float distance = length(direction);
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

public:
  explicit SpawnSystem(sf::RenderTarget &target) : size(target.getSize()), count(size.x * size.y * DENSITY) {}

  void update(EntityManager &es, float dt) override {
    int c = 0;
    es.for_each<Collideable>([&](Entity, Collideable&) { c++; });

    for (int i = 0; i < count - c; i++) {
      Entity entity = es.create();

      // Mark as collideable (explosion particles will not be collideable).
      Collideable *collideable = entity.assign<Collideable>(r(10, 5));

      // "Physical" attributes.
      entity.assign<Body>(
        sf::Vector2f(r(size.x), r(size.y)),
        sf::Vector2f(r(100, -50), r(100, -50)));

      // Shape to apply to entity.
      sf::Shape *shape = new sf::CircleShape(collideable->radius);
      shape->setFillColor(sf::Color(r(128, 127), r(128, 127), r(128, 127), 0));
      shape->setOrigin(collideable->radius, collideable->radius);
      entity.assign<Renderable>(shape);
    }
  }

private:
  sf::Vector2u size;
  int count;
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
  };
};


// Bounce bodies off the edge of the screen.
class BounceSystem : public System {
public:
  explicit BounceSystem(sf::RenderTarget &target) : size(target.getSize()) {}

  void update(EntityManager &es, float dt) override {
    es.for_each<Body>([&](Entity, Body &body) {
      if (body.position.x + body.direction.x < 0 ||
          body.position.x + body.direction.x >= size.x)
        body.direction.x = -body.direction.x;
      if (body.position.y + body.direction.y < 0 ||
          body.position.y + body.direction.y >= size.y)
        body.direction.y = -body.direction.y;
    });
  }

private:
  sf::Vector2u size;
};


// For any two colliding bodies, destroys the bodies and emits a bunch of bodgy explosion particles.
class ExplosionSystem : public System {
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
    sf::Color colour = renderable->getFillColor();
    colour.a = 200;

    float area = (M_PI * collideable.radius * collideable.radius) / 3.0;
    for (int i = 0; i < area; i++) {
      Entity particle = es.create();

      float rotationd = r(720, 180);
      if (std::rand() % 2 == 0) rotationd = -rotationd;

      float offset = r(collideable.radius, 1);
      float angle = r(360) * M_PI / 180.0;
      particle.assign<Body>(
        body.position + sf::Vector2f(offset * cos(angle), offset * sin(angle)),
        body.direction + sf::Vector2f(offset * 2 * cos(angle), offset * 2 * sin(angle)),
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
    sf::Vector2f position;
    float radius;
    Entity entity;
  };

public:
  explicit CollisionSystem(ExplosionSystem *explosions, sf::RenderTarget &target) : explosions(explosions), size(target.getSize()) {
    size.x = size.x / PARTITIONS + 1;
    size.y = size.y / PARTITIONS + 1;
  }

  void update(EntityManager &es, float dt) override {
    reset();
    collect(es);
    collide();
  };

private:
  ExplosionSystem *explosions;
  std::vector<std::vector<Candidate>> grid;
  sf::Vector2u size;

  void reset() {
    grid.clear();
    grid.resize(size.x * size.y);
  }

  void collect(EntityManager &es) {
    es.for_each<Body, Collideable>([&](Entity entity, Body &body, Collideable &collideable) {
      const unsigned int
        left = static_cast<int>(body.position.x - collideable.radius) / PARTITIONS,
        top = static_cast<int>(body.position.y - collideable.radius) / PARTITIONS,
        right = static_cast<int>(body.position.x + collideable.radius) / PARTITIONS,
        bottom = static_cast<int>(body.position.y + collideable.radius) / PARTITIONS;
      Candidate candidate {body.position, collideable.radius, entity};
      const unsigned int slots[4] = {
        clamp(left + top * size.x),
        clamp(right + top * size.x),
        clamp(left  + bottom * size.x),
        clamp(right + bottom * size.x),
      };
      grid[slots[0]].push_back(candidate);
      if (slots[0] != slots[1]) grid[slots[1]].push_back(candidate);
      if (slots[1] != slots[2]) grid[slots[2]].push_back(candidate);
      if (slots[2] != slots[3]) grid[slots[3]].push_back(candidate);
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
    return length(left.position - right.position) < left.radius + right.radius;
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
        particle.colour.a = particle.alpha;
      }
    });
  }
};


// Render all particles in one giant vertex array.
class ParticleRenderSystem : public System {
public:
  explicit ParticleRenderSystem(sf::RenderTarget &target) : target(target) {}

  void update(EntityManager &es, float dt) override {
    sf::VertexArray vertices(sf::Quads);
    es.for_each<Particle, Body>([&](Entity entity, Particle &particle, Body &body) {
      (void)entity;
      const float r = particle.radius;
      // Spin the particles.
      sf::Transform transform;
      transform.rotate(body.rotation);
      vertices.append(sf::Vertex(body.position + transform.transformPoint(sf::Vector2f(-r, -r)), particle.colour));
      vertices.append(sf::Vertex(body.position + transform.transformPoint(sf::Vector2f(r, -r)), particle.colour));
      vertices.append(sf::Vertex(body.position + transform.transformPoint(sf::Vector2f(r, r)), particle.colour));
      vertices.append(sf::Vertex(body.position + transform.transformPoint(sf::Vector2f(-r, r)), particle.colour));
    });
    target.draw(vertices);
  }
private:
  sf::RenderTarget &target;
};


// Render all Renderable entities and draw some informational text.
class RenderSystem : public System {
public:
  explicit RenderSystem(sf::RenderTarget &target, sf::Font &font) : target(target) {
    text.setFont(font);
    text.setPosition(sf::Vector2f(2, 2));
    text.setCharacterSize(18);
    text.setFillColor(sf::Color::White);
  }

  void update(EntityManager &es, float dt) override {
    es.for_each<Body, Renderable>([&](Entity, Body &body, Renderable &renderable) {
      sf::Color fillColor = renderable->getFillColor();
      fillColor.a = sf::Uint8(body.alpha * 255);
      renderable->setFillColor(fillColor);
      renderable->setPosition(body.position);
      renderable->setRotation(body.rotation);
      target.draw(*renderable);
    });
    last_update += dt;
    frame_count++;
    if (last_update >= 0.5) {
      std::ostringstream out;
      const double fps = frame_count / last_update;
      out << es.size() << " entities (" << static_cast<int>(fps) << " fps)";
      text.setString(out.str());
      last_update = 0.0;
      frame_count = 0.0;
    }
    target.draw(text);
  }

private:
  double last_update = 0.0;
  double frame_count = 0.0;
  sf::RenderTarget &target;
  sf::Text text;
};


class Application {
public:
  explicit Application(sf::RenderTarget &target, sf::Font &font, bool benchmark) {
    systems.push_back(new SpawnSystem(target));
    systems.push_back(new BodySystem());
    systems.push_back(new BounceSystem(target));
    ExplosionSystem *explosions = new ExplosionSystem();
    systems.push_back(new CollisionSystem(explosions, target));
    systems.push_back(explosions);
    systems.push_back(new ParticleSystem());
    CursorInputSystem *input_system = new CursorInputSystem(target, entities);
    systems.push_back(input_system);
    systems.push_back(new CursorPushSystem(input_system->get_cursor_entity()));

    if (!benchmark) {
      systems.push_back(new ParticleRenderSystem(target));
      systems.push_back(new RenderSystem(target, font));
    }
  }

  void update(float dt) {
    for (System *system : systems)
      system->update(entities, dt);
  }

  void event(sf::Event event) {
    switch (event.type) {
      case sf::Event::Closed:
      case sf::Event::KeyPressed:
        running_ = false;
        break;

      default:
        break;
    }
    for (auto receiver : systems)
      receiver->receive(event);
  }

  int count() const {
    return entities.size();
  }

  bool running() {
    return running_;
  }

private:
  bool running_ = true;
  EntityManager entities;
  std::vector<System*> systems;
};



int main(int argc, const char **argv) {
  std::srand(std::time(nullptr));

  const bool benchmark = (argc == 2 && std::string(argv[1]) == "benchmark");

  sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "EntityX Example", sf::Style::Fullscreen);
  window.setMouseCursorVisible(false);

  sf::Font font;
  if (!font.loadFromFile("LiberationSans-Regular.ttf")) {
    cerr << "error: failed to load LiberationSans-Regular.ttf" << endl;
    return 1;
  }

  Application app(window, font, benchmark);

  sf::Clock total;
  float frames = 0;
  std::int64_t entities = 0;
  sf::Clock clock;
  while (app.running()) {
    sf::Event event;
    while (window.pollEvent(event))app.event(event);

    window.clear();
    sf::Time elapsed = clock.restart();
    app.update(elapsed.asSeconds());
    window.display();
    frames++;
    entities += app.count();
  }
  float elapsed = total.restart().asSeconds();
  cout << elapsed << " seconds, " << frames << " total frames, " << frames / elapsed << " fps " << endl;
  cout << entities / frames << " average entities per frame, " << std::int64_t(entities / elapsed) << " entities per second" << endl;
}
