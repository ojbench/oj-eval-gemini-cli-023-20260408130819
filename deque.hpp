#ifndef SJTU_DEQUE_HPP
#define SJTU_DEQUE_HPP

#include "exceptions.hpp"
#include <cstddef>
#include <utility>

namespace sjtu {

template <class T> class deque {
private:
    static const int MAX_CAP = 512;

    struct Block {
        char storage[MAX_CAP * sizeof(T)];
        int head;
        int sz;
        Block* prev;
        Block* next;

        Block() : head(0), sz(0), prev(nullptr), next(nullptr) {}

        ~Block() {
            for (int i = 0; i < sz; ++i) {
                ptr(i)->~T();
            }
        }

        T* ptr(int i) {
            return reinterpret_cast<T*>(storage + ((head + i) % MAX_CAP) * sizeof(T));
        }
        const T* ptr(int i) const {
            return reinterpret_cast<const T*>(storage + ((head + i) % MAX_CAP) * sizeof(T));
        }

        void push_back(const T& val) {
            new (ptr(sz)) T(val);
            sz++;
        }

        void push_front(const T& val) {
            head = (head - 1 + MAX_CAP) % MAX_CAP;
            new (ptr(0)) T(val);
            sz++;
        }

        void pop_back() {
            ptr(sz - 1)->~T();
            sz--;
        }

        void pop_front() {
            ptr(0)->~T();
            head = (head + 1) % MAX_CAP;
            sz--;
        }

        void insert(int pos, const T& val) {
            if (pos == 0) {
                push_front(val);
                return;
            }
            if (pos == sz) {
                push_back(val);
                return;
            }
            if (pos < sz / 2) {
                head = (head - 1 + MAX_CAP) % MAX_CAP;
                new (ptr(0)) T(*ptr(1));
                for (int i = 1; i < pos; ++i) {
                    *ptr(i) = *ptr(i + 1);
                }
                *ptr(pos) = val;
            } else {
                new (ptr(sz)) T(*ptr(sz - 1));
                for (int i = sz - 1; i > pos; --i) {
                    *ptr(i) = *ptr(i - 1);
                }
                *ptr(pos) = val;
            }
            sz++;
        }

        void erase(int pos) {
            if (pos == 0) {
                pop_front();
                return;
            }
            if (pos == sz - 1) {
                pop_back();
                return;
            }
            if (pos < sz / 2) {
                for (int i = pos; i > 0; --i) {
                    *ptr(i) = *ptr(i - 1);
                }
                pop_front();
            } else {
                for (int i = pos; i < sz - 1; ++i) {
                    *ptr(i) = *ptr(i + 1);
                }
                pop_back();
            }
        }

        Block* split() {
            Block* nb = new Block();
            int half = sz / 2;
            for (int i = half; i < sz; ++i) {
                nb->push_back(*ptr(i));
            }
            while (sz > half) {
                pop_back();
            }
            nb->next = next;
            nb->prev = this;
            if (next) next->prev = nb;
            next = nb;
            return nb;
        }

        void merge(Block* other) {
            for (int i = 0; i < other->sz; ++i) {
                push_back(*other->ptr(i));
            }
            next = other->next;
            if (next) next->prev = this;
            delete other;
        }

        void borrow_from_next(Block* nb) {
            push_back(*nb->ptr(0));
            nb->pop_front();
        }

        void borrow_from_prev(Block* pb) {
            push_front(*pb->ptr(pb->sz - 1));
            pb->pop_back();
        }
    };

    Block* head_block;
    Block* tail_block;
    size_t total_size;

    void maintain_after_insert(Block* b) {
        if (b->sz == MAX_CAP) {
            Block* nb = b->split();
            if (b == tail_block) tail_block = nb;
        }
    }

    void maintain_after_erase(Block* b) {
        if (b->sz >= MAX_CAP / 2) return;
        if (b->next && b->sz + b->next->sz < MAX_CAP) {
            Block* nb = b->next;
            if (nb == tail_block) tail_block = b;
            b->merge(nb);
        } else if (b->prev && b->sz + b->prev->sz < MAX_CAP) {
            Block* pb = b->prev;
            if (b == tail_block) tail_block = pb;
            pb->merge(b);
        } else if (b->next) {
            b->borrow_from_next(b->next);
        } else if (b->prev) {
            b->borrow_from_prev(b->prev);
        }
        
        if (head_block == tail_block && head_block->sz == 0) {
            delete head_block;
            head_block = tail_block = nullptr;
        }
    }

public:
  class const_iterator;
  class iterator {
    friend class deque;
    friend class const_iterator;
  private:
    Block* block;
    int idx;
    deque* deque_ptr;
  public:
    iterator(Block* b = nullptr, int i = 0, deque* d = nullptr) : block(b), idx(i), deque_ptr(d) {}
    
    iterator operator+(const int &n) const {
        if (n < 0) return operator-(-n);
        int global_idx = deque_ptr->get_global_index(*this) + n;
        return deque_ptr->get_iterator(global_idx);
    }
    iterator operator-(const int &n) const {
        if (n < 0) return operator+(-n);
        int global_idx = deque_ptr->get_global_index(*this) - n;
        return deque_ptr->get_iterator(global_idx);
    }
    int operator-(const iterator &rhs) const {
        if (deque_ptr != rhs.deque_ptr) throw invalid_iterator();
        return deque_ptr->get_global_index(*this) - deque_ptr->get_global_index(rhs);
    }
    iterator &operator+=(const int &n) {
        *this = *this + n;
        return *this;
    }
    iterator &operator-=(const int &n) {
        *this = *this - n;
        return *this;
    }
    iterator operator++(int) {
        iterator tmp = *this;
        ++(*this);
        return tmp;
    }
    iterator &operator++() {
        if (!block) throw invalid_iterator();
        idx++;
        if (idx >= block->sz) {
            block = block->next;
            idx = 0;
        }
        return *this;
    }
    iterator operator--(int) {
        iterator tmp = *this;
        --(*this);
        return tmp;
    }
    iterator &operator--() {
        if (!block) {
            block = deque_ptr->tail_block;
            if (!block) throw invalid_iterator();
            idx = block->sz - 1;
        } else {
            idx--;
            if (idx < 0) {
                block = block->prev;
                if (!block) throw invalid_iterator();
                idx = block->sz - 1;
            }
        }
        return *this;
    }
    T &operator*() const {
        if (!block) throw invalid_iterator();
        return *block->ptr(idx);
    }
    T *operator->() const noexcept {
        return block->ptr(idx);
    }
    bool operator==(const iterator &rhs) const {
        return block == rhs.block && idx == rhs.idx && deque_ptr == rhs.deque_ptr;
    }
    bool operator==(const const_iterator &rhs) const {
        return block == rhs.block && idx == rhs.idx && deque_ptr == rhs.deque_ptr;
    }
    bool operator!=(const iterator &rhs) const {
        return !(*this == rhs);
    }
    bool operator!=(const const_iterator &rhs) const {
        return !(*this == rhs);
    }
  };

  class const_iterator {
    friend class deque;
    friend class iterator;
  private:
    const Block* block;
    int idx;
    const deque* deque_ptr;
  public:
    const_iterator(const Block* b = nullptr, int i = 0, const deque* d = nullptr) : block(b), idx(i), deque_ptr(d) {}
    const_iterator(const iterator& other) : block(other.block), idx(other.idx), deque_ptr(other.deque_ptr) {}
    
    const_iterator operator+(const int &n) const {
        if (n < 0) return operator-(-n);
        int global_idx = deque_ptr->get_global_index(*this) + n;
        return deque_ptr->cget_iterator(global_idx);
    }
    const_iterator operator-(const int &n) const {
        if (n < 0) return operator+(-n);
        int global_idx = deque_ptr->get_global_index(*this) - n;
        return deque_ptr->cget_iterator(global_idx);
    }
    int operator-(const const_iterator &rhs) const {
        if (deque_ptr != rhs.deque_ptr) throw invalid_iterator();
        return deque_ptr->get_global_index(*this) - deque_ptr->get_global_index(rhs);
    }
    const_iterator &operator+=(const int &n) {
        *this = *this + n;
        return *this;
    }
    const_iterator &operator-=(const int &n) {
        *this = *this - n;
        return *this;
    }
    const_iterator operator++(int) {
        const_iterator tmp = *this;
        ++(*this);
        return tmp;
    }
    const_iterator &operator++() {
        if (!block) throw invalid_iterator();
        idx++;
        if (idx >= block->sz) {
            block = block->next;
            idx = 0;
        }
        return *this;
    }
    const_iterator operator--(int) {
        const_iterator tmp = *this;
        --(*this);
        return tmp;
    }
    const_iterator &operator--() {
        if (!block) {
            block = deque_ptr->tail_block;
            if (!block) throw invalid_iterator();
            idx = block->sz - 1;
        } else {
            idx--;
            if (idx < 0) {
                block = block->prev;
                if (!block) throw invalid_iterator();
                idx = block->sz - 1;
            }
        }
        return *this;
    }
    const T &operator*() const {
        if (!block) throw invalid_iterator();
        return *block->ptr(idx);
    }
    const T *operator->() const noexcept {
        return block->ptr(idx);
    }
    bool operator==(const iterator &rhs) const {
        return block == rhs.block && idx == rhs.idx && deque_ptr == rhs.deque_ptr;
    }
    bool operator==(const const_iterator &rhs) const {
        return block == rhs.block && idx == rhs.idx && deque_ptr == rhs.deque_ptr;
    }
    bool operator!=(const iterator &rhs) const {
        return !(*this == rhs);
    }
    bool operator!=(const const_iterator &rhs) const {
        return !(*this == rhs);
    }
  };

  deque() : head_block(nullptr), tail_block(nullptr), total_size(0) {}
  
  deque(const deque &other) : head_block(nullptr), tail_block(nullptr), total_size(0) {
      Block* curr = other.head_block;
      while (curr) {
          for (int i = 0; i < curr->sz; ++i) {
              push_back(*curr->ptr(i));
          }
          curr = curr->next;
      }
  }

  ~deque() {
      clear();
  }

  deque &operator=(const deque &other) {
      if (this == &other) return *this;
      clear();
      Block* curr = other.head_block;
      while (curr) {
          for (int i = 0; i < curr->sz; ++i) {
              push_back(*curr->ptr(i));
          }
          curr = curr->next;
      }
      return *this;
  }

  T &at(const size_t &pos) {
      if (pos >= total_size) throw index_out_of_bound();
      return *get_iterator(pos);
  }
  const T &at(const size_t &pos) const {
      if (pos >= total_size) throw index_out_of_bound();
      return *cget_iterator(pos);
  }
  T &operator[](const size_t &pos) {
      if (pos >= total_size) throw index_out_of_bound();
      return *get_iterator(pos);
  }
  const T &operator[](const size_t &pos) const {
      if (pos >= total_size) throw index_out_of_bound();
      return *cget_iterator(pos);
  }

  const T &front() const {
      if (total_size == 0) throw container_is_empty();
      return *head_block->ptr(0);
  }
  const T &back() const {
      if (total_size == 0) throw container_is_empty();
      return *tail_block->ptr(tail_block->sz - 1);
  }

  iterator begin() { return iterator(head_block, 0, this); }
  const_iterator cbegin() const { return const_iterator(head_block, 0, this); }
  iterator end() { return iterator(nullptr, 0, this); }
  const_iterator cend() const { return const_iterator(nullptr, 0, this); }

  bool empty() const { return total_size == 0; }
  size_t size() const { return total_size; }

  void clear() {
      Block* curr = head_block;
      while (curr) {
          Block* next = curr->next;
          delete curr;
          curr = next;
      }
      head_block = tail_block = nullptr;
      total_size = 0;
  }

  iterator insert(iterator pos, const T &value) {
      if (pos.deque_ptr != this) throw invalid_iterator();
      T copy = value;
      int global_idx = get_global_index(pos);
      if (global_idx == total_size) {
          push_back(copy);
          return get_iterator(global_idx);
      }
      Block* b = pos.block;
      int idx = pos.idx;
      b->insert(idx, copy);
      total_size++;
      maintain_after_insert(b);
      return get_iterator(global_idx);
  }

  iterator erase(iterator pos) {
      if (pos.deque_ptr != this || pos == end()) throw invalid_iterator();
      int global_idx = get_global_index(pos);
      Block* b = pos.block;
      int idx = pos.idx;
      b->erase(idx);
      total_size--;
      maintain_after_erase(b);
      return get_iterator(global_idx);
  }

  void push_back(const T &value) {
      T copy = value;
      if (!tail_block) {
          head_block = tail_block = new Block();
      }
      tail_block->push_back(copy);
      total_size++;
      maintain_after_insert(tail_block);
  }

  void pop_back() {
      if (total_size == 0) throw container_is_empty();
      tail_block->pop_back();
      total_size--;
      maintain_after_erase(tail_block);
  }

  void push_front(const T &value) {
      T copy = value;
      if (!head_block) {
          head_block = tail_block = new Block();
      }
      head_block->push_front(copy);
      total_size++;
      maintain_after_insert(head_block);
  }

  void pop_front() {
      if (total_size == 0) throw container_is_empty();
      head_block->pop_front();
      total_size--;
      maintain_after_erase(head_block);
  }

private:
    int get_global_index(const const_iterator& pos) const {
        if (pos == cend()) return total_size;
        int idx = pos.idx;
        const Block* curr = head_block;
        while (curr && curr != pos.block) {
            idx += curr->sz;
            curr = curr->next;
        }
        if (!curr) throw invalid_iterator();
        return idx;
    }
    int get_global_index(const iterator& pos) const {
        return get_global_index(const_iterator(pos));
    }

    iterator get_iterator(int global_idx) {
        if (global_idx < 0 || global_idx > total_size) throw index_out_of_bound();
        if (global_idx == total_size) return end();
        Block* curr = head_block;
        while (curr && global_idx >= curr->sz) {
            global_idx -= curr->sz;
            curr = curr->next;
        }
        return iterator(curr, global_idx, this);
    }

    const_iterator cget_iterator(int global_idx) const {
        if (global_idx < 0 || global_idx > total_size) throw index_out_of_bound();
        if (global_idx == total_size) return cend();
        const Block* curr = head_block;
        while (curr && global_idx >= curr->sz) {
            global_idx -= curr->sz;
            curr = curr->next;
        }
        return const_iterator(curr, global_idx, this);
    }
};

} // namespace sjtu

#endif