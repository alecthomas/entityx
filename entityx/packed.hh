#pragma once


#include <algorithm>
#include <cstdint>
#include <vector>
#include <utility>
#include <cstring>
#include <numeric>


namespace entityx {

template <typename C>
struct ComponentIndexPair {
  std::uint32_t *first;
  C *second;
};


template <typename IdType, typename CType>
class ComponentIterator : public std::iterator<std::random_access_iterator_tag, ComponentIndexPair<CType>, int> {
public:
  typedef ComponentIndexPair<CType> Pair;

  explicit ComponentIterator(IdType *id, CType *component) : pair_{id, component} {}

  ComponentIterator &operator ++() { ++pair_.first; ++pair_.second; return *this; }

  const Pair &operator * () { return pair_; }
  bool operator != (const ComponentIterator &other) const { return pair_.first != other.pair_.first || pair_.second != other.pair_.second; }

private:
  Pair pair_;
};

// Sorting two vectors in parallel.
// http://stackoverflow.com/questions/17074324/how-can-i-sort-two-vectors-in-the-same-way-with-criteria-that-uses-only-one-of
template <typename T, typename Compare>
std::vector<int> sort_permutation(std::vector<T> const& vec, Compare compare) {
    std::vector<int> p(vec.size());
    std::iota(p.begin(), p.end(), 0);
    std::sort(p.begin(), p.end(),
        [&](int i, int j){ return compare(vec[i], vec[j]); });
    return p;
}


template <typename T>
std::vector<T> apply_permutation(std::vector<T> const& vec, std::vector<int> const& p) {
    std::vector<T> sorted_vec(p.size());
    std::transform(p.begin(), p.end(), sorted_vec.begin(), [&](int i){ return vec[i]; });
    return sorted_vec;
}


template <typename C>
class PackedColumn {
private:
  struct Component { std::uint8_t data[sizeof(C)]; };
  static_assert(sizeof(Component) == sizeof(C), "must be same size");

public:
  typedef entityx::ComponentIndexPair<C> Pair;
  typedef ComponentIterator<std::uint32_t, C> iterator;
  typedef ComponentIterator<const std::uint32_t, const C> const_iterator;

  PackedColumn() {}

  iterator begin() { return iterator(&ids_.front(), reinterpret_cast<C*>(&components_.front())); }
  iterator end() { return iterator(&ids_.back() + 1, reinterpret_cast<C*>(&components_.back()) + 1); }
  const_iterator begin() const { return iterator(&ids_.front(), reinterpret_cast<const C*>(&components_.front())); }
  const_iterator end() const { return iterator(&ids_.back() + 1, reinterpret_cast<const C*>(&components_.back()) + 1); }

  void optimize() {
    // Sorting two collections in parallel is difficult in c++ :(
    const std::vector<int> perm = sort_permutation(ids_, [](std::uint32_t a, std::uint32_t b) { return a < b; });
    ids_ = apply_permutation(ids_, perm);
    components_ = apply_permutation(components_, perm);
  }

  template <typename ... Args>
  void create(const std::uint32_t entity, Args && ... args) {
    alloc(entity);
    new (get(entity)) C(std::forward<Args>(args) ...);
  }

  void destroy(const std::uint32_t entity) {
    // Component indices.
    const int dest = index(entity);
    const int source = static_cast<int>(components_.size()) - 1;
    // Call destructor.
    reinterpret_cast<C*>(components_[dest].data)->~C();

    // Maybe move tail.
    if (source != dest) {
      index_[ids_[source]] = index_[entity];
      ids_[dest] = ids_[source];
      std::memcpy(components_[dest].data, components_[source].data, sizeof(C));
    }

    // Truncate vectors.
    components_.resize(source);
    ids_.resize(source);
  }

  C *get(const std::uint32_t entity) {
    return reinterpret_cast<C*>(components_[index(entity)].data);
  }

private:
  int index(const std::uint32_t entity) {
    assert(entity < index_.size());
    assert(index_[entity] > 0);
    assert(ids_[index_[entity] - 1] == entity);
    return index_[entity] - 1;
  }

  void alloc(const std::uint32_t entity) {
    assert(entity >= index_.size() || index_[entity] == 0);
    if (entity >= index_.size())
      index_.resize(entity + 1);
    index_[entity] = components_.size() + 1;
    components_.push_back(Component());
    ids_.push_back(entity);
  }

  std::vector<int> index_;            // 1-based mapping from entityx::Id to slot in components_.
                                      // 0 indicates component is unassigned.
  std::vector<std::uint32_t> ids_;    // Reverse mapping from component_ slot to entity ID.
  std::vector<Component> components_;
};


}  // namespace entityx
