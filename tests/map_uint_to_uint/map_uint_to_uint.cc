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

#include "map_uint_to_uint.h"

#include <iostream>
#include <map>
#include <initializer_list>
#include <cstdlib>

static const unsigned KEY_LIMIT = 1000;

static void print_map(std::map<unsigned, unsigned> &map)
{
	std::cout << "std::map:\n";
	for (const auto& p: map) {
		std::cout << "map[" << p.first << "]: " << p.second << '\n';
	}
	std::cout << std::endl;
}

static void print_map(map_uint_to_uint &map)
{
	std::cout << "map_uint_to_uint:\n";
	map_uint_to_uint::iterator curr = map.begin();
	map_uint_to_uint::iterator end = map.end();
	for (; ! (curr == end); ++curr) {
		std::cout << "map[" << curr.key() << "]: " << *curr << '\n';
	}
	std::cout << std::endl;
}

static void compare(
	map_uint_to_uint& map1,
	std::map<unsigned, unsigned>& map2)
{
//	print_map(map1);
//	print_map(map2);

	assert(map1.size() == map2.size());

	for (const auto &p: map2) {
		assert(map1.contains(p.first));
		assert(map1[p.first] == map2.at(p.first));
	}

	for (unsigned key = 0; key < KEY_LIMIT; ++key) {
		assert(! ((map2.find(key) != map2.end()) ^ map1.contains(key)));
	}
}

static void test(
	map_uint_to_uint& map1,
	std::map<unsigned, unsigned>& map2)
{
	// check correctness
	compare(map1, map2);

	// erase two elements
	map_uint_to_uint::iterator it = map1.begin();
	map2.erase(it.key());
	it.erase();
	++it;
	map2.erase(it.key());
	it.erase();

	// check correctness
	compare(map1, map2);

	// test a lot of erasions if map is big enough
	if (map2.size() > 500) {
		for (int i = 0; i < 50; ++i) { // 50 erasions
			map_uint_to_uint::iterator it = map1.begin();
			for (int j = 0; j < (rand() % map2.size()) - 1; ++j) {
				++it;
			}

			map2.erase(it.key());
			it.erase();

			compare(map1, map2);
		}
	}
}

static void test(const std::initializer_list<unsigned>& keys)
{
	assert(keys.size() > 3);

	map_uint_to_uint map1;
	map1.set_key_limit(KEY_LIMIT);

	std::map<unsigned, unsigned> map2;

	// add keys
	for (unsigned key: keys) {
		map1[key] = key + 1000;
		map2[key] = key + 1000;
	}

	//
	test(map1, map2);
}

int main(int argc, char **argv)
{
	// manual tests
	test({1,2,3,4,5});
	test({100,200,300,400,500,600,999});

	// random tests
	map_uint_to_uint map1;
	assert(map1.empty());
	map1.set_key_limit(KEY_LIMIT);
	assert(map1.empty());

	std::map<unsigned, unsigned> map2;

	compare(map1, map2); // tests empty map

	srand(0x91FBE456);
	for (int tn = 0; tn < 100; ++tn) {
		map1.clear();
		map2.clear();
		assert(map1.empty());

		for (int kc = 0; kc < KEY_LIMIT * 9 / 10; ++kc) {
			int r = rand() % KEY_LIMIT;
//			printf("r=%d\n", r);
			map1[r] = r + 1000;
			map2[r] = r + 1000;
		}

		assert(!map1.empty());

		test(map1, map2);
	}
}
