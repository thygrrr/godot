// Minimal shim for Godot's HashMap<K, V> — standalone CLI builds only.
// Iterates in insertion order like Godot's HashMap, so passes emit identical SPIR-V in the CLI and engine.
#pragma once

#include <cstdint>
#include <deque>
#include <unordered_map>

template <typename K, typename V>
struct KeyValue {
	const K key;
	V value;
};

template <typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>>
class HashMap {
	std::deque<KeyValue<K, V>> _elements; // Stable addresses for getptr().
	std::unordered_map<K, V *, H, E> _index;

public:
	V &operator[](const K &p_key) {
		V *value = getptr(p_key);
		return value ? *value : insert(p_key, V());
	}
	const V &operator[](const K &p_key) const { return *_index.at(p_key); }

	bool has(const K &p_key) const { return _index.count(p_key) > 0; }

	V *getptr(const K &p_key) {
		auto it = _index.find(p_key);
		return it != _index.end() ? it->second : nullptr;
	}
	const V *getptr(const K &p_key) const {
		auto it = _index.find(p_key);
		return it != _index.end() ? it->second : nullptr;
	}

	V &insert(const K &p_key, const V &p_value) {
		V *existing = getptr(p_key);
		if (existing) {
			return *existing = p_value;
		}
		_elements.push_back({ p_key, p_value });
		return *(_index[p_key] = &_elements.back().value);
	}
	void clear() {
		_elements.clear();
		_index.clear();
	}
	int64_t size() const { return (int64_t)_elements.size(); }
	bool is_empty() const { return _elements.empty(); }

	typename std::deque<KeyValue<K, V>>::iterator begin() { return _elements.begin(); }
	typename std::deque<KeyValue<K, V>>::iterator end() { return _elements.end(); }
	typename std::deque<KeyValue<K, V>>::const_iterator begin() const { return _elements.begin(); }
	typename std::deque<KeyValue<K, V>>::const_iterator end() const { return _elements.end(); }
};
