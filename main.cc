#include <iostream>

#define DEBUG

#include "cache.h"

namespace test_cache {
	template <typename KeyT, typename ValueT>
	bool lookup_update(caches::LFUCache<KeyT, ValueT> &cache, int key)
	{
		bool res = true;
		if (!cache.contains(key)) 
		{
			res = false;
			cache.insert(key, key);
		} else
		{
			cache.get(key); // we don't need this value
		}
#ifdef DEBUG
		std::cout << cache.traverse();
#endif /* DEBUG */
		return res;
	}
}


int main()
{
	using KeyValueT = int;

	size_t capacity{};
	size_t hits{};
	KeyValueT elem{};

	std::cin >> capacity;
	caches::LFUCache<int, int> cache(capacity);

	while (std::cin >> elem) {
		hits += test_cache::lookup_update(cache, elem) ? 1 : 0;
	}

	std::cout << hits << std::endl;
}
