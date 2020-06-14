#ifndef __CPPUTILS_QUEUE_
#define __CPPUTILS_QUEUE_
#include "list.h"

template <typename T>
class queue {
public:
	queue():_list(){};
	virtual ~queue(){clear();};

	void clear() {
		_list.clear();
	}

	bool empty() const {
		return _list.empty();
	}

	int size() const {
		return _list.getSize();
	}

	void enqueue(const T& v) {
		_list.push_back(v);
	}

	T dequeue() {
		T t;
		if(_list.empty())
			return T(0);
		typename list<T>::node* f = _list.begin();
		t = f->element;
		_list.erase(f);
		
		return t;
	}
private:
	list<T> _list;
};
#endif
