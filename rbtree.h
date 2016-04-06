#ifndef __CPPUTILS_RBTREE__
#define __CPPUTILS_RBTREE__
#include "stack.h"

template <class KeyType,class ValueType>
class RBTree {
	
	template <class K,class V>
	class Node {
	public:
		Node(const K& k,const V& v):_key(k),_value(v),_parent(NULL),_leftc(NULL),_rightc(NULL),_isRed(true){}
		Node():_parent(NULL),_leftc(NULL),_rightc(NULL),_isRed(true){}

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
		size = 0;
		_root = NULL;
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

	int _size;
	Node* _root;
};

#endif
