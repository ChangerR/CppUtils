#ifndef __CPPUTILS_RBTREE__
#define __CPPUTILS_RBTREE__
#include "stack.h"
#include "queue.h"
#include <stdio.h>

template <class KeyType,class ValueType>
class RBTree {
	
	template <class K,class V>
	class Node {
	public:
		Node(const K& k,const V& v):_key(k),_value(v),_parent(NULL),_leftc(NULL),_rightc(NULL),_isRed(true){}
		Node():_parent(NULL),_leftc(NULL),_rightc(NULL),_isRed(true),_key(),_value(){}

		void setLeft(Node* l) {
			_leftc = l;
			if(l != NULL) l->_parent = this;
		}

		void setRight(Node* r) {
			_rightc = r;
			if(r != NULL) r->_parent = this;
		}

		void setParent(Node* p) {
			_parent = p;
		}

		void setValue(const V& v) {
			_value = v;
		}

		void setRed() {_isRed = true;}

		void setBlack() {_isRed = false;}

		V& getValue() {return _value;}

		K& getKey() {return _key;}

		bool isRoot() {return _parent == NULL;}

		bool isLeft() {return _parent && _parent->_leftc == this;}

		bool isRight() {return _parent && _parent->_rightc == this;}

		bool isLeaf() {return _leftc == NULL && _rightc == NULL;}

		bool getLeftColor(){ return _leftc != NULL && _leftc->_isRed == true;}

		bool getRightColor(){ return _rightc != NULL && _rightc->_isRed == true;}

	private:
		K _key;
		V _value;
		Node* _parent;
		Node* _leftc;
		Node* _rightc;
		bool _isRed;
		friend class RBTree<K,V>;
	};

public:
	typedef Node<KeyType,ValueType> RBNode;

	RBTree():_root(NULL),_size(0){}
	virtual ~RBTree(){clear();}

	bool insert(const KeyType& k,const ValueType& v) {

		RBNode* n = new RBNode(k,v);
		if(insert_without_balance(n) == false) {
			delete n;
			return false;
		}
		
		while(n != _root) {
			if(n->_parent->_isRed == false)
				break;
			
			if(n->_parent->isLeft()) {

				//case 1
				if(n->_parent->_parent->getRightColor() == true) {
					n->_parent->setBlack();
					n->_parent->_parent->setRed();
					n->_parent->_parent->_rightc->setBlack();
					n = n->_parent->_parent;
				}else {
					//case 2
					if(n->isRight() == true) {
						n = n->_parent;
						rotateLeft(n);
					}
					//case 3
					n->_parent->setBlack();
					n->_parent->_parent->setRed();
					rotateRight(n->_parent->_parent);
				}
			}else{

				//case 1
				if(n->_parent->_parent->getLeftColor() == true) {
					n->_parent->setBlack();
					n->_parent->_parent->setRed();
					n->_parent->_parent->_leftc->setBlack();
					n = n->_parent->_parent;
				}else {
					//case 2
					if(n->isLeft() == true) {
						n = n->_parent;
						rotateRight(n);
					}
					//case 3
					n->_parent->setBlack();
					n->_parent->_parent->setRed();
					rotateLeft(n->_parent->_parent);
				}
			}

		}
		_root->setBlack();
		return true;
	}
	
	bool empty() const {
		return _size == 0;
	}

	int size() const {
		return _size;
	}
	
	void clear() {
		stack<RBNode*> s;

		if(_root)s.push(_root);

		while(!s.empty()) {
			RBNode* n = s.pop();
			if(n->_rightc)s.push(n->_rightc);
			if(n->_leftc)s.push(n->_leftc);
			delete n;
		}
		_size = 0;
		_root = NULL;
	}
	
	void erase(const KeyType& key) {
		RBNode* n = search(key);

		if(n != NULL)
			erase(n);
	}

	void debug() {
		queue<RBNode*> s1,s2;
		queue<RBNode*> *ps_current,*ps_backup;

		ps_current = &s1;
		ps_backup = &s2;
		
		ps_current->enqueue(_root);

		while(!ps_current->empty()) {
			RBNode* p = ps_current->dequeue();
			if(p == NULL) {
				printf("NIL ");
			}else {
				printf("%d(%s) ",p->getKey(),p->_isRed ? "RED":"BLACK");
				ps_backup->enqueue(p->_leftc);
				ps_backup->enqueue(p->_rightc);
			}
			if(ps_current->empty() == true&&ps_backup->empty() == false) {
				queue<RBNode*> *p = ps_current;
				ps_current = ps_backup;
				ps_backup = p;
				printf("\n");
			}
		}
		printf("\n");
	}
	
private:

	bool insert_without_balance(RBNode* n) {

		if(_root == NULL) {
			_root = n;
		} else {
			RBNode* p = _root;

			while(p) {
				if(p->_key == n->_key) {
					p->_value = n->_value;
					return false;
				}else if(n->_key < p->_key) {
					if(p->_leftc == NULL) {
						p->setLeft(n);
						p = NULL;
					}else
						p = p->_leftc;
				}else{
					if(p->_rightc == NULL) {
						p->setRight(n);
						p = NULL;
					}else
						p = p->_rightc;
				}
		
			}
		}
		++_size;
		return true;
	}

	void treeTransplant(RBNode* u,RBNode* v) {
		if(u == _root) {
			_root = v;
		}else if(u->isLeft()) {
			u->_parent->_leftc = v;
		}else {
			u->_parent->_rightc = v;
		}
		v->_parent = u->_parent;
	}

	int erase(RBNode* z) {
		RBNode* x,*y;
		bool y_original_color = z->_isRed;
		y = z;
		
		if(z->_leftc == NULL) {
			x = z->_rightc;
			if(x == NULL) {
				_erase_p = z->_parent;
				if(z->isLeft())
					_erase_p->_leftc = NULL;
				else
					_erase_p->_rightc = NULL;
			}else
				treeTransplant(z,x);
		} else if(z->_rightc == NULL) {
			x = z->_leftc;
			treeTransplant(z,x);
		}else {
			y = treeMinimum(z->_rightc);
			y_original_color = y->_isRed;
			x = y->_rightc;
			if(y->_parent != z) {
				if(x == NULL) {
					_erase_p = y->_parent;
					if(y->isLeft())
						_erase_p->_leftc = NULL;
					else
						_erase_p->_rightc = NULL;
				}else
					treeTransplant(y,x);
				y->_rightc = z->_rightc;
				y->_rightc->_parent = y;
			}else if(x == NULL) {
				_erase_p = y;
			}
			treeTransplant(z,y);
			y->_leftc = z->_leftc;
			y->_leftc->_parent = y;
			y->_isRed = z->_isRed;
		}
		
		delete z;
		--_size;

		if(!y_original_color){
			rbtreeDeleteFixup(x);		
		}

		return _size;
	}

	void rbtreeDeleteFixup(RBNode* x) {
		RBNode* w,*p;
		while(x != _root && (x == NULL || x->_isRed == false)) {
			if((x == NULL && _erase_p->_leftc == NULL)||x->isLeft()) {
				p = (x == NULL) ? _erase_p: x->_parent;
				w = p->_rightc;
				//case 1
				if(w->_isRed) {
					p->setRed();
					w->setBlack();
					rotateLeft(p);
				}else if(w->getLeftColor() == false && w->getRightColor() == false) {
					//case 2
					w->setRed();
					x = p;
				}else{
					//case 3
					if(w->getRightColor() == false) {
						w->_leftc->setBlack();
						w->setRed();
						rotateRight(w);
					}
					//case 4
					w->_isRed = p->_isRed;
					p->setBlack();
					w->_rightc->setBlack();
					rotateLeft(p);
					x = _root;
				}

			}else{
				p = (x == NULL) ? _erase_p:x->_parent;
				w = p->_leftc;
				//case 1
				if(w->_isRed) {
					p->setRed();
					w->setBlack();
					rotateRight(p);
				}else if(w->getLeftColor() == false && w->getRightColor() == false) {
					//case 2
					w->setRed();
					x = p;
				}else {
					//case 3
					if(w->getLeftColor() == false) {
						w->_rightc->setBlack();
						w->setRed();
						rotateLeft(w);
					}
					//case 4
					w->_isRed = p->_isRed;
					p->setBlack();
					w->_leftc->setBlack();
					rotateRight(p);
					x = _root;
				}
			}
		}
		x->setBlack();
	}

	RBNode* search(const KeyType& k) {
		RBNode* n = _root;
		while(n) {
			if(k == n->getKey())
				break;
			else if(k < n->getKey())
				n = n->_leftc;
			else
				n = n->_rightc;
		}
		return n;
	}

	void rotateLeft(RBNode* x) {
		RBNode* y = x->_rightc;

		x->setRight(y->_leftc);

		if(x == _root)
			_root = y;
		else {
			if(x->isLeft()) {
				x->_parent->setLeft(y);
			}else{
				x->_parent->setRight(y);
			}
		}
		y->setLeft(x);
	}

	void rotateRight(RBNode* y) {
		RBNode* x = y->_leftc;

		y->setLeft(x->_rightc);

		if(y == _root)
			_root = y;
		else {
			if(y->isLeft()) {
				y->_parent->setLeft(x);
			} else {
				y->_parent->setRight(x);
			}
		}
		x->setRight(y);
	}

	RBNode* treeMaximum(RBNode* x) {
		while(x->_rightc != NULL)
			x = x->_rightc;
		return x;
	}

	RBNode* treeMinimum(RBNode* x) {
		while(x->_leftc != NULL)
			x = x->_leftc;
		return x;
	}

	int _size;
	RBNode* _root;
	RBNode* _erase_p;
};

#endif
