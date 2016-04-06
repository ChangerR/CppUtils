#ifndef _SL_LIST__H
#define _SL_LIST__H

template <class T>
class list
{
public:
	class node {
	public:
		node(const T& e):prev(0),next(0),element(e){}
		node():prev(0),next(0){}
		virtual ~node(){}
	public:
		node* prev;
		node* next;
		T element;
	};

	list():m_size(0){
		head = new node();
		tail = new node();
		head->prev = NULL;
		tail->next = NULL;
		head->next = tail;
		tail->prev = head;
	}

	~list() {
		clear();
		delete head;
		delete tail;
	}

	int getSize() const {
		return m_size;
	}

	void clear() {
		node* p = head->next;
		while (p != tail)
		{
			head->next = head->next->next;
			delete p;
			p = head->next;
			--m_size;
		}
		head->prev = NULL;
		tail->next = NULL;
		head->next = tail;
		tail->prev = head;
	}

	bool empty() const {
		return m_size == 0;
	}

	void push_back(const T& element) {
		node *p_node = new node(element);

		tail->prev->next = p_node;
		p_node->prev = tail->prev;
		tail->prev = p_node;
		p_node->next = tail;
		m_size++;
	}

	void push_front(const T& element) {
		node *p_node = new node(element);

		p_node->next = head->next;
		head->next->prev = p_node;
		head->next  = p_node;
		p_node->prev = head;
		m_size++;
	}

	void insert(const T& element, int i) {
		if (i > m_size)
			return;
		node *p_node = new node(element);

		node* p  = head;
		for (int index = 0; index < i; index++)
			p = p->next;
		p_node->next = p->next;
		p->next->prev = p_node;
		p_node->prev = p;
		p->next = p_node;
		m_size++;
	}

	node* begin() const {
		return head->next;
	}

	node* end() const {
		return tail;
	}

	void erase(int i) {
		if (i >=(int)m_size)
			return;
		node* p = head->next;
		for (int index = 0; index < i;index++)
			p = p->next;
		p->prev->next = p->next;
		p->next->prev = p->prev;
		delete p;
		m_size--;
	}

	void erase(node* p) {
		p->prev->next = p->next;
		p->next->prev = p->prev;
		delete p;
		m_size--;
	}

	T& operator [](int m) {
		if( m < 0 || m >= m_size)
			throw "Out of list range";
		node* p = head->next;
		while(m--) {
			p = p->next;
		}
		return p->element;
	}

	void sort() {
		this->qsort(0,m_size - 1);
	}
	
#ifdef _DEBUG
#include <stdio.h>

	void print() {
		int i = 1;
		node* p = head->next;
		printf("NOTE: print all list element\n");
		while(p != tail) {
			printf("%d\t",p->element);
			if(i++ % 10 == 0) {
				printf("\n");
			}
			p = p->next;
		}
		if(i % 10 != 1)
			printf("\n");
	}
#endif
private:
	node* head;
	node* tail;
	int m_size;

	void qsort(int l,int n) {
		if( l >= n )
			return;

		int m = l;
		T t = 0;
		list<T>& array = *this;

		for(int i = l + 1;i <= n;i++) {
			if(array[i] < array[l]) {
				t = array[++m];
				array[m] = array[i];
				array[i] = t;
			}
		}
		t = array[m];
		array[m] = array[l];
		array[l] = t;
		qsort(l,m-1);
		qsort(m+1,n);

	}
};

#endif
