// Minimal shim for Godot's HashSet<T> — standalone CLI builds only.
// Iterates in insertion order like Godot's HashSet.
#pragma once

#include <cstdint>
#include <unordered_set>
#include <vector>

template <typename T, typename H = std::hash<T>, typename E = std::equal_to<T>>
class HashSet {
	std::vector<T> _elements;
	std::unordered_set<T, H, E> _index;

public:
	void insert(const T &p_value) {
		if (_index.insert(p_value).second) {
			_elements.push_back(p_value);
		}
	}
	bool has(const T &p_value) const { return _index.count(p_value) > 0; }
	void clear() {
		_elements.clear();
		_index.clear();
	}
	int64_t size() const { return (int64_t)_elements.size(); }
	bool is_empty() const { return _elements.empty(); }

	typename std::vector<T>::const_iterator begin() const { return _elements.begin(); }
	typename std::vector<T>::const_iterator end() const { return _elements.end(); }
};
