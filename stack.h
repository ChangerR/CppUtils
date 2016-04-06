#ifndef __CPPUTILS_STACK_
#define __CPPUTILS_STACK_
#include "list.h"

template <typename T>
class stack {
public:
	stack():_list(){};
	virtual ~stack(){clear();};

	void clear() {
		_list.clear();
	}

	bool empty() const {
		return _list.empty();
	}

	int size() const {
		return _list.getSize();
	}

	void push(const T& v) {
		_list.push_front(v);
	}

	T pop() {
		T t;
		if(_list.empty())
			return T(0);
		list<T>::node* f = _list.begin();
		t = f->element;
		_list.erase(f);
		
		return t;
	}
private:
	list<T> _list;
};
#endif
