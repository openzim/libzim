/*
 * File:   lrucache.hpp
 * Author: Alexander Ponomarev
 *
 * Created on June 20, 2013, 5:09 PM
 */

#ifndef _LRUCACHE_HPP_INCLUDED_
#define	_LRUCACHE_HPP_INCLUDED_

#include <map>
#include <list>
#include <cstddef>
#include <stdexcept>

namespace zim {

template<typename key_t, typename value_t>
class lru_cache {
public: // types
	typedef typename std::pair<key_t, value_t> key_value_pair_t;
	typedef typename std::list<key_value_pair_t>::iterator list_iterator_t;

  class AccessResult
  {
    const bool hit_;
    const value_t val_;
  public:
    explicit AccessResult(const value_t& val) : hit_(true), val_(val) {}
    AccessResult() : hit_(false), val_() {}

    bool hit() const { return hit_; }
    bool miss() const { return !hit(); }
    const value_t& value() const
    {
      if ( miss() )
        throw std::range_error("There is no such key in cache");
      return val_;
    }

    operator const value_t& () const { return value(); }
  };

public: // functions
	explicit lru_cache(size_t max_size) :
		_max_size(max_size) {
	}

	void put(const key_t& key, const value_t& value) {
		auto it = _cache_items_map.find(key);
		if (it != _cache_items_map.end()) {
			_cache_items_list.splice(_cache_items_list.begin(), _cache_items_list, it->second);
      it->second->second = value;
		} else {
      _cache_items_list.push_front(key_value_pair_t(key, value));
		  _cache_items_map[key] = _cache_items_list.begin();
      if (_cache_items_map.size() > _max_size) {
        auto last = _cache_items_list.end();
        last--;
        _cache_items_map.erase(last->first);
        _cache_items_list.pop_back();
      }
    }
	}

	AccessResult get(const key_t& key) {
		auto it = _cache_items_map.find(key);
		if (it == _cache_items_map.end()) {
			return AccessResult();
		} else {
			_cache_items_list.splice(_cache_items_list.begin(), _cache_items_list, it->second);
			return AccessResult(it->second->second);
		}
	}

	bool exists(const key_t& key) const {
		return _cache_items_map.find(key) != _cache_items_map.end();
	}

	size_t size() const {
		return _cache_items_map.size();
	}

private: // data
	std::list<key_value_pair_t> _cache_items_list;
	std::map<key_t, list_iterator_t> _cache_items_map;
	size_t _max_size;
};

} // namespace zim

#endif	/* _LRUCACHE_HPP_INCLUDED_ */
