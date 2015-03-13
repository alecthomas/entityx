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

#include <cstdint>
#include <cstddef>
#include <vector>
#include <list>
#include <unordered_map>
#include <memory>
#include <utility>
#include <functional>
#include <type_traits>
#include "entityx/config.h"
#include "entityx/3rdparty/simplesignal.h"
#include "entityx/help/NonCopyable.h"
#include "entityx/help/UIDGenerator.h"

namespace entityx {

typedef Simple::Signal<void (const void*)> EventSignal;
typedef std::shared_ptr<EventSignal> EventSignalPtr;
typedef std::weak_ptr<EventSignal> EventSignalWeakPtr;

template <typename E>
using CallbackSignature = std::function<void(const E &)>;

class Receiver {
 public:
  virtual ~Receiver() {
    for (auto connection : connections_) {
      auto &ptr = connection.second.first;
      if (!ptr.expired()) {
        ptr.lock()->disconnect(connection.second.second);
      }
    }
  }

  // Return number of signals connected to this receiver.
  std::size_t connected_signals() const {
    std::size_t size = 0;
    for (auto connection : connections_) {
      if (!connection.second.first.expired()) {
        size++;
      }
    }
    return size;
  }

 private:
  friend class EventManager;
  std::unordered_map<UIDGenerator::Family, std::pair<EventSignalWeakPtr, std::size_t>> connections_;
};


/**
 * Handles event subscription and delivery.
 *
 * Subscriptions are automatically removed when receivers are destroyed..
 */
class EventManager : entityx::help::NonCopyable {
 public:
  EventManager() = default;
  virtual ~EventManager() = default;

  /**
   * Subscribe an object to receive events of type E.
   *
   * Receivers must be subclasses of Receiver and must implement a method accepting the given event type.
   *
   * eg.
   *
   *     struct ExplosionReceiver : public Receiver {
   *       void receive(const Explosion &explosion) {
   *       }
   *     };
   *
   *     ExplosionReceiver receiver;
   *     em.subscribe<Explosion>(receiver);
   */

  template <typename E, typename Receiver>
  void subscribe(Receiver &receiver) {
    void (Receiver::*receive)(const E &) = &Receiver::receive;
    subscribe<E>(receiver, std::bind(receive, &receiver, std::placeholders::_1));
  }

  template <typename E, typename Receiver>
  void subscribe(Receiver &receiver, CallbackSignature<E> callback) {
    static_assert(std::is_base_of<entityx::Receiver, Receiver>::value, "Receiver should inherit from entityx::Receiver");
    assert(callback && "Invalid callback");
    if (callback) {
      auto sig = signal_for(UIDGenerator::get_uid<E>());
      auto wrapper = EventCallbackWrapper<E>(callback);
      auto connection = sig->connect(wrapper);
      entityx::Receiver &base = receiver;
      base.connections_.insert(std::make_pair(UIDGenerator::get_uid<E>(), std::make_pair(EventSignalWeakPtr(sig), connection)));
    }
  }

  /**
   * Unsubscribe an object in order to not receive events of type E anymore.
   *
   * Receivers must have subscribed for event E before unsubscribing from event E.
   *
   */
  template <typename E, typename Receiver>
  void unsubscribe(Receiver &receiver) {
    static_assert(std::is_base_of<entityx::Receiver, Receiver>::value, "Receiver should inherit from entityx::Receiver");
    entityx::Receiver &base = receiver;
    //Assert that it has been subscribed before
    assert(base.connections_.find(UIDGenerator::get_uid<E>()) != base.connections_.end());
    auto pair = base.connections_[UIDGenerator::get_uid<E>()];
    auto connection = pair.second;
    auto &ptr = pair.first;
    if (!ptr.expired()) {
      ptr.lock()->disconnect(connection);
    }
    base.connections_.erase(UIDGenerator::get_uid<E>());
  }

  template <typename E>
  void emit(const E &event) {
    auto sig = signal_for(UIDGenerator::get_uid<E>());
    sig->emit(&event);
  }

  /**
   * Emit an already constructed event.
   */
  template <typename E>
  void emit(std::unique_ptr<E> event) {
    auto sig = signal_for(UIDGenerator::get_uid<E>());
    sig->emit(event.get());
  }

  /**
   * Emit an event to receivers.
   *
   * This method constructs a new event object of type E with the provided arguments, then delivers it to all receivers.
   *
   * eg.
   *
   * std::shared_ptr<EventManager> em = new EventManager();
   * em->emit<Explosion>(10);
   *
   */
  template <typename E, typename ... Args>
  void emit(Args && ... args) {
    // Using 'E event(std::forward...)' causes VS to fail with an internal error. Hack around it.
    E event = E(std::forward<Args>(args) ...);
    auto sig = signal_for(std::size_t(UIDGenerator::get_uid<E>()));
    sig->emit(&event);
  }

  std::size_t connected_receivers() const {
    std::size_t size = 0;
    for (EventSignalPtr handler : handlers_) {
      if (handler) size += handler->size();
    }
    return size;
  }

 private:
  EventSignalPtr &signal_for(std::size_t id) {
    if (id >= handlers_.size())
      handlers_.resize(id + 1);
    if (!handlers_[id])
      handlers_[id] = std::make_shared<EventSignal>();
    return handlers_[id];
  }

  // Functor used as an event signal callback that casts to E.
  template <typename E>
  struct EventCallbackWrapper {
    EventCallbackWrapper(CallbackSignature<E> callback) : callback(callback) {}
    void operator()(const void *event) { callback(*(static_cast<const E*>(event))); }
    CallbackSignature<E> callback;
  };

  std::vector<EventSignalPtr> handlers_;
};

}  // namespace entityx
