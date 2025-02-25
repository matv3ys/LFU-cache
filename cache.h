#include <stdexcept>
#include <cassert>
#include <unordered_map>
#include <set>
#include <string>
#include <sstream>

#ifdef DEBUG
namespace 
{
	std::string pointer_to_string(const void *p)
	{
		std::stringstream ss;
		ss << p;
		return ss.str();
	}
}
#endif /* DEBUG */

namespace caches {
template <typename KeyT, typename ValueT>
class LFUCache 
{
	struct FreqNode;

	struct LFUItem {
		ValueT value;
		FreqNode *parent;
	public:
		LFUItem(ValueT value, FreqNode *parent): value(value), parent(parent) {}
	};

	struct FreqNode 
	{
		unsigned value = 0;
		FreqNode *prev = nullptr;
		FreqNode *next = nullptr;
		std::set<KeyT> keys;
	public:
		FreqNode(unsigned value, FreqNode *prev, FreqNode *next) : value(value), prev(prev), next(next) 
		{
			if (prev != nullptr)
				prev->next = this;
			if (next != nullptr)
				next->prev = this;
		}
		FreqNode(const FreqNode& other) = delete;
		FreqNode(FreqNode&& other) = delete;
		FreqNode& operator=(const FreqNode& other) = delete;
		FreqNode& operator=(FreqNode&& other) = delete;
		~FreqNode() = default;
	};

	class FreqList 
	{
		FreqNode *first_elem_ = nullptr;

		void clear_list()
		{
			FreqNode *tmp;
			while (first_elem != nullptr)
			{
				tmp = first_elem;
				first_elem = tmp->next;
				delete tmp
			}
		}

	public:
		FreqList() = default;

		FreqList(const FreqList& other) = delete;
		FreqList(FreqList&& other)
		{
			first_elem_ = other.first_elem_;
			other.first_elem_ = nullptr;
		}

		FreqList& operator=(const FreqList& other) = delete;
		FreqList& operator=(FreqList&& other)
		{
			if ( this != &other ) 
			{
				clear_list();
				first_elem_ = other.first_elem_;
				other.first_elem_ = nullptr;
			}
			return *this;
		}

		~FreqList()
		{
			clear_list();
		}
	}

	size_t size_ = 0;
	size_t capacity_ = 10;
	std::unordered_map<KeyT, LFUItem> table;
	FreqNode *freq_head_ = nullptr;

	FreqNode *get_new_freq_node(unsigned value, FreqNode *prev, FreqNode *next)
	{
		FreqNode *new_node;

		if (prev != nullptr)
			assert(prev->next == next);
		if (next != nullptr)
			assert(next->prev == prev);
		
		return new FreqNode(value, prev, next);
	}

	void *delete_freq_node(FreqNode *node) {
		FreqNode *next = node->next, *prev = node->prev;
		
		if (node->prev != nullptr)
			node->prev->next = next;
		if (node->next != nullptr)
			node->next->prev = prev;
		delete node;

		return next;
	}

	void pop_lfu_elem()
	{
		auto key = *(freq_head_->keys.begin());
		freq_head_->keys.erase(key);
		table.erase(key);
		if (freq_head_->keys.empty())
		{
			FreqNode *next = freq_head_->next;
			delete_freq_node(freq_head_);
			freq_head_ = next;
		}
	}

public:

	LFUCache(size_t capacity): capacity_(capacity) {}

#ifdef DEBUG
	/**
	 * Стоит под макросом, так как std::to_string определен не для каждого типа
	 */
	std::string traverse()
	{
		std::string res;
		FreqNode *p = freq_head_;

		while (p != nullptr)
		{
			res += "---------\n";
			res += pointer_to_string(static_cast<const void*>(p)) + " " + std::to_string(p->value) + "\n";
			res += "PREV: " + pointer_to_string(static_cast<const void*>(p->prev)) + "\n";
			res += "NEXT: " + pointer_to_string(static_cast<const void*>(p->next)) + "\n";
			for (auto &elem: p->keys)
				res += std::to_string(elem) + " ";
			res += "\n---------\n";
			p = p->next;
		}
		return res;
	}
#endif

	bool contains(KeyT key)
	{
		if (table.find(key) != table.end())
			return true;
		return false;
	}

	void insert(KeyT key, ValueT value) 
	{
		if (contains(key))
		{
			throw std::invalid_argument("Key already exists");
		}

		if (size_ == capacity_) 
		{
			pop_lfu_elem();
		} else {
			size_++;
		}

		if (freq_head_ == nullptr || freq_head_->value != 1)
		{
			freq_head_ = get_new_freq_node(1, nullptr, freq_head_);
		}
		freq_head_->keys.insert(key);
		table.insert({key, LFUItem(value, freq_head_)});
	}

	ValueT get(KeyT key) 
	{
		FreqNode *freq, *next_freq;

		auto it = table.find(key);
		if (it == table.end())
		{
			throw std::invalid_argument("Key not found");
		}

		LFUItem &item = it->second;
		freq = item.parent;
		next_freq = freq->next;
		if (next_freq == nullptr || next_freq->value != freq->value + 1)
		{
			next_freq = get_new_freq_node(freq->value + 1, freq, next_freq);
		}
		next_freq->keys.insert(key);
		item.parent = next_freq;
		
		freq->keys.erase(key);
		if (freq->keys.empty())
		{
			delete_freq_node(freq);
			if (freq_head_ == freq)
			{
				freq_head_ = next_freq;
			}
		}
		return item.value;
	}

	~LFUCache()
	{
		FreqNode *elem;

		while (freq_head_ != nullptr)
		{
			elem = freq_head_;
			freq_head_ = elem->next;
			delete elem;
		}
	}
};
}
