#include <stdio.h>
#define _DEBUG
#include "list.h"

template <typename T>
void qsort(T* array,int l,int n) {
	
	if(array == NULL || l >= n)
		return;
	int m = l;
	T t = 0;
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
	qsort(array,l,m-1);
	qsort(array,m+1,n);
}

int a[] = {
	140,125,554,544,85,57,89,46,487,334,3218,8792,55,4887,6315,5547,0
};

void printResult(int* array,int len) {
	for(int i = 1;i <= len; i++) {
		printf("%d\t",array[i - 1]);
		if(i % 6 == 0)
			printf("\n");
	}
	printf("\n");
}

int main(int argc,char** argv) {
	
	list<int> l;
	for(int i = 0; i < sizeof(a) / 4;i++)
		l.push_back(a[i]);
	
	l.sort();
	//qsort(a,0,(sizeof(a) / 4) - 1);	
	//printResult(a,sizeof(a) / 4);
	l.print();
	
	return 0;
}
