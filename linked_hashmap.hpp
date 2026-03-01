/**
 * implement a container like std::linked_hashmap
 */
#ifndef SJTU_LINKEDHASHMAP_HPP
#define SJTU_LINKEDHASHMAP_HPP

// only for std::equal_to<T> and std::hash<T>
#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {
    /**
     * In linked_hashmap, iteration ordering is differ from map,
     * which is the order in which keys were inserted into the map.
     * You should maintain a doubly-linked list running through all
     * of its entries to keep the correct iteration order.
     *
     * Note that insertion order is not affected if a key is re-inserted
     * into the map.
     */

template<
	class Key,
	class T,
	class Hash = std::hash<Key>,
	class Equal = std::equal_to<Key>
> class linked_hashmap {
public:
	/**
	 * the internal type of data.
	 * it should have a default constructor, a copy constructor.
	 * You can use sjtu::linked_hashmap as value_type by typedef.
	 */
	typedef pair<const Key, T> value_type;

private:
	struct node {
		value_type data;
		node *bucket_next;
		node *prev;
		node *next;
		node(const value_type &v) : data(v), bucket_next(nullptr), prev(nullptr), next(nullptr) {}
	};

	node **buckets;
	size_t bucket_cnt;
	size_t cur_size;
	node *head;
	node *tail;
	Hash hash_fn;
	Equal equal_fn;

	static const size_t INIT_BUCKET = 16;

	size_t get_bucket(const Key &key) const {
		return static_cast<size_t>(hash_fn(key)) % bucket_cnt;
	}

	node *find_node(const Key &key) const {
		size_t id = get_bucket(key);
		node *p = buckets[id];
		while (p != nullptr) {
			if (equal_fn(p->data.first, key)) return p;
			p = p->bucket_next;
		}
		return nullptr;
	}

	void init_buckets(size_t n) {
		bucket_cnt = n;
		buckets = new node *[bucket_cnt];
		for (size_t i = 0; i < bucket_cnt; ++i) buckets[i] = nullptr;
	}

	void place_to_bucket(node *p) {
		size_t id = get_bucket(p->data.first);
		p->bucket_next = buckets[id];
		buckets[id] = p;
	}

	void rehash(size_t ncnt) {
		node **old_buckets = buckets;
		size_t old_cnt = bucket_cnt;
		init_buckets(ncnt);
		node *p = head;
		while (p != nullptr) {
			place_to_bucket(p);
			p = p->next;
		}
		delete[] old_buckets;
		(void)old_cnt;
	}

	void maybe_rehash_for_insert() {
		if ((cur_size + 1) * 4 > bucket_cnt * 3) {
			rehash(bucket_cnt << 1);
		}
	}

	void append_node(node *p) {
		if (tail == nullptr) {
			head = tail = p;
		} else {
			tail->next = p;
			p->prev = tail;
			tail = p;
		}
	}

	void erase_node(node *p) {
		size_t id = get_bucket(p->data.first);
		node *cur = buckets[id], *pre = nullptr;
		while (cur != nullptr && cur != p) {
			pre = cur;
			cur = cur->bucket_next;
		}
		if (cur == nullptr) throw invalid_iterator();
		if (pre == nullptr) buckets[id] = cur->bucket_next;
		else pre->bucket_next = cur->bucket_next;

		if (p->prev != nullptr) p->prev->next = p->next;
		else head = p->next;
		if (p->next != nullptr) p->next->prev = p->prev;
		else tail = p->prev;

		delete p;
		--cur_size;
	}

	void copy_from(const linked_hashmap &other) {
		for (node *p = other.head; p != nullptr; p = p->next) {
			insert(p->data);
		}
	}

public:
	/**
	 * see BidirectionalIterator at CppReference for help.
	 *
	 * if there is anything wrong throw invalid_iterator.
	 *     like it = linked_hashmap.begin(); --it;
	 *       or it = linked_hashmap.end(); ++end();
	 */
	class const_iterator;
	class iterator {
	private:
		node *ptr;
		linked_hashmap *owner;
		friend class linked_hashmap;
		friend class const_iterator;
		iterator(linked_hashmap *o, node *p) : ptr(p), owner(o) {}
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = typename linked_hashmap::value_type;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::output_iterator_tag;


		iterator() : ptr(nullptr), owner(nullptr) {}
		iterator(const iterator &other) : ptr(other.ptr), owner(other.owner) {}
		/**
		 * TODO iter++
		 */
		iterator operator++(int) {
			if (owner == nullptr || ptr == nullptr) throw invalid_iterator();
			iterator ret(*this);
			ptr = ptr->next;
			return ret;
		}
		/**
		 * TODO ++iter
		 */
		iterator & operator++() {
			if (owner == nullptr || ptr == nullptr) throw invalid_iterator();
			ptr = ptr->next;
			return *this;
		}
		/**
		 * TODO iter--
		 */
		iterator operator--(int) {
			if (owner == nullptr) throw invalid_iterator();
			iterator ret(*this);
			if (ptr == nullptr) {
				if (owner->tail == nullptr) throw invalid_iterator();
				ptr = owner->tail;
			} else {
				if (ptr->prev == nullptr) throw invalid_iterator();
				ptr = ptr->prev;
			}
			return ret;
		}
		/**
		 * TODO --iter
		 */
		iterator & operator--() {
			if (owner == nullptr) throw invalid_iterator();
			if (ptr == nullptr) {
				if (owner->tail == nullptr) throw invalid_iterator();
				ptr = owner->tail;
			} else {
				if (ptr->prev == nullptr) throw invalid_iterator();
				ptr = ptr->prev;
			}
			return *this;
		}
		/**
		 * a operator to check whether two iterators are same (pointing to the same memory).
		 */
		value_type & operator*() const {
			if (owner == nullptr || ptr == nullptr) throw invalid_iterator();
			return ptr->data;
		}
		bool operator==(const iterator &rhs) const {
			return owner == rhs.owner && ptr == rhs.ptr;
		}
		bool operator==(const const_iterator &rhs) const {
			return owner == rhs.owner && ptr == rhs.ptr;
		}
		/**
		 * some other operator for iterator.
		 */
		bool operator!=(const iterator &rhs) const {
			return !(*this == rhs);
		}
		bool operator!=(const const_iterator &rhs) const {
			return !(*this == rhs);
		}

		/**
		 * for the support of it->first.
		 * See <http://kelvinh.github.io/blog/2013/11/20/overloading-of-member-access-operator-dash-greater-than-symbol-in-cpp/> for help.
		 */
		value_type* operator->() const noexcept {
			if (owner == nullptr || ptr == nullptr) throw invalid_iterator();
			return &(ptr->data);
		}
	};

	class const_iterator {
		// it should has similar member method as iterator.
		//  and it should be able to construct from an iterator.
		private:
			node *ptr;
			const linked_hashmap *owner;
			friend class linked_hashmap;
			friend class iterator;
			const_iterator(const linked_hashmap *o, node *p) : ptr(p), owner(o) {}
		public:
			using difference_type = std::ptrdiff_t;
			using value_type = typename linked_hashmap::value_type;
			using pointer = const value_type*;
			using reference = const value_type&;
			using iterator_category = std::output_iterator_tag;

			const_iterator() : ptr(nullptr), owner(nullptr) {}
			const_iterator(const const_iterator &other) : ptr(other.ptr), owner(other.owner) {}
			const_iterator(const iterator &other) : ptr(other.ptr), owner(other.owner) {}

			const_iterator operator++(int) {
				if (owner == nullptr || ptr == nullptr) throw invalid_iterator();
				const_iterator ret(*this);
				ptr = ptr->next;
				return ret;
			}
			const_iterator & operator++() {
				if (owner == nullptr || ptr == nullptr) throw invalid_iterator();
				ptr = ptr->next;
				return *this;
			}
			const_iterator operator--(int) {
				if (owner == nullptr) throw invalid_iterator();
				const_iterator ret(*this);
				if (ptr == nullptr) {
					if (owner->tail == nullptr) throw invalid_iterator();
					ptr = owner->tail;
				} else {
					if (ptr->prev == nullptr) throw invalid_iterator();
					ptr = ptr->prev;
				}
				return ret;
			}
			const_iterator & operator--() {
				if (owner == nullptr) throw invalid_iterator();
				if (ptr == nullptr) {
					if (owner->tail == nullptr) throw invalid_iterator();
					ptr = owner->tail;
				} else {
					if (ptr->prev == nullptr) throw invalid_iterator();
					ptr = ptr->prev;
				}
				return *this;
			}
			reference operator*() const {
				if (owner == nullptr || ptr == nullptr) throw invalid_iterator();
				return ptr->data;
			}
			pointer operator->() const noexcept {
				if (owner == nullptr || ptr == nullptr) throw invalid_iterator();
				return &(ptr->data);
			}

			bool operator==(const iterator &rhs) const {
				return owner == rhs.owner && ptr == rhs.ptr;
			}
			bool operator==(const const_iterator &rhs) const {
				return owner == rhs.owner && ptr == rhs.ptr;
			}
			bool operator!=(const iterator &rhs) const {
				return !(*this == rhs);
			}
			bool operator!=(const const_iterator &rhs) const {
				return !(*this == rhs);
			}
	};

	/**
	 * TODO two constructors
	 */
	linked_hashmap() : buckets(nullptr), bucket_cnt(0), cur_size(0), head(nullptr), tail(nullptr), hash_fn(Hash()), equal_fn(Equal()) {
		init_buckets(INIT_BUCKET);
	}
	linked_hashmap(const linked_hashmap &other) : buckets(nullptr), bucket_cnt(0), cur_size(0), head(nullptr), tail(nullptr), hash_fn(other.hash_fn), equal_fn(other.equal_fn) {
		init_buckets(other.bucket_cnt);
		copy_from(other);
	}

	/**
	 * TODO assignment operator
	 */
	linked_hashmap & operator=(const linked_hashmap &other) {
		if (this == &other) return *this;
		clear();
		delete[] buckets;
		hash_fn = other.hash_fn;
		equal_fn = other.equal_fn;
		init_buckets(other.bucket_cnt);
		copy_from(other);
		return *this;
	}

	/**
	 * TODO Destructors
	 */
	~linked_hashmap() {
		clear();
		delete[] buckets;
	}

	/**
	 * TODO
	 * access specified element with bounds checking
	 * Returns a reference to the mapped value of the element with key equivalent to key.
	 * If no such element exists, an exception of type `index_out_of_bound'
	 */
	T & at(const Key &key) {
		node *p = find_node(key);
		if (p == nullptr) throw index_out_of_bound();
		return p->data.second;
	}
	const T & at(const Key &key) const {
		node *p = find_node(key);
		if (p == nullptr) throw index_out_of_bound();
		return p->data.second;
	}

	/**
	 * TODO
	 * access specified element
	 * Returns a reference to the value that is mapped to a key equivalent to key,
	 *   performing an insertion if such key does not already exist.
	 */
	T & operator[](const Key &key) {
		node *p = find_node(key);
		if (p != nullptr) return p->data.second;
		maybe_rehash_for_insert();
		node *nw = new node(value_type(key, T()));
		append_node(nw);
		place_to_bucket(nw);
		++cur_size;
		return nw->data.second;
	}

	/**
	 * behave like at() throw index_out_of_bound if such key does not exist.
	 */
	const T & operator[](const Key &key) const {
		return at(key);
	}

	/**
	 * return a iterator to the beginning
	 */
	iterator begin() {
		return iterator(this, head);
	}
	const_iterator cbegin() const {
		return const_iterator(this, head);
	}

	/**
	 * return a iterator to the end
	 * in fact, it returns past-the-end.
	 */
	iterator end() {
		return iterator(this, nullptr);
	}
	const_iterator cend() const {
		return const_iterator(this, nullptr);
	}

	/**
	 * checks whether the container is empty
	 * return true if empty, otherwise false.
	 */
	bool empty() const {
		return cur_size == 0;
	}

	/**
	 * returns the number of elements.
	 */
	size_t size() const {
		return cur_size;
	}

	/**
	 * clears the contents
	 */
	void clear() {
		node *p = head;
		while (p != nullptr) {
			node *nxt = p->next;
			delete p;
			p = nxt;
		}
		head = tail = nullptr;
		cur_size = 0;
		for (size_t i = 0; i < bucket_cnt; ++i) buckets[i] = nullptr;
	}

	/**
	 * insert an element.
	 * return a pair, the first of the pair is
	 *   the iterator to the new element (or the element that prevented the insertion),
	 *   the second one is true if insert successfully, or false.
	 */
	pair<iterator, bool> insert(const value_type &value) {
		node *p = find_node(value.first);
		if (p != nullptr) return pair<iterator, bool>(iterator(this, p), false);
		maybe_rehash_for_insert();
		node *nw = new node(value);
		append_node(nw);
		place_to_bucket(nw);
		++cur_size;
		return pair<iterator, bool>(iterator(this, nw), true);
	}

	/**
	 * erase the element at pos.
	 *
	 * throw if pos pointed to a bad element (pos == this->end() || pos points an element out of this)
	 */
	void erase(iterator pos) {
		if (pos.owner != this || pos.ptr == nullptr) throw invalid_iterator();
		erase_node(pos.ptr);
	}

	/**
	 * Returns the number of elements with key
	 *   that compares equivalent to the specified argument,
	 *   which is either 1 or 0
	 *     since this container does not allow duplicates.
	 */
	size_t count(const Key &key) const {
		return find_node(key) == nullptr ? 0 : 1;
	}

	/**
	 * Finds an element with key equivalent to key.
	 * key value of the element to search for.
	 * Iterator to an element with key equivalent to key.
	 *   If no such element is found, past-the-end (see end()) iterator is returned.
	 */
	iterator find(const Key &key) {
		return iterator(this, find_node(key));
	}
	const_iterator find(const Key &key) const {
		return const_iterator(this, find_node(key));
	}
};

}

#endif