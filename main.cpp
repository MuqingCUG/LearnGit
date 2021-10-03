//
//  main.cpp
//  mydeque
//
//  Created by limuqing on 2021/7/9.
//

#include <iostream>
#include "stdfax.h"
//#include "my_deque.h"
#include "Deque.h"
int main(int argc, const char * argv[]) {
    // insert code here...
    Deque<int> test;
    for(int i =0;i<30; ++i)
    {
        test.push_back(i+7);
    }
    auto it = test.begin();
    for(;it!=test.end();++it)
    {
        cout << *it <<"\t";
    }
    cout << endl;
    
    cout << test.front() << endl;
    cout << test.back() << endl;
    
    
    return 0;
}
