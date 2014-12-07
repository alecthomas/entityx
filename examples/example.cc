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
 *    c++ -O3 -std=c++11 -Wall -lsfml-system -lsfml-window -lsfml-graphics -lentityx example.cc -o example
 */
#include <cmath>
#include <unordered_set>
#include <sstream>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <entityx/entityx.h>

using std::cerr;
using std::cout;
using std::endl;

namespace ex = entityx;

namespace std {
template <>
struct hash<ex::Entity> {
  std::size_t operator()(const ex::Entity& k) const { return k.id().id(); }
};
}


float r(int a, float b = 0) {
  return static_cast<float>(std::rand() % (a * 1000) + b * 1000) / 1000.0;
}


struct Body : ex::Component<Body> {
  Body(const sf::Vector2f &position, const sf::Vector2f &direction, float rotationd = 0.0)
    : position(position), direction(direction), rotationd(rotationd) {}

  sf::Vector2f position;
  sf::Vector2f direction;
  float rotation = 0.0, rotationd;
};


struct Renderable : ex::Component<Renderable> {
  explicit Renderable(std::unique_ptr<sf::Shape> shape) : shape(std::move(shape)) {}

  std::unique_ptr<sf::Shape> shape;
};


struct Fadeable : ex::Component<Fadeable> {
  explicit Fadeable(sf::Uint8 alpha, float duration) : alpha(alpha), d(alpha / duration) {}

  float alpha, d;
};


struct Collideable : ex::Component<Collideable> {
  explicit Collideable(float radius) : radius(radius) {}

  float radius;
};


// Emitted when two entities collide.
struct CollisionEvent : public ex::Event<CollisionEvent> {
  CollisionEvent(ex::Entity left, ex::Entity right) : left(left), right(right) {}

  ex::Entity left, right;
};


// Updates a body's position and rotation.
struct BodySystem : public ex::System<BodySystem> {
  void update(ex::EntityManager &es, ex::EventManager &events, ex::TimeDelta dt) override {
    Body::Handle body;
    for (ex::Entity entity : es.entities_with_components(body)) {
      body->position += body->direction * static_cast<float>(dt);
      body->rotation += body->rotationd * dt;
    }
  };
};


// Fades out the alpha value of any Renderable and Fadeable entity. Once the
// object has completely faded out it is destroyed.
struct FadeOutSystem : public ex::System<FadeOutSystem> {
  void update(ex::EntityManager &es, ex::EventManager &events, ex::TimeDelta dt) override {
    Fadeable::Handle fade;
    Renderable::Handle renderable;
    for (ex::Entity entity : es.entities_with_components(fade, renderable)) {
      fade->alpha -= fade->d * dt;
      if (fade->alpha <= 0) {
        entity.destroy();
      } else {
        sf::Color color = renderable->shape->getFillColor();
        color.a = fade->alpha;
        renderable->shape->setFillColor(color);
      }
    }
  }
};


// Bounce bodies off the edge of the screen.
class BounceSystem : public ex::System<BounceSystem> {
public:
  explicit BounceSystem(sf::RenderTarget &target) : size(target.getSize()) {}

  void update(ex::EntityManager &es, ex::EventManager &events, ex::TimeDelta dt) override {
    Body::Handle body;
    for (ex::Entity entity : es.entities_with_components(body)) {
      if (body->position.x + body->direction.x < 0 ||
          body->position.x + body->direction.x >= size.x)
        body->direction.x = -body->direction.x;
      if (body->position.y + body->direction.y < 0 ||
          body->position.y + body->direction.y >= size.y)
        body->direction.y = -body->direction.y;
    }
  }

private:
  sf::Vector2u size;
};


// Determines if two Collideable bodies have collided. If they have it emits a
// CollisionEvent. This is used by ExplosionSystem to create explosion
// particles, but it could be used by a SoundSystem to play an explosion
// sound, etc..
//
// Uses a fairly rudimentary 2D partition system, but performs reasonably well.
class CollisionSystem : public ex::System<CollisionSystem> {
  static const int PARTITIONS = 200;

  struct Candidate {
    sf::Vector2f position;
    float radius;
    ex::Entity entity;
  };

public:
  explicit CollisionSystem(sf::RenderTarget &target) : size(target.getSize()) {
    size.x = size.x / PARTITIONS + 1;
    size.y = size.y / PARTITIONS + 1;
  }

  void update(ex::EntityManager &es, ex::EventManager &events, ex::TimeDelta dt) override {
    reset();
    collect(es);
    collide(events);
  };

private:
  std::vector<std::vector<Candidate>> grid;
  sf::Vector2u size;

  void reset() {
    grid.clear();
    grid.resize(size.x * size.y);
  }

  void collect(ex::EntityManager &entities) {
    Body::Handle body;
    Collideable::Handle collideable;
    for (ex::Entity entity : entities.entities_with_components(body, collideable)) {
      unsigned int
          left = static_cast<int>(body->position.x - collideable->radius) / PARTITIONS,
          top = static_cast<int>(body->position.y - collideable->radius) / PARTITIONS,
          right = static_cast<int>(body->position.x + collideable->radius) / PARTITIONS,
          bottom = static_cast<int>(body->position.y + collideable->radius) / PARTITIONS;
        Candidate candidate {body->position, collideable->radius, entity};
        unsigned int slots[4] = {
          left + top * size.x,
          right + top * size.x,
          left  + bottom * size.x,
          right + bottom * size.x,
        };
        grid[slots[0]].push_back(candidate);
        if (slots[0] != slots[1]) grid[slots[1]].push_back(candidate);
        if (slots[1] != slots[2]) grid[slots[2]].push_back(candidate);
        if (slots[2] != slots[3]) grid[slots[3]].push_back(candidate);
    }
  }

  void collide(ex::EventManager &events) {
    for (const std::vector<Candidate> &candidates : grid) {
      for (const Candidate &left : candidates) {
        for (const Candidate &right : candidates) {
          if (left.entity == right.entity) continue;
          if (collided(left, right))
            events.emit<CollisionEvent>(left.entity, right.entity);
        }
      }
    }
  }

  float length(const sf::Vector2f &v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
  }

  bool collided(const Candidate &left, const Candidate &right) {
    return length(left.position - right.position) < left.radius + right.radius;
  }
};


// For any two colliding bodies, destroys the bodies and emits a bunch of bodgy explosion particles.
class ExplosionSystem : public ex::System<ExplosionSystem>, public ex::Receiver<ExplosionSystem> {
public:
  void configure(ex::EventManager &events) override {
    events.subscribe<CollisionEvent>(*this);
  }

  void update(ex::EntityManager &es, ex::EventManager &events, ex::TimeDelta dt) override {
    for (ex::Entity entity : collided) {
      emit_particles(es, entity);
      entity.destroy();
    }
    collided.clear();
  }

  void emit_particles(ex::EntityManager &es, ex::Entity entity) {
    Body::Handle body = entity.component<Body>();
    Renderable::Handle renderable = entity.component<Renderable>();
    Collideable::Handle collideable = entity.component<Collideable>();
    sf::Color colour = renderable->shape->getFillColor();
    colour.a = 200;

    float area = (M_PI * collideable->radius * collideable->radius) / 3.0;
    for (int i = 0; i < area; i++) {
      ex::Entity particle = es.create();

      float rotationd = r(720, 180);
      if (std::rand() % 2 == 0) rotationd = -rotationd;

      particle.assign<Body>(
        body->position + sf::Vector2f(r(collideable->radius * 2, -collideable->radius), r(collideable->radius * 2, -collideable->radius)),
        body->direction + sf::Vector2f(r(50, -25), r(100, -50)),
        rotationd);

      float radius = r(3, 1);
      std::unique_ptr<sf::Shape> shape(new sf::RectangleShape(sf::Vector2f(radius * 2, radius * 2)));
      shape->setFillColor(colour);
      shape->setOrigin(radius, radius);
      particle.assign<Renderable>(std::move(shape));

      particle.assign<Fadeable>(colour.a, radius / 2);
    }
  }

  void receive(const CollisionEvent &collision) {
    // Events are immutable, so we can't destroy the entities here. We defer
    // the work until the update loop.
    collided.insert(collision.left);
    collided.insert(collision.right);
  }

private:
  std::unordered_set<ex::Entity> collided;
};


// Render all Renderable entities and draw some informational text.
class RenderSystem  :public ex::System<RenderSystem> {
public:
  explicit RenderSystem(sf::RenderTarget &target, sf::Font &font) : target(target) {
    text.setFont(font);
    text.setPosition(sf::Vector2f(2, 2));
    text.setCharacterSize(18);
    text.setColor(sf::Color::White);
  }

  void update(ex::EntityManager &es, ex::EventManager &events, ex::TimeDelta dt) override {
    Body::Handle body;
    Renderable::Handle renderable;
    for (ex::Entity entity : es.entities_with_components(body, renderable)) {
      renderable->shape->setPosition(body->position);
      renderable->shape->setRotation(body->rotation);
      target.draw(*renderable->shape.get());
    }
    last_update += dt;
    if (last_update >= 0.5) {
      std::ostringstream out;
      out << es.size() << " entities (" << static_cast<int>(1.0 / dt) << " fps)";
      text.setString(out.str());
      last_update = 0.0;
    }
    target.draw(text);
  }

private:
  float last_update = 0.0;
  sf::RenderTarget &target;
  sf::Text text;
};


class Application : public ex::EntityX {
public:
  explicit Application(sf::RenderTarget &target, sf::Font &font) {
    systems.add<BodySystem>();
    systems.add<FadeOutSystem>();
    systems.add<BounceSystem>(target);
    systems.add<CollisionSystem>(target);
    systems.add<ExplosionSystem>();
    systems.add<RenderSystem>(target, font);
    systems.configure();

    sf::Vector2u size = target.getSize();
    for (int i = 0; i < 500; i++) {
      ex::Entity entity = entities.create();

      // Mark as collideable (explosion particles will not be collideable).
      Collideable::Handle collideable = entity.assign<Collideable>(r(10, 5));

      // "Physical" attributes.
      entity.assign<Body>(
        sf::Vector2f(r(size.x), r(size.y)),
        sf::Vector2f(r(100, -50), r(100, -50)));

      // Shape to apply to entity.
      std::unique_ptr<sf::Shape> shape(new sf::CircleShape(collideable->radius));
      shape->setFillColor(sf::Color(r(128, 127), r(128, 127), r(128, 127)));
      shape->setOrigin(collideable->radius, collideable->radius);
      entity.assign<Renderable>(std::move(shape));
    }
  }

  void update(ex::TimeDelta dt) {
    systems.update<BodySystem>(dt);
    systems.update<FadeOutSystem>(dt);
    systems.update<BounceSystem>(dt);
    systems.update<CollisionSystem>(dt);
    systems.update<ExplosionSystem>(dt);
    systems.update<RenderSystem>(dt);
  }
};



int main() {
  std::srand(std::time(nullptr));

  sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "EntityX Example", sf::Style::Fullscreen);
  sf::Font font;
  if (!font.loadFromFile("LiberationSans-Regular.ttf")) {
    cerr << "error: failed to load LiberationSans-Regular.ttf" << endl;
    return 1;
  }

  Application app(window, font);

  sf::Clock clock;
  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      switch (event.type) {
        case sf::Event::Closed:
        case sf::Event::KeyPressed:
          window.close();
          break;

        default:
          break;
      }
    }

    window.clear();
    sf::Time elapsed = clock.restart();
    app.update(elapsed.asSeconds());
    window.display();
  }
}
