## Question1：容器的安全问题。

- double free

  我所知道的会引起double free的就是浅拷贝。

  接下来用一个例子来说明:

  ```c++
  - #include <iostream>
    using namespace std;
  
    class test
    {
    public:
    	test(int m_integer,int m_pointer) {
    		integer = m_integer;
    		pointer = new int(m_pointer);
    		cout << "模拟的默认构造函数调用" << endl;
    	}
    	~test() {
    		cout << "test析构函数调用" << endl;
    	}
    	int integer;
    	int *pointer;
    };
  
    void fun() {
    	test demo(10,20);
  	cout << "demo中的整数为" << demo.integer << "指针中的地址" << demo.pointer << endl;
  	test demo1(demo);
  	cout << "demo1中的整数为" << demo1.integer << "指针中的地址" << demo1.pointer << endl; 	
    }
  
    int main() {
    	fun();
    	system("pause");
    }
  
  
  ```

  

  ![image-20200529013524155](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200529013524155.png)

我们可以看到demo和demo1中的指针是指向同一个地方并且会调用两次析构函数，如果我们将析构函数改为下面这样，就会出现double free的情况。

```c++
~test() {
		if (pointer != NULL)
			delete pointer;
             pointer = NULL;
		cout << "test析构函数调用" << endl;
	}
```



- erase函数误用

  先来看错误的用法

  ```c++
  #include <iostream>
  #include <vector>
  using namespace std;
  int main( ) {
      vector<int> demo;
      int i;
      for (i = 0; i < 5; i++ ) {
      demo.push_back( i );
      }
      for ( vector<int>::iterator it = demo.begin(); it != demo.end();++it) {
      	if ( *it == 4 ){ 
          	demo.erase(it);
      }
      int Size = demo.size();
      for (  i = 0 ; i < Size; i++ )  {
      cout << " i= " << i <<  ", " << demo[ i ] << endl;
      }
  	return 0; 
  }
  ```

  vector是关联式容器，调用erase之后，删除位置之后的迭代器就会失效，所以erase之后再去使用失效的迭代器就会出现问题，会访问到非法内存。

  erase调用后会返回被删除的元素的后继迭代器所以我们改成下面这样就可以解决此问题。

  ```c++
  for ( vector<int>::iterator it = demo.begin(); it != demo.end();) {
      	if ( *it == 4 )
  		{ 
          it=demo.erase(it);
      	}else{
      		++it;
  		}	
      }
  ```

  

  还有一个就是sgi stl中的erase函数没有检查边界问题，也就是会发生越界。

  ```c++
  #include <iostream>
  #include <vector>
  using namespace std;
  int main( ) {
      vector<int> demo;
      demo.push_back( 1 );
      vector<int>::iterator it = demo.begin();
      demo.erase( it+2 );//越界
      return 0;
  }
  ```

  

## Question2：

我觉得应该就是是否已经初始化的区别。如果目标区间未初始化就调用uninitialized_XXX，否则XXX。至于为什么我不知道(逃)。

## Question3：内存图

![image-20200529165837758](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200529165837758.png)





![image-20200529165901260](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200529165901260.png)





- 小作业

```
#include <iostream>
#include <cstdlib>
#include <cstring>
using namespace std;
class Element {
private:
    int number;
public:
    Element() :number(0) {
        cout << "ctor" << endl;
    }
    Element(int num) :number(num) {
        cout << "ctor" << endl;
    }
    Element(const Element& e) :number(e.number) {
        cout << "copy ctor" << endl;
    }
    Element(Element&& e) :number(e.number) {
        cout << "right value ctor" << endl;
    }
    ~Element() {
        cout << "dtor" << endl;
    }
    void operator=(const Element& item) {
        number = item.number;
    }
    bool operator==(const Element& item) {
        return (number == item.number);
    }
    void operator()() {
        cout << number;
    }
    int GetNumber() {
        return number;
    }
};
template<typename T>
class Vector {
private:
    T* items;
    int count;
public:
    Vector() :count{ 0 }, items{ nullptr } {

    }
    Vector(const Vector& vector) :count{ vector.count } {
        items = static_cast<T*>(malloc(sizeof(T) * count));
        memcpy(items, vector.items, sizeof(T) * count);
    }
    Vector(Vector&& vector) :count{ vector.count }, items{ vector.items } {
        //TODO
        vector.count = 0;
        vector.items = nullptr;
    }
    ~Vector() {
        //TODO
        clear();
    }
    T& operator[](int index) {
        if (index < 0 || index >= count) {
            cout << "invalid index" << endl;
            return items[0];
        }
        return items[index];
    }
    int returnCount() {
        return count;
    }
    void Clear() {
        //TODO
        auto first = items;
        for (; first != items+count;++first) {
            first->~T();
        }
        free(items);
        items = nullptr;
        count = 0;
    }
    
    void Add(const T& item) {
        //TODO
        auto new_items = static_cast<T*>(malloc(sizeof(T) * (count + 1)));
        //第一部分
        for (int i = 0;i < count;i++) {
            new(&new_items[i]) T(move(itmes[i]));
        }
        //第二部分
        new(&new_items[count]) T(move(item));
        clear();
        count = count + 1;
        items = new_items;
    }
    bool Insert(const T& item, int index) {
        //TODO
        if (index + 1 > count || index < 0)
            return false;
        auto new_items = static_cast<T*>(malloc(sizeof(T) * (count + 1)));
        //第一部分
        for (int i = 0;i < index;i++) {
            new(&new_items[i]) T(move(itmes[i]));
        }
        //插入的值
        new(&new_items[index]) T(move(item));
        //第三部分
        for (int i = index+1;i < count+1;i++) {
            new(&new_items[i]) T(move(items[i - 1]));
        }
        clear();
        count = count + 1;
        items = new_items;
    }
    bool Remove(int index) {
        //TODO
        if (index + 1 > count || index < 0)
            return false;
        auto new_items = static_cast<T*>(malloc(sizeof(T) * (count - 1)));
        //分两部分move
        for (int i = 0;i < index;i++) {
            new(&new_items[i]) T(move(itmes[i]));
            
        }
        //第二部分
        for (int i = index;i < count-1;i++) {
            new(&new_items[i]) T(move(items[i + 1]));
        }
        clear();
        count = count - 1;
        items = new_items;
        return ture;
    }
    int Contains(const T& item) {
        //TODO
        for (int i;i < count;i++) {
            if (items[i] == item) {
                return i;
            }
            else {
                return -1;
            }
        }
    }

};
template<typename T>
void PrintVector(Vector<T>& v) {
    int count = v.returnCount();
    for (int i = 0; i < count; i++)
    {
        v[i]();
        cout << " ";
    }
    cout << endl;
}
int main() {
    Vector<Element>v;
    for (int i = 0; i < 4; i++) {
        Element e(i);
        v.Add(e);
    }
    PrintVector(v);
    Element e2(4);
    if (!v.Insert(e2, 10))
    {
        v.Insert(e2, 2);
    }
    PrintVector(v);
    if (!v.Remove(10))
    {
        v.Remove(2);
    }
    PrintVector(v);
    Element e3(1), e4(10);
    cout << v.Contains(e3) << endl;
    cout << v.Contains(e4) << endl;
    Vector<Element>v2(v);
    Vector<Element>v3(move(v2));
    PrintVector(v3);
    v2.Add(e3);
    PrintVector(v2);
    return 0;
}
```

  