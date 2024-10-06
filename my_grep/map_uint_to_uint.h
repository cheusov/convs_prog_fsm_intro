/*
 * Copyright (c) 2024 Aleksey Cheusov <vle@gmx.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _SET_OF_INT_H_
#define _SET_OF_INT_H_

#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdlib>

// High-performance map of unsigned to unsigned
class map_uint_to_uint {
private:
	struct triple {
		unsigned m_next;
		unsigned m_iteration_counter;
		unsigned m_value;
	};

	triple *m_data = nullptr;
	unsigned m_iteration_counter = 1;
	unsigned m_first_key = unsigned(-1);
	unsigned m_key_limit = 0;
public:
	map_uint_to_uint() = default;

	void set_key_limit(unsigned key_limit) {
		if (m_key_limit != key_limit) {
			std::size_t data_size = key_limit * sizeof(m_data[0]);
			m_data = (triple *) realloc(m_data, data_size);

			// cleaning m_iteration_counter members
			std::memset(m_data, 0, data_size);
		}
	}

	~map_uint_to_uint() {
		free(m_data);
	}

	inline unsigned get_first_key() const {
		return m_first_key;
	}

	inline unsigned get_next_key(unsigned key) const {
		return m_data[key].m_next;
	}

	 // O(1)
	inline bool contains(unsigned key) const {
		return m_data[key].m_iteration_counter == m_iteration_counter;
	}

	 // O(1)
	inline unsigned operator[](unsigned key) const {
		triple& triple = m_data[key];
		assert(triple.m_iteration_counter == m_iteration_counter);
		return triple.m_value;
	}

	 // O(1)
	inline unsigned& operator[](unsigned key) {
		triple& triple = m_data[key];
		if (triple.m_iteration_counter != m_iteration_counter) {
			triple.m_value = 0;
			triple.m_iteration_counter = m_iteration_counter;
			triple.m_next = m_first_key;
			m_first_key = key;
		}
		return triple.m_value;
	}

	// O(1)
	inline void insert(unsigned key, unsigned value) {
		triple& triple = m_data[key];
		triple.m_value = value;
		if (triple.m_iteration_counter != m_iteration_counter) {
			triple.m_iteration_counter = m_iteration_counter;
			triple.m_next = m_first_key;
			m_first_key = key;
		}
	}

	// amortized: O(1), worst: O(k) where k is the key_limit
	inline void clear() {
		m_first_key = -1;
		++m_iteration_counter;
		if (m_iteration_counter == 0) {
			// this happens once per 2^(8*sizeof(unsigned))-2 invocation of clear()
			std::memset(m_data, -1, m_key_limit * sizeof(m_data[0]));
		}
	}

	class iterator {
	private:
		map_uint_to_uint* m_obj;
		unsigned* m_key;
	public:
		iterator(map_uint_to_uint *obj, unsigned* key) {
			m_obj = obj;
			m_key = key;
		}
		iterator() {
			static const unsigned minus1 = (unsigned)-1;
			m_obj = nullptr;
			m_key = (unsigned*) ((const char*)&minus1 - (const char*)0);
		}

		inline bool operator== (const iterator& it) const {
			return *m_key == *it.m_key;
		}

		inline unsigned operator* () const {
			return m_obj->m_data[*m_key].m_value;
		}

		inline iterator& operator++ () {
			m_key = &(m_obj->m_data[*m_key].m_next);
			return *this;
		}

		inline unsigned key() const {
			return *m_key;
		}

		inline bool is_end() const {
			return *m_key == -1;
		}

		inline void erase() const {
			unsigned key = *m_key;
			triple& triple = m_obj->m_data[key];
			--triple.m_iteration_counter;
			*m_key = m_obj->m_data[key].m_next;
		}
	};

	inline iterator begin() {
		return iterator(this, &m_first_key);
	}

	inline iterator end() {
		return iterator();
	}

	inline unsigned get_key_limit() const {
		return m_key_limit;
	}

	inline bool empty() const {
		return m_first_key == -1;
	}
};

#endif
