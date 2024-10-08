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

// Algorithm:
// * Convert glob pattern to NFA.
// * Map input weights of NFA (ASCII characters) to positive numbers,
//   unseen character in this map is mapped to 0.
// * Convert NFA to MinDFA using Brzozoeski algorithm.
// * Match using MinDFA.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include <errno.h>
#include <getopt.h>

#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <iostream>
#include <utility>

#include <mkc_err.h>

#include "readlines.h"
#include "map_uint_to_uint.h"

static std::ostream &debug = std::cerr;

struct iw_to {
	unsigned iw = 0; // input weight
	unsigned to = 0; // destination state

	iw_to () {}
	iw_to (unsigned iw, unsigned to) {
		this->iw = iw;
		this->to = to;
	}
};

typedef std::vector<unsigned> vector_uint;
typedef std::vector<iw_to> vector_iwto;
typedef std::set<unsigned> set_uint;

enum fsa_operation {
	UNION,
	INTERSECT,
	SUBTRACT,
	NEGATE,
};

static uint32_t nextpow2(uint32_t value)
{
	uint32_t ret = value;
	ret |= ret >> 1;
	ret |= ret >> 2;
	ret |= ret >> 4;
	ret |= ret >> 16;
	return ret + 1;
}

class set_uint2id {
private:
	unsigned m_count = 0;
	std::map<set_uint, unsigned> m_map;
public:
	set_uint2id() {}

	// Returns id of inserted item, i.e., 0, 1, 2, etc.
	unsigned add(set_uint v) {
		std::map<set_uint, unsigned>::const_iterator found = m_map.find(v);
		if (found == m_map.end()) {
			m_map[v] = m_count;
			return m_count++;
		} else {
			return found->second;
		}
	}
};

// Finite State Automaton used for building NFA and DFA.
// It is slow but it does not matter for this test ;-)
class fsa {
private:
	unsigned m_state_count = 0;
	set_uint m_initial_states;
	set_uint m_finite_states;
	set_uint m_iws;
	std::vector<vector_iwto> m_outgoing_arcs;

	static const vector_iwto m_empty_iwto;

public:
	fsa() {}

	~fsa() {
		clear();
	}

	void clear()
	{
		m_state_count = 0;
		m_initial_states.clear();
		m_finite_states.clear();
		m_iws.clear();
		m_outgoing_arcs.clear();
	}

	inline unsigned get_state_count() const {
		return m_state_count;
	}

	inline const set_uint& get_initial_states() const {
		return m_initial_states;
	}

	inline const set_uint& get_finite_states() const {
		return m_finite_states;
	}

	inline const set_uint& get_iws() const {
		return m_iws;
	}

	inline const vector_iwto& get_arcs(unsigned state) const {
		if (state < get_state_count())
			return m_outgoing_arcs[state];
		else
			return m_empty_iwto;
	}

	vector_uint get_arcs(unsigned state, unsigned iw) const {
		vector_uint ret;
		const vector_iwto& outgoing_arcs = m_outgoing_arcs[state];
		for (unsigned i = 0; i < outgoing_arcs.size(); ++i) {
			const iw_to& arc = outgoing_arcs[i];
			if (arc.iw == iw)
				ret.push_back(arc.to);
		}
		return ret;
	}

public:
	void add_finite_state(unsigned state) {
		update_state_count(state);
		m_finite_states.insert(state);
	}

	void add_initial_state(unsigned state) {
		update_state_count(state);
		m_initial_states.insert(state);
	}

	void add_iw(unsigned iw) {
		m_iws.insert(iw);
	}

	void add_arc(unsigned from, unsigned iw, unsigned to) {
		update_state_count(from);
		update_state_count(to);
		add_iw(iw);

		vector_iwto &outgoing_arcs = m_outgoing_arcs[from];
		for (unsigned i = 0; i < outgoing_arcs.size(); ++i) {
			iw_to &arc = outgoing_arcs[i];
			if (arc.iw == iw && arc.to == to)
				return;
		}
		outgoing_arcs.push_back(iw_to(iw, to));
	}

	bool is_finite_state(unsigned state) const {
		return m_finite_states.find(state) != m_finite_states.end();
	}

private:
	void update_state_count(unsigned state){
		if (state < m_state_count)
			return;

		m_state_count = state + 1;
		while (m_outgoing_arcs.size() < m_state_count) {
			m_outgoing_arcs.resize(m_outgoing_arcs.size() + 1);
		}
	}
};

const vector_iwto fsa::m_empty_iwto;

// In order to reduce memory consumption for storing matrix of arcs,
// we map characters seen in glob pattern to positive numbers 1, 2, 3 etc.
// For example, we map 'A' to 1, 'B' to 2 etc.
// Returned value is an allocated and filled IW map array.
static unsigned *build_iwmap(fsa& dst_fsa, unsigned iw_map_size, const fsa& src_fsa)
{
	// build iw_map
	const set_uint& iws = src_fsa.get_iws();

	unsigned iw_count = 0;
	for (unsigned iw: iws) {
		if (iw >= iw_count)
			iw_count = iw + 1;
	}
	assert(iw_count <= iw_map_size);

	unsigned *iw_map = new unsigned[iw_map_size];
	// iw == 0 means all possible ASCII characters except seen in glob pattern
	memset(iw_map, 0, iw_map_size * sizeof(iw_map[0]));

	unsigned max_new_iw = 0;
	for (unsigned iw: iws) {
		if (iw != 0 && !iw_map[iw]){ // map 0 to 0
			iw_map[iw] = ++max_new_iw; // others non-0 to positive numbers
		}
	}

	// build dst_fsa
	const set_uint& initial_states = src_fsa.get_initial_states();
	for (unsigned state: initial_states)
		dst_fsa.add_initial_state(state);

	const set_uint& finite_states = src_fsa.get_finite_states();
	for (unsigned state: finite_states)
		dst_fsa.add_finite_state(state);

	for (unsigned from = 0; from < src_fsa.get_state_count(); ++from) {
		const vector_iwto& outgoing_arcs = src_fsa.get_arcs(from);
		for (unsigned i = 0; i < outgoing_arcs.size(); ++i) {
			unsigned iw = outgoing_arcs[i].iw;
			unsigned to = outgoing_arcs[i].to;
			dst_fsa.add_arc(from, iw_map[iw], to);
		}
	}

	return iw_map;
}

// Inverts FSA, that is, inverts all arcs, make initial states finite and vice versa.
static void invert(fsa& dst_fsa, const fsa& src_fsa)
{
	dst_fsa.clear();

	for (unsigned iw: src_fsa.get_iws())
		dst_fsa.add_iw(iw);

	for (unsigned state: src_fsa.get_initial_states())
		dst_fsa.add_finite_state(state);

	for (unsigned state: src_fsa.get_finite_states())
		dst_fsa.add_initial_state(state);

	std::size_t state_count = src_fsa.get_state_count();
	for (unsigned from = 0; from < state_count; ++from) {
		for (const iw_to& iwto: src_fsa.get_arcs(from)) {
			unsigned iw = iwto.iw;
			unsigned to = iwto.to;
			dst_fsa.add_arc(to, iw, from);
		}
	}
}

// Convert Non-deterministic FSA to Deterministic FSA
// https://en.m.wikipedia.org/wiki/Powerset_construction
static void nfa2dfa(
	fsa& dfa,
	const fsa &nfa,
	const std::map<unsigned, unsigned> finite_state2fsa_num,
	fsa_operation operation)
{
	dfa.clear();

	for (unsigned iw: nfa.get_iws())
		dfa.add_iw(iw);

	std::vector<set_uint> state_set_stack;
	const set_uint &initial_states = nfa.get_initial_states();
	if (!initial_states.empty()) {
		state_set_stack.push_back(initial_states);
		dfa.add_initial_state(0);
	}

	unsigned original_fsa_count = 0;
	for (std::pair<unsigned, unsigned> p: finite_state2fsa_num) {
		if (p.second > original_fsa_count)
			original_fsa_count = p.second;
	}
	++original_fsa_count;
	//debug << "original_fsa_count:" << original_fsa_count << '\n';

	const set_uint& iws = nfa.get_iws();

	set_uint2id set2id;

	while (!state_set_stack.empty()) {
		set_uint from_set(state_set_stack.back());
		state_set_stack.pop_back();

		unsigned dfa_from = set2id.add(from_set);

		switch (operation) {
			case UNION:
				for (unsigned from_state: from_set)
					if (nfa.is_finite_state(from_state))
						dfa.add_finite_state(dfa_from);
				break;

			case INTERSECT:
				{
					set_uint fsa_nums;
					for (unsigned from_state: from_set)
						if (nfa.is_finite_state(from_state))
							fsa_nums.insert(finite_state2fsa_num.find(from_state)->second);
					//debug << "fsa_nums:" << fsa_nums.size() << '\n';
					if (fsa_nums.size() == original_fsa_count)
						dfa.add_finite_state(dfa_from);
				}
				break;

			case SUBTRACT:
				{
					set_uint fsa_nums;
					for (unsigned from_state: from_set)
						if (nfa.is_finite_state(from_state))
							fsa_nums.insert(finite_state2fsa_num.find(from_state)->second);
					//debug << "fsa_nums:" << fsa_nums.size() << '\n';
					if (fsa_nums.size() == 1 && *fsa_nums.begin() == 0)
						dfa.add_finite_state(dfa_from);
				}
				break;

			default:
				abort();
		}

		for (unsigned iw: iws) {
			set_uint to_set;
			for (unsigned from_state: from_set) {
				for (unsigned to: nfa.get_arcs(from_state, iw))
					to_set.insert(to);
			}
			if (!to_set.empty()) {
				unsigned dfa_to = set2id.add(to_set);
				if (dfa_to == dfa.get_state_count())
					state_set_stack.push_back(to_set);
				dfa.add_arc(dfa_from, iw, dfa_to);
			}
		}
	}
}

static void nfa2dfa(fsa& dfa, const fsa &nfa)
{
	std::map<unsigned, unsigned> empty;
	nfa2dfa(dfa, nfa, empty, UNION);
}

// Convert Non-deterministic FSA to Minimal Deterministic FSA
// with the help of Brzozowski algorithm.
// https://en.wikipedia.org/wiki/DFA_minimization
static void nfa2mindfa(fsa& dfa, const fsa &nfa)
{
	fsa inv;
	invert(inv, nfa);

	fsa tmp_dfa;
	nfa2dfa(tmp_dfa, inv);

	inv.clear();
	invert(inv, tmp_dfa);

	nfa2dfa(dfa, inv);
}

// copy NFAs to single NFA
static void copy_nfas(
	fsa& dst,
	std::map<unsigned, unsigned>& finite_state2fsa_num,
	const std::vector<fsa> &src)
{
	dst.clear();

	set_uint common_iws;
	for (const fsa& nfa: src) {
		common_iws.insert(nfa.get_iws().begin(), nfa.get_iws().end());
	}

	unsigned offset = 0;
	for (size_t fsa_num = 0; fsa_num < src.size(); ++fsa_num) {
		const fsa& nfa = src[fsa_num];

		for (unsigned state: nfa.get_initial_states())
			dst.add_initial_state(state + offset);

		for (unsigned state: nfa.get_finite_states())
			finite_state2fsa_num[state + offset] = fsa_num;

		const set_uint& current_iws = nfa.get_iws();

		set_uint iws_diff;
		std::set_difference(
			common_iws.begin(), common_iws.end(),
			current_iws.begin(), current_iws.end(),
			std::inserter(iws_diff, iws_diff.begin()));

		size_t state_count = nfa.get_state_count();
		for (unsigned state = 0; state < state_count; ++state) {
			for (const iw_to& iwto: nfa.get_arcs(state)) {
				unsigned iw = iwto.iw;
				unsigned to = iwto.to;
				if (iw) {
					dst.add_arc(state + offset, iw, to + offset);
				} else {
					dst.add_arc(state + offset, 0, to + offset);
					for (unsigned new_iw: iws_diff) {
						dst.add_arc(state + offset, new_iw, to + offset);
					}
				}
			}
		}

		offset = dst.get_state_count();
	}
}

// Union of several NFA
static void union_nfa(
	fsa& dst,
	const std::vector<fsa> &src)
{
	std::map<unsigned, unsigned> finite_state2fsa_num;
	copy_nfas(dst, finite_state2fsa_num, src);
	for (std::pair<unsigned, unsigned> p: finite_state2fsa_num) {
		dst.add_finite_state(p.first);
	}
}

// Intersect of several NFA
static void intersect_nfa(
	fsa& dst,
	const std::vector<fsa> &src)
{
	std::map<unsigned, unsigned> finite_state2fsa_num;
	fsa tmp_nfa;
	copy_nfas(tmp_nfa, finite_state2fsa_num, src);
	for (std::pair<unsigned, unsigned> p: finite_state2fsa_num) {
		tmp_nfa.add_finite_state(p.first);
	}
	nfa2dfa(dst, tmp_nfa, finite_state2fsa_num, INTERSECT);
}

// Subtract of several NFA
static void subtract_nfa(
	fsa& dst,
	const std::vector<fsa> &src)
{
	std::map<unsigned, unsigned> finite_state2fsa_num;
	fsa tmp_nfa;
	copy_nfas(tmp_nfa, finite_state2fsa_num, src);
	for (std::pair<unsigned, unsigned> p: finite_state2fsa_num) {
		tmp_nfa.add_finite_state(p.first);
	}
	nfa2dfa(dst, tmp_nfa, finite_state2fsa_num, SUBTRACT);
}

// Functions for debugging
static void print_vector(const set_uint &s)
{
	for (unsigned v: s) {
		debug << ' ' << v;
	}
}

static void print_fsa(fsa &_fsa)
{
	debug << "state count: " << _fsa.get_state_count() << '\n';

	debug << "initial states:";
	print_vector(_fsa.get_initial_states());
	debug << '\n';

	debug << "finite states:";
	print_vector(_fsa.get_finite_states());
	debug << '\n';

	debug << "input weights:";
	print_vector(_fsa.get_iws());
	debug << '\n';

//	debug << "finite states2:\n";
//	for (unsigned i = 0; i < _fsa.get_state_count(); ++i) {
//		debug << " state " << i << " is finite: " << _fsa.is_finite_state(i) << '\n';
//	}

	debug << "arcs:\n";
	for (unsigned from = 0; from < _fsa.get_state_count(); ++from) {
		for (const iw_to& iwto: _fsa.get_arcs(from)) {
			unsigned iw = iwto.iw;
			unsigned to = iwto.to;
			char iwc = isalnum(iw) ? (char)iw : ' ';
			debug << ' ' << from << ' ' << iw << '/' << iwc << ' ' << to << '\n';
		}
	}
	debug << '\n';
}

// Deterministic Finite State Automaton used during match
// Initial state is 0.
class fast_dfa {
protected:
	unsigned *m_arcs = nullptr;

private:
	unsigned m_state_count;
	unsigned m_iw_count;
	unsigned m_initial_state;
	unsigned m_first_finite_state;

	void process_completely_finite_states()
	{
		for (unsigned state = 0; state < m_state_count; ++state) {
			bool loop = true;
			for (unsigned iw = 0; iw < m_iw_count; ++iw) {
				if (m_arcs[state * m_iw_count + iw] != state) {
					loop = false;
					break;
				}
			}
			if (loop) {
				for (unsigned iw = 0; iw < m_iw_count; ++iw) {
					m_arcs[state * m_iw_count + iw] = -2;
				}
			}
		}
		for (unsigned state = 0; state < m_state_count; ++state) {
			for (unsigned iw = 0; iw < m_iw_count; ++iw) {
				int to_state = m_arcs[state * m_iw_count + iw];
				if (to_state >= 0 && m_arcs[to_state * m_iw_count] == -2) {
					m_arcs[state * m_iw_count + iw] = -2;
				}
			}
		}
	}

	// too lazy to implement them
	fast_dfa& operator= (const fast_dfa &) = delete;
	fast_dfa& operator= (fast_dfa &&) = delete;
	fast_dfa(const fast_dfa &) = delete;
	fast_dfa(fast_dfa &&) = delete;

protected:
	unsigned calc_iw_count(const fsa &dfa) const
	{
		unsigned ret = 0;
		for (unsigned iw: dfa.get_iws()) {
			if (iw >= ret)
				ret = iw + 1;
		}

		return ret;
	}

public:
	fast_dfa() noexcept {}

	~fast_dfa()
	{
		clear();
	}

	void clear()
	{
		delete [] m_arcs;
		m_arcs = nullptr;
	}

	// Convert slow 'fsa' to fast 'fast_dfa'
	void set(
		const fsa &dfa,
		unsigned iw_count = (unsigned)-1,
		unsigned state_count = (unsigned) -1)
	{
		clear();

		const set_uint& iws = dfa.get_iws();

		// set m_state_count
		if (state_count == (unsigned)-1)
			m_state_count = (unsigned)dfa.get_state_count();
		else
			m_state_count = state_count;

		// calculating m_iw_count which is max input weight + 1
		if (iw_count == (unsigned)-1)
			m_iw_count = calc_iw_count(dfa);
		else
			m_iw_count = iw_count;

		// optimize finite states by moving them to the right of
		// non-finite states
		std::vector<unsigned> state_map;
		state_map.resize(m_state_count);

		m_first_finite_state = m_state_count - dfa.get_finite_states().size();
		unsigned current_finite_state = m_first_finite_state;
		unsigned current_nonfinite_state = 0;
		for (unsigned state = 0; state < m_state_count; ++state) {
			if (dfa.is_finite_state(state)) {
				state_map[state] = current_finite_state++;
			} else {
				state_map[state] = current_nonfinite_state++;
			}
		}
		if (dfa.is_finite_state(0))
			m_initial_state = m_first_finite_state;
		else
			m_initial_state = 0;

		// from_state * iws -> to_state matrix.
		// to_state == -1 means "no arc"
		m_arcs = new unsigned[m_state_count * m_iw_count];
		memset(m_arcs, -1, m_state_count * m_iw_count * sizeof(m_arcs[0]));

		for (unsigned from = 0; from < m_state_count; ++from) {
			const vector_iwto& outgoing_arcs = dfa.get_arcs(from);

			for (unsigned i = 0; i < outgoing_arcs.size(); ++i) {
				unsigned iw = outgoing_arcs[i].iw;
				unsigned to = outgoing_arcs[i].to;
				m_arcs[state_map[from] * m_iw_count + iw] = state_map[to];
			}
		}

		process_completely_finite_states();
	}

	inline int get_arc(int state, unsigned iw) const noexcept {
		return m_arcs[state * m_iw_count + iw];
	}

	inline void set_arc(int from, unsigned iw, int to) const noexcept {
		assert(iw < m_iw_count);
		assert(from < m_state_count);
		assert(to >= -2 && to < (int)m_state_count);
		m_arcs[from * m_iw_count + iw] = to;
	}

	inline bool is_finite_state(int state) const noexcept {
		return state >= m_first_finite_state;
	}

	inline unsigned get_initial_state() const noexcept {
		return m_initial_state;
	}

	inline unsigned get_state_count() const noexcept {
		return m_state_count;
	}

	inline unsigned get_iw_count() const noexcept {
		return m_iw_count;
	}

	inline unsigned get_first_finite_state() const noexcept {
		return m_first_finite_state;
	}
};

// The same as fast_dfa but uses shift instead of multiplication in
// get_arc method
class fast_dfa_shift: public fast_dfa {
private:
	unsigned m_iw_shift;

public:
	// Convert slow 'fsa' to fast 'fast_dfa'
	void set(const fsa &dfa, unsigned state_count = (unsigned)-1)
	{
		unsigned iw_count = nextpow2(calc_iw_count(dfa) - 1);
		m_iw_shift = 31 - __builtin_clz(iw_count);
		fast_dfa::set(dfa, iw_count, state_count);
	}

	inline int get_arc(int state, unsigned iw) const noexcept {
		return m_arcs[(state << m_iw_shift) + iw];
	}
};

// Build NFA from glob pattern
static void parse_glob(fsa &nfa, const char *glob)
{
	std::set<unsigned> iws;
	for (const char *p = glob; *p; ++p) {
		unsigned iw = (unsigned) (unsigned char) *p;
		if (iw != '*' && iw != '?')
			iws.insert(iw);
	}

	//
	unsigned current_state = 0;
	nfa.add_initial_state(0);
	for (const char *p = glob; *p; ++p) {
		unsigned curr_iw = (unsigned) (unsigned char) *p;
		unsigned addon;
		switch (curr_iw) {
			case '*':
			case '?':
				addon = (curr_iw == '?');
				// iw == 0 means all possible ASCII characters except seen in glob pattern
				nfa.add_arc(current_state, 0, current_state + addon);
				for (unsigned iw: iws)
					nfa.add_arc(current_state, iw, current_state + addon);
				current_state += addon;
				break;
			default:
				nfa.add_arc(current_state, curr_iw, current_state + 1);
				++current_state;
		}
	}
	nfa.add_finite_state(current_state);
}

// Interface class for DFA-based matcher
class dfa_matcher_i {
public:
	virtual void set_nfa(const fsa& nfa) = 0;
	virtual int match(const char *buffer, size_t buffer_size) = 0;
	virtual std::pair<int, int> search(const char *buffer, size_t buffer_size) = 0;
};

// Class for DFA-based matcher with weight mapping
class dfa_matcher_iwmap_base: public dfa_matcher_i {
protected:
	uint8_t *m_iw_map = nullptr;  // map symbols used in regexp to 1, 2 etc., map others to 0
	unsigned m_iw_map_size = 0;

public:
	dfa_matcher_iwmap_base() = default;
	virtual ~dfa_matcher_iwmap_base()
	{
		delete [] m_iw_map;
		m_iw_map = nullptr;
	}

	// too lazy to implement them
	dfa_matcher_iwmap_base& operator= (const dfa_matcher_iwmap_base &) = delete;
	dfa_matcher_iwmap_base& operator= (dfa_matcher_iwmap_base &&) = delete;
	dfa_matcher_iwmap_base(const dfa_matcher_iwmap_base &) = delete;
	dfa_matcher_iwmap_base(dfa_matcher_iwmap_base &&) = delete;

protected:
	void build_iw_map(fsa& dst_fsa, const fsa& src_nfa)
	{
		dst_fsa.clear();

		m_iw_map_size = 256;
		m_iw_map = new uint8_t[m_iw_map_size];
		memset(m_iw_map, 0, m_iw_map_size * sizeof(m_iw_map[0]));

		unsigned *temp_iw_map = build_iwmap(dst_fsa, m_iw_map_size, src_nfa);

		for (unsigned i = 0; i < m_iw_map_size; ++i) {
			m_iw_map[i] = temp_iw_map[i];
		}

		delete [] temp_iw_map;

//		print_fsa(dst_fsa);
	}
};

static bool option_D = false;

// Class used for matching using DFA with iwmap
template <typename DFAType>
class dfa_matcher_iwmap : public dfa_matcher_iwmap_base {
private:
	// glob pattern
	DFAType m_fast_dfa;

	// search mode
	map_uint_to_uint m_state2start;
public:
	dfa_matcher_iwmap() = default;

	~dfa_matcher_iwmap() { }

	virtual void set_nfa(const fsa& nfa)
	{
		m_fast_dfa.clear();

		fsa nfa_iwmap;
		build_iw_map(nfa_iwmap, nfa);

		fsa dfa;
		nfa2mindfa(dfa, nfa_iwmap);

		if (option_D)
			print_fsa(dfa);

		//
		m_fast_dfa.set(dfa);
		m_state2start.set_key_limit(dfa.get_state_count());
	}

	virtual int match(const char *buffer, size_t buffer_size)
	{
		unsigned iw = 0;
		int state = m_fast_dfa.get_initial_state();

		for (size_t pos = 0; pos < buffer_size; ++pos) {
			iw = (unsigned) (unsigned char) buffer[pos];
			iw = m_iw_map[iw];
			state = m_fast_dfa.get_arc(state, iw);
			if (state < 0)
				return state + 1;
		}

		return m_fast_dfa.is_finite_state(state);
	}

private:
	int search(int state, const char *buffer, size_t buffer_size) {
		unsigned iw = 0;
		int best = 0;

		for (size_t pos = 0; pos < buffer_size; ) {
			iw = (unsigned) (unsigned char) buffer[pos];
			++pos;
			iw = m_iw_map[iw];
			state = m_fast_dfa.get_arc(state, iw);
			switch (state) {
				case -2:
					return buffer_size;
				case -1:
					return best;
				default:
					if (m_fast_dfa.is_finite_state(state))
						best = pos;
			}
		}

		return best;
	}

public:
	virtual std::pair<int, int> search(const char *buffer, size_t buffer_size)
	{
		unsigned best_start = -1;
		int best_length = 0;

		m_state2start.clear();

		unsigned iw = 0;
		const int init_state = m_fast_dfa.get_initial_state();

		for (size_t pos = 0; pos < buffer_size; ++pos) {
			iw = (unsigned) (unsigned char) buffer[pos];
			iw = m_iw_map[iw];

			//
			map_uint_to_uint::iterator curr_it = m_state2start.begin();
			for (; ! curr_it.is_end(); ++curr_it) {
				const unsigned start = *curr_it;
				if (start > best_start) {
					curr_it.erase();
					if (curr_it.is_end())
						break;
					else
						continue;
				}

				unsigned old_state = curr_it.key();
				int state = m_fast_dfa.get_arc(old_state, iw);
				switch (state) {
					case -1:
						curr_it.erase();
						break;
					case -2:
						curr_it.erase();
						if (start <= best_start) {
							best_start = start;
							best_length = buffer_size - start;
						}
						break;
					default:
						bool different = (old_state != state);
						if (different) {
							curr_it.erase();
//						} else {
//							assert(m_state2start[state] == start);
						}

						if (m_fast_dfa.is_finite_state(state)){
							if (start <= best_start) {
								unsigned next_pos = pos + 1;
								unsigned new_length = next_pos - start;
								if (start < best_start || new_length > best_length) {
									best_start = start;
									best_length = new_length;
									if (m_state2start.empty()) {
										int end = search(state, buffer + next_pos, buffer_size - next_pos);
										best_length += end;
										return std::pair<int, int>(best_start, best_length);
									}
								}
							}
						}

						if (different) {
							if (! m_state2start.contains(state)) {
								m_state2start[state] = start;
							} else {
								unsigned &set_start = m_state2start[state];
								if (start < set_start)
									set_start = start;
							}
						}
				}

				if (curr_it.is_end())
					break;
			}

			if ((int)best_start < 0) {
				// let's start from pos
				int state = m_fast_dfa.get_arc(init_state, iw);
				switch (state) {
					case -1:
						break;
					case -2:
						best_start = pos;
						best_length = buffer_size - pos;

						if (m_state2start.empty()){
							// Optimization. No need to process the rest of string, because
							// all other pretenders will be righter than this one.
							return std::pair<int, int>(best_start, best_length);
						}

						break;
					default:
						if (m_fast_dfa.is_finite_state(state)) {
							best_start = pos;
							best_length = 1;
							if (m_state2start.empty()) {
								unsigned next_pos = pos + 1;
								int end = search(state, buffer + next_pos, buffer_size - next_pos);
								best_length += end;
								return std::pair<int, int>(best_start, best_length);
							}
						}

						if (! m_state2start.contains(state)) {
							m_state2start[state] = pos;
						}
				}
			}
		}

	exit:
		return std::pair<int, int>(best_start, best_length);
	}
};

static dfa_matcher_i *matcher;

static void match(const char *line, size_t line_len)
{
	if (matcher->match(line, line_len)){
		fwrite(line, 1, line_len, stdout);
		fputc('\n', stdout);
	}
}

static bool option_O = false;
static void search(const char *line, size_t line_len)
{
	std::pair<int, int> result = matcher->search(line, line_len);
	if (result.first >= 0){
		if (option_O)
			printf("%d %d ", result.first, result.second);
		fwrite(line + result.first, 1, result.second, stdout);
		fputc('\n', stdout);
	}
}

static void usage()
{
	fprintf(stderr, "usage: my_grep [OPTIONS] GLOB_PATTERNs FILE\n\
where PATTERN is a glob pattern like apple* or a??le\n\
and FILE is a filename to scan.\n\
\n\
OPTIONS:\n\
   -h     --    display this screen\n\
   -o     --    show only the part of a matching line that matches GLOB_PATTERNs\n\
   -O     --    imply -o and output index and length of found string\n\
   -Wu    --    union of several glob patterns\n\
   -Wi    --    intersect several glob patterns\n\
   -Ws    --    subtract several glob patterns\n\
\n\
   -D     --    enable debugging output\n\
\n\
If FILE is '-', than stdin is read\n\
\n\
Examples:\n\
   my_grep 'apple*' /usr/share/dict/words\n\
   my_grep 'apple*' '*apple' /usr/share/dict/words\n\
   my_grep -Wu 'apple*' '*apple' /usr/share/dict/words\n\
   my_grep -Wi '*app*' '*pie*' /usr/share/dict/words\n\
   my_grep -Wi 'comp*' '*ing' /usr/share/dict/words\n\
   my_grep -Ws 'apple*' 'apple' 'apples' /usr/share/dict/words\n");
}

int main(int argc, char **argv)
{
	int opt;

	fsa_operation op = UNION;
	bool option_o = false;

	while ((opt = getopt(argc, argv, "+DhoOW:")) != -1) {
		switch (opt) {
			case 'D':
				option_D = true;
				break;
			case 'h':
				usage();
				exit(0);
			case 'o':
				option_o = true;
				break;
			case 'O':
				option_O = option_o = true;
				break;
			case 'W':
				switch (optarg[0]) {
					case 'u':
						op = UNION;
						break;
					case 'i':
						op = INTERSECT;
						break;
					case 's':
						op = SUBTRACT;
						break;
					default:
						usage();
						exit(1);
				}
				break;
			default:
				usage();
				exit(1);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 2) {
		usage();
		exit(1);
	}

	std::vector<fsa> nfas;
	nfas.resize(argc - 1);
	for (unsigned i = 0; i < argc - 1; ++i) {
		parse_glob(nfas[i], argv[i]);
	}

	fsa nfa;
	switch (op) {
		case UNION:
			union_nfa(nfa, nfas);
			break;
		case INTERSECT:
			intersect_nfa(nfa, nfas);
			break;
		case SUBTRACT:
			subtract_nfa(nfa, nfas);
			break;
		default:
			abort();
	}

	//	print_fsa(nfa);

//	dfa_matcher_iwmap<fast_dfa> matcher_iwmap;
	dfa_matcher_iwmap<fast_dfa_shift> matcher_iwmap;
	matcher_iwmap.set_nfa(nfa);
	matcher = &matcher_iwmap;

	if (option_o)
		readlines2(search, argv[argc - 1]);
	else
		readlines2(match, argv[argc - 1]);

	return 0;
}
