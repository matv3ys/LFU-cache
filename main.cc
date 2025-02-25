#include <iostream>
#include "cache.h"

namespace test_cache {
	template <typename KeyT, typename ValueT>
	bool lookup_update(caches::LFUCache<KeyT, ValueT> &cache, int key)
	{
		if (!cache.contains(key)) {
			cache.insert(key, key);
			std::cout << cache.traverse();
			return false;
		}
		cache.get(key); // we don't need this value
		std::cout << cache.traverse();
		return true;
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
