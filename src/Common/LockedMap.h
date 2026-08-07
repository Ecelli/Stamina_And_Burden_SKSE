#pragma once

#include <cstddef>
#include <shared_mutex>
#include <unordered_map>

template <class Key, class Value, class Hash = std::hash<Key>>
class LockedMap
{
public:
	using size_type = std::size_t;

	LockedMap() = default;
	LockedMap(const LockedMap&) = delete;
	LockedMap& operator=(const LockedMap&) = delete;

	void insert_or_assign(const Key& a_key, Value a_value)
	{
		std::unique_lock lock(m_mtx);
		// equivalent to m_map[a_key] = a_value
		m_map.insert_or_assign(a_key, std::move(a_value));
	}

	size_type erase(const Key& a_key)
	{
		std::unique_lock lock(m_mtx);
		return m_map.erase(a_key);
	}

	void clear()
	{
		std::unique_lock lock(m_mtx);
		m_map.clear();
	}

	bool get(const Key& a_key, Value& a_out) const
	{
        // Returns by value
		std::shared_lock lock(m_mtx);
		auto it = m_map.find(a_key);
		if (it == m_map.end())
			return false;
		a_out = it->second;
		return true;
	}

	bool contains(const Key& a_key) const
	{
		std::shared_lock lock(m_mtx);
		return m_map.contains(a_key);
	}

	size_type size() const
	{
		std::shared_lock lock(m_mtx);
		return m_map.size();
	}

	template <class Fn>
	void for_each(Fn&& a_fn) const
	{
		std::shared_lock lock(m_mtx);
		for (const auto& [key, value] : m_map) {
			a_fn(key, value);
		}
	}

private:
	mutable std::shared_mutex m_mtx;
	std::unordered_map<Key, Value, Hash> m_map;
};
