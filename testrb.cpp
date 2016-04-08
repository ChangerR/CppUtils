#include "rbtree.h"

int main(int argc,char** argv) {
	RBTree<int,int> tree;
	for(int i = 0;i < 20; i++) {
		tree.insert(i,i);
	}
	tree.debug();
	tree.erase(4);
	tree.debug();
	tree.erase(7);
	tree.debug();
	tree.erase(8);
	tree.debug();
	tree.erase(15);
	tree.debug();


	return 0;
}
