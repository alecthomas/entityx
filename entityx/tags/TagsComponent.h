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

#include <string>
#include <unordered_set>
#include "entityx/Entity.h"

namespace entityx {
namespace tags {

/**
 * Allow entities to be tagged with strings.
 */
class TagsComponent : public Component<TagsComponent> {
  struct TagsPredicate {
    explicit TagsPredicate(const std::string &tag) : tag(tag) {}

    bool operator() (ptr<EntityManager> manager, Entity::Id id) {
      auto tags = manager->component<TagsComponent>(id);
      return tags && tags->tags.find(tag) != tags->tags.end();
    }

    std::string tag;
  };

 public:
  /**
   * Construct a new TagsComponent with the given tags.
   *
   * eg. TagsComponent tags("a", "b", "c");
   */
  template <typename ... Args>
  TagsComponent(const std::string &tag, const Args & ... tags) {
    set_tags(tag, tags ...);
  }

  /**
   * Filter the provided view to only those entities with the given tag.
   */
  static EntityManager::View view(const EntityManager::View &view, const std::string &tag) {
    return EntityManager::View(view, TagsPredicate(tag));
  }

  std::unordered_set<std::string> tags;

 private:
  template <typename ... Args>
  void set_tags(const std::string &tag1, const std::string &tag2, const Args & ... tags) {
    this->tags.insert(tag1);
    set_tags(tag2, tags ...);
  }

  void set_tags(const std::string &tag) {
    tags.insert(tag);
  }
};

}  // namespace tags
}  // namespace entityx
