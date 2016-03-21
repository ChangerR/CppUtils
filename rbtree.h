#ifndef __CPPUTILS_RBTREE__
#define __CPPUTILS_RBTREE__

template <class KeyTyepe,class ValueType>
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

		bool isRoot {return _parent == NULL;}

		bool isLeft {return _parent && _parent->_leftc == this;}

		bool isRight{return _parent && _parent->_rightc == this;}

		bool isLeaf {return _leftc == NULL && _rightc == NULL;}

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
	typedef Node<KeyTyepe,ValueType> RBNode;

private:
	int _size;
	Node* _root;
};

#endif
