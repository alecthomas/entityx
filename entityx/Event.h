/*
 * Copyright (C) 2012 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#pragma once

#include <stdint.h>
#include <list>
#include <utility>
#include <unordered_map>
#include "entityx/config.h"
#include "entityx/3rdparty/simplesignal.h"
#include "entityx/help/NonCopyable.h"


namespace entityx {


/// Used internally by the EventManager.
class BaseEvent {
 public:
  typedef uint64_t Family;

  virtual ~BaseEvent() {}

  virtual Family my_family() const = 0;

 protected:
  static Family family_counter_;
};


typedef Simple::Signal<void (const BaseEvent*)> EventSignal;
typedef ptr<EventSignal> EventSignalPtr;
typedef weak_ptr<EventSignal> EventSignalWeakPtr;


/**
 * Event types should subclass from this.
 *
 * struct Explosion : public Event<Explosion> {
 *   Explosion(int damage) : damage(damage) {}
 *   int damage;
 * };
 */
template <typename Derived>
class Event : public BaseEvent {
 public:
  /// Used internally for registration.
  static Family family() {
    static Family family = family_counter_++;
    return family;
  }

  virtual Family my_family() const override { return Derived::family(); }
};


class BaseReceiver {
 public:
  virtual ~BaseReceiver() {
    for (auto connection : connections_) {
      auto &ptr = connection.first;
      if (!ptr.expired()) {
        ptr.lock()->disconnect(connection.second);
      }
    }
  }

  // Return number of signals connected to this receiver.
  int connected_signals() const {
    size_t size = 0;
    for (auto connection : connections_) {
      if (!connection.first.expired()) {
        size++;
      }
    }
    return size;
  }

 private:
  friend class EventManager;
  std::list<std::pair<EventSignalWeakPtr, size_t>> connections_;
};


template <typename Derived>
class Receiver : public BaseReceiver {
 public:
  virtual ~Receiver() {}
};


/**
 * Handles event subscription and delivery.
 *
 * Subscriptions are automatically removed when receivers are destroyed..
 */
class EventManager : entityx::help::NonCopyable {
 public:
  EventManager();
  virtual ~EventManager();

  static ptr<EventManager> make() {
    return ptr<EventManager>(new EventManager());
  }

  /**
   * Subscribe an object to receive events of type E.
   *
   * Receivers must be subclasses of Receiver and must implement a receive() method accepting the given event type.
   *
   * eg.
   *
   *     struct ExplosionReceiver : public Receiver<ExplosionReceiver> {
   *       void receive(const Explosion &explosion) {
   *       }
   *     };
   *
   *     ExplosionReceiver receiver;
   *     em.subscribe<Explosion>(receiver);
   */
  template <typename E, typename Receiver>
  void subscribe(Receiver &receiver) {  //NOLINT
    void (Receiver::*receive)(const E &) = &Receiver::receive;
    auto sig = signal_for(E::family());
    auto wrapper = EventCallbackWrapper<E>(std::bind(receive, &receiver, std::placeholders::_1));
    auto connection = sig->connect(wrapper);
    static_cast<BaseReceiver&>(receiver).connections_.push_back(
      std::make_pair(EventSignalWeakPtr(sig), connection));
  }

  void emit(const BaseEvent &event);

  /**
   * Emit an already constructed event.
   */
  template <typename E>
  void emit(ptr<E> event) {
    auto sig = signal_for(E::family());
    sig->emit(static_cast<BaseEvent*>(event.get()));
  }

  /**
   * Emit an event to receivers.
   *
   * This method constructs a new event object of type E with the provided arguments, then delivers it to all receivers.
   *
   * eg.
   *
   * ptr<EventManager> em = new EventManager();
   * em->emit<Explosion>(10);
   *
   */
  template <typename E, typename ... Args>
  void emit(Args && ... args) {
    E event(args ...);
    auto sig = signal_for(E::family());
    sig->emit(static_cast<BaseEvent*>(&event));
  }

  int connected_receivers() const {
    int size = 0;
    for (auto pair : handlers_) {
      size += pair.second->size();
    }
    return size;
  }

 private:
  EventSignalPtr signal_for(int id) {
    auto it = handlers_.find(id);
    if (it == handlers_.end()) {
      EventSignalPtr sig(new EventSignal());
      handlers_.insert(std::make_pair(id, sig));
      return sig;
    }
    return it->second;
  }

  // Functor used as an event signal callback that casts to E.
  template <typename E>
  struct EventCallbackWrapper {
    EventCallbackWrapper(std::function<void(const E &)> callback) : callback(callback) {}
    void operator()(const BaseEvent* event) { callback(*(static_cast<const E*>(event))); }
    std::function<void(const E &)> callback;
  };

  std::unordered_map<int, EventSignalPtr> handlers_;
};

}  // namespace entityx
