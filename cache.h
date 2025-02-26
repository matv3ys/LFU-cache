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
		unsigned value;
		FreqNode *prev = nullptr;
		FreqNode *next = nullptr;
		std::set<KeyT> keys;

		//TODO check for call for ctor without params
		FreqNode(unsigned value) : value(value) {}

		/* BIG 5*/
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
			while (first_elem_ != nullptr)
			{
				tmp = first_elem_;
				first_elem_ = tmp->next;
				delete tmp;
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

#if 0
		FreqNode *get_new_freq_node(unsigned value, FreqNode *prev, FreqNode *next)
		{
			FreqNode *new_node;

			if (prev != nullptr)
				assert(prev->next == next);
			if (next != nullptr)
				assert(next->prev == prev);
			
			return new FreqNode(value, prev, next);
		}
#endif

		bool is_empty()
		{
			return first_elem_ == nullptr;
		}

		FreqNode* pop_front() // CONST наверное не нужен
		{
			return first_elem_;
		}

		void push_front(FreqNode* node)
		{
			node->next = first_elem_;
			node->prev = nullptr;
			first_elem_ = node;
		}

		FreqNode *insert_node(FreqNode *node, FreqNode *prev)
		{
			assert(prev != nullptr);
			node->prev = prev;
			node->next = prev->next;
			prev->next = node;
			return node;
		}

		void delete_node(FreqNode *node) {
			FreqNode *next = node->next, *prev = node->prev;
			
			if (prev != nullptr)
				prev->next = next;
			if (next != nullptr)
				next->prev = prev;
			
			if (first_elem_ == node)
				first_elem_ = node->next;

			delete node;
		}

	};

	size_t size_ = 0;
	size_t capacity_ = 10;
	std::unordered_map<KeyT, LFUItem> table_;
	FreqList freq_list_;

	void delete_lfu_elem()
	{
		FreqNode *first_elem = freq_list_.pop_front(); 
		auto key = *(first_elem->keys.begin());
		first_elem->keys.erase(key);
		table_.erase(key);
		if (first_elem->keys.empty())
		{
			freq_list_.delete_node(first_elem);
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
		FreqNode *p = freq_list_.pop_front();

		res += "***************************************\n";
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
		res += "***************************************\n";
		return res;
	}
#endif

	bool contains(KeyT key)
	{
		if (table_.find(key) != table_.end())
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
			delete_lfu_elem();
		} else 
		{
			size_++;
		}


		if (freq_list_.is_empty() || freq_list_.pop_front()->value != 1)
		{
			freq_list_.push_front(new FreqNode{1});
		}
		FreqNode *node = freq_list_.pop_front();
		node->keys.insert(key);
		table_.insert({key, LFUItem(value, node)});
	}

	ValueT get(KeyT key) 
	{
		FreqNode *freq, *next_freq;

		auto it = table_.find(key);
		if (it == table_.end())
		{
			throw std::invalid_argument("Key not found");
		}

		LFUItem &item = it->second;
		freq = item.parent;
		next_freq = freq->next;
		if (next_freq == nullptr || next_freq->value != freq->value + 1)
		{
			next_freq = freq_list_.insert_node(new FreqNode{freq->value + 1}, freq);
		}
		next_freq->keys.insert(key);
		item.parent = next_freq;
		
		freq->keys.erase(key);
		if (freq->keys.empty())
		{
			freq_list_.delete_node(freq);
		}
		return item.value;
	}

	~LFUCache() = default;
};
}
