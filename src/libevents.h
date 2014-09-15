#pragma once 
#ifndef _LIBEVENTS_H
#define _LIBEVENTS_H

#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdarg>
#include <sstream>
#include <typeinfo>
#include <sstream>
#include <type_traits>
#include <future>
#include <list>

#ifndef WIN32
#include <assert.h>
#define _ASSERT(x) assert(x)
#define _ASSERTE(x) assert(x)
#endif
//MACROS

// Helper functions / classes

class split_impl
{
	std::vector<std::string> & operator()(const std::string &s, char delim, std::vector<std::string> &elems) {
		std::stringstream ss(s);
		std::string item;
		while (std::getline(ss, item, delim)) {
			elems.push_back(item);
		}
		return elems;
	}
	public:

	std::vector<std::string> operator()(const std::string &s, char delim) {
		std::vector<std::string> elems;
		(*this)(s, delim, elems);
		return elems;
	}
};

static split_impl split;



// Split functions courtesy of Evan Teran and StackOverflow community (http://stackoverflow.com/questions/236129/splitting-a-string-in-c)



//ITEMS 

class itembase
{
	public:
		virtual void* getVal() = 0;
		virtual std::string getType() = 0;
		virtual std::string str() = 0;
		virtual void fromStr(std::string value) = 0;
		virtual itembase* getCopy()=0;
		virtual ~itembase() { } ;
};

template <typename T>
class item : public itembase
{
	private:
		T _orig_item;
		std::string _str_type;
	public:
		virtual ~item() {}

		item(T value) : _orig_item(value), _str_type(typeid(T).name())
		{

		}

		void* getVal()
		{
			return &_orig_item;
		}

		T get()
		{
			return _orig_item;
		}

		T operator*()
		{
			return _orig_item;
		}

		std::string getType()
		{
			return _str_type;
		}

		std::string str()
		{
			std::stringstream ss;

			ss << _orig_item;

			return ss.str();
		}

		void fromStr(std::string value)
		{
			std::stringstream ss;
			ss << value;
			ss >> _orig_item;
		}

		itembase* getCopy()
		{
			auto result = new item<T>(_orig_item);

			return result;
		}
};

class itemwrapper
{
	private:
		itembase* _item;
	public:
		itemwrapper() : _item(NULL) {}
		~itemwrapper() 
		{
			if (!_item)
				delete _item;
			_item = NULL;
		}

		bool isEmpty()
		{
			return (_item == NULL);
		}

		itemwrapper(const itemwrapper& iw_source)
		{
			_item = iw_source.getItemCopy();
		}

		itembase* getItemCopy() const
		{	
			itembase* result = NULL;
			if (_item)
				result = _item->getCopy();
			return result;
		}

		template<typename T>
			void item(T value)
			{
				_item = new ::item<T>(value);
			}

		template<typename T>
			T item()
			{
				T result = ((::item<T>*)_item)->operator*();
				return result;
			}

		std::string type()
		{
			if (_item != NULL)
			{
				return _item->getType();
			}
			return NULL;
		}

		std::string toStr()
		{
			return _item->str();
		}

		void fromStr(std::string st)
		{
			_item->fromStr(st);
		}
};

class ItemsRepo
{
	private:
		std::unordered_map < std::string, itemwrapper > _types_repo;
		ItemsRepo() {}

		static ItemsRepo& I()
		{
			static ItemsRepo _repo;
			return _repo;
		}
	public:

		template<typename T>
			static void add(std::string type_name, T variable)
			{
				itemwrapper wrapper;
				wrapper.item<T>(variable);
				I()._types_repo[type_name] = wrapper;
			}

		static itemwrapper get(std::string str)
		{
			itemwrapper iw = I()._types_repo[str];
			_ASSERTE(!iw.isEmpty());
			return iw;
		}
};

class parameter_format
{
	public:
		virtual std::string operator()(std::vector<std::string> types, std::vector<std::string> values) = 0;
};

class simple_format : public parameter_format
{
	char delim;
	public:
	simple_format() : delim('~') {}
	virtual std::string operator()(std::vector<std::string> types, std::vector<std::string> values)
	{
		std::stringstream ss;
		_ASSERT(types.size() == values.size());

		for (unsigned int i = 0; i < types.size(); i++)
		{
			ss << types[i];

			if (i < types.size() - 1)
				ss << delim;
		}

		ss << ",";

		for (unsigned int i = 0; i < values.size(); i++)
		{
			ss << values[i];

			if (i < values.size() - 1)
				ss << delim;
		}

		return ss.str();
	}
};

class parameters
{
	private:
		std::vector<itemwrapper> _items;
		char delim;

	public:
		parameters() : delim('~')
	{}

		itemwrapper& operator [](int index)
		{
			return _items[index];
		}

		template<typename T>
			T at(int index)
			{
				T result;

				result = _items[index].item<T>();
				return result;
			}

		template <typename T>
			void add(T _value)
			{
				itemwrapper it;
				it.item<T> (_value);
				_items.push_back(it);

				//PM@NOTE Insert the type in the ItemsRepo for dynamic creation after
				ItemsRepo::add<T>(it.type(), _value);
			}


		//PM@TODO To support several strings formats we should do something like parameters.toStr<JSON>() or parameters.toStr<SimpleFormat>()
		// where the template type implements IStringFormat interface.
		template<class FormatClass>
			std::string toStr(FormatClass format = FormatClass())
			{
				std::vector<std::string> v_values;
				std::vector<std::string> v_types;
				unsigned int count = _items.size();

				for (unsigned int i = 0; i < count; i++)
				{
					v_types.push_back(_items[i].type());
					v_values.push_back(_items[i].toStr());
				}

				return format(v_types, v_values);
			}

		std::string toStr()
		{
			return toStr<simple_format>();
		}

		size_t size()
		{
			return _items.size();
		}

		void fromStr(std::string str)
		{
			auto _parts = split(str, ',');
			auto types = split(_parts[0], delim);
			auto values = split(_parts[1], delim);

			_ASSERT(types.size() == values.size());
			_items.clear();

			for (unsigned int i = 0; i < types.size(); i++)
			{
				itemwrapper iw = ItemsRepo::get(types[i]);
				iw.fromStr(values[i]);
				_items.push_back(iw);
			}
		}
};

class EventManager
{
	private:
		EventManager(){}
		std::unordered_map < std::string , std::list < std::function<void(parameters)> >> _events;
		//std::unordered_map<int, bool> _owners_invalid;

	public:
		static EventManager& I()
		{
			static EventManager _manager;
			return _manager;
		}

		void trigger(std::string event_name, parameters params)
		{
			auto& callbacks = _events[event_name];

			for (auto& lambda : callbacks) 
			{
				//auto f = std::async(std::launch::async ,lambda, params);
				lambda(params);
			}
		}

		void unregister_all_events_of(int owner)
		{

			/*for (auto& _event : _events)
			  {
			  auto& local_callbacks = _event.second;
			  auto _comparer = [&](std::pair < int, std::function<void(parameters)> __item) { return (__item.first == owner); };

			  local_callbacks.erase(std::remove_if(local_callbacks.begin(), local_callbacks.end(), _comparer ), local_callbacks.end());

			  }*/
		}

		void addEvent(std::string event_name, std::function<void(parameters)> callback)
		{
			_events[event_name].push_back( std::function<void(parameters)>(callback));
		}
};


class MakeParameters
{

	private:
		static void make_params_iter(parameters &params)
		{

		}

		template<typename T, typename ... types>
			static void make_params_iter(parameters &params, T it, types... _types)
			{
				params.add<T>(it);
				return make_params_iter(params, _types ...);
			}
	public:
		template <typename ... types>
			static parameters make(types... _types)
			{
				parameters params;
				make_params_iter(params, _types...);

				return params;
			}
};

#define _event_manager EventManager::I()
#define event_lambda(X)  [](parameters param) {X}
#define make_event(X) [this](parameters param) { X(param); } 
#define function_to_event(X) [](parameters param) { X(param); }

inline void register_event(const char* event_name, std::function<void(parameters)> callback)
{
	EventManager::I().addEvent(event_name, callback);
}

inline void unregister_events_of(void* owner)
{
	//EventManager::I().unregister_all_events_of((int) owner);
}

	template <typename ... types>
inline void trigger_event(std::string event, types... _parameters)
{
	parameters params = MakeParameters::make<types...>(_parameters...);
	//auto f = std::async([=](){ EventManager::I().trigger(event, MakeParameters::make<types...>(_parameters...)); });
	EventManager::I().trigger(event, params);
}

	template <>
inline void trigger_event(std::string event)
{
	EventManager::I().trigger(event, parameters());
}
#endif
