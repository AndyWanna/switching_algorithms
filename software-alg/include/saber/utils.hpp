// utils.hpp header file
#ifndef _UTILS_HPP_
#define _UTILS_HPP_

#include <iostream>
#include <vector>
#include <numeric>
#include <queue>
#include <cmath>
#include <iomanip>      // std::setw
#include <iostream>
#include <map>
#include <cassert>

// Overloaded functions to make printing std::vector easier
// ////////////////////////////////////////////////////////
// overload operator<< for std::pair
template<typename T, typename S>
inline std::ostream& operator<<(std::ostream&os, const std::pair<T, S> &p) {
  os << "(" << p.first << ", " << p.second << ")";
  return os;
}
// overload operator<< for std::vector
template<typename T>
inline std::ostream &operator<<(std::ostream &os, const std::vector<T> &v) {
  for (const auto &vv : v) os << vv << " ";
  return os;
}

// overload operator<< for std::queue, which queues pointers
template<typename T>
inline std::ostream &operator<<(std::ostream &os, const std::queue<T *> &q) {
  auto q_bk = q;
  int cnt = 1;
  while (!q_bk.empty()) {
    os << "Item " << std::to_string(cnt) << " : \n{\n";
    os << *q_bk.front() << "\n}\n";
    q_bk.pop();
    ++cnt;
  }
  return os;
}


namespace saber {
// Classes to support fixed width printing
class StdOutUtil {
 public:
  //! fixed width print : base case
  template<const int width, const int precision, typename T>
  static void print_fw(const T &t) {
    std::cout << std::fixed << std::setw(width) << std::setprecision(precision);
    std::cout << t;
  }
  //! fixed width print : recursion
  template<const int width, const int precision, typename T, typename... Ts>
  static void print_fw(const T &t, const Ts &... ts) {
    std::cout << std::fixed << std::setw(width) << std::setprecision(precision);
    std::cout << t;
    print_fw<width, precision>(ts...);
  }
};
/*
class SimUtil {
 public:
  static int next;
  static std::vector<unsigned> seeds;
  static unsigned get_seed() {
    assert(!seeds.empty());
    return seeds[(next++) % seeds.size()];
  }
};
*/

// APIs for dealing with vector-based binary search trees
class BST {
 private:
  template<typename T>
  static int _upper_bound_search(const std::vector<T> &bst, int current, double bound) {
    int left = 2 * current;
    int right = 2 * current + 1;
    if (left >= bst.size()) return current;
    if (bound < bst[left]) return _upper_bound_search(bst, left, bound);
    else return _upper_bound_search(bst, right, bound - bst[left]);
  }
 public:
  template<typename T>
  static int upper_bound(const std::vector<T> &bst, double bound) {
    if (bst[1] == 0) return -1;
    assert(bound >= 0 && bound <= bst[1]);
    return _upper_bound_search(bst, 1, bound);
  }
  template<typename T>
  static T remove(std::vector<T> &bst, int i) {
    T old = bst[i];
    update(bst, i, -old);
    return old;
  }
  template<typename T>
  static void update(std::vector<T> &bst, int i) {
    update(bst, i, 1);
  }
  template<typename T>
  static void update(std::vector<T> &bst, int i, const T &delta) {
    assert (bst[i] + delta >= 0 && "Now we only support non-negative numbers");
    bst[i] += delta;
    if (i == 1) return;
    update(bst, i / 2, delta);
  }
  static size_t nearest_power_of_two(size_t n) {
    return (1u << ((int) (std::ceil(std::log2((double) n)))));
  }
  template<typename T>
  static std::vector<T> create(int num_lefts) {
    assert (num_lefts > 0);
    std::vector<T> bst(2 * nearest_power_of_two(num_lefts), 0);
    return bst;
  }
};// BST

// APIs for permutation/matching validity checking
// ///////////////////////////////////////////////////////////////////////////////
inline bool is_a_matching(const std::vector<int> &perm, size_t n, int free = -1) {
  if (n > 100 * perm.size()) {
    std::map<int, int> back_map;
    for (int i = 0; i < perm.size(); ++i) {
      if (perm[i] == free) continue;
      assert(perm[i] >= 0 && perm[i] < n);
      if (back_map.count(perm[i])) return false;
      else {
        back_map[perm[i]] = i;
      }
    }
  } else {
    std::vector<int> back_vec(n, -1);
    for (int i = 0; i < perm.size(); ++i) {
      if (perm[i] == free) continue;
      assert(perm[i] >= 0 && perm[i] < n);
      if (back_vec[perm[i]] != -1) return false;
      else {
        back_vec[perm[i]] = i;
      }
    }
  }
  return true;
}
// 
inline bool is_a_matching(const std::vector<int> &perm, int free = -1) {
  return is_a_matching(perm, perm.size(), free);
}
} // namespace saber

#endif //_UTILS_HPP_
