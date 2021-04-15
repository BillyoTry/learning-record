## SGI  STL学习

mystl（未完待续）

### STL简单概述

1.STL组成：

- 容器（Container）
- 迭代器（Iterator）
- 算法（Algorithm）
- 仿函数（Function object）
- 适配器（Adaptor）
- 空间配置器（allocator）

2.STL六大组件内在联系：

- **空间配置器**为**容器**分配内存空间。

> 注：无论分配任何类型内存，**分配器**仅进行内存分配，**不**调用构造函数。

- **容器**与**算法**间需要**迭代器**将两者连接起来。

- **算法**常常搭配**仿函数**来完成工作。

- **容器**的相当一部分关于**迭代器**的操作是在容器内部实现。

  

### 详细信息

1.配置器

- 容器使用配置器来进行内存的**分配**与**释放**，相关头文件为`stl_alloc.h`等。

  两种空间分配器如下：

  i.第一级分配器__malloc_alloc_template，即时分配即使释放。

  ii.第二级分配器__default_alloc_template，小型内存池。

- 容器调用配置器的静态函数来**分配**与**释放**内存，而归根结底它的底层是通过`malloc`和`free`来实现。

2.迭代器

- 迭代器本质上就是一个泛型指针。

- 算法使用迭代器对容器来进行相关操作，致使它不必了解容器本身结构。

- 迭代器相关：

  i.迭代器的相关实现（相关源代码位于`stl_iterator.h``和``stl_iterator_base.h`）

  ```
  template <class _Category, class _Tp, class _Distance = ptrdiff_t,
      class _Pointer = _Tp *, class _Reference = _Tp &>
      struct iterator {
      typedef _Category  iterator_category;   // 迭代器的种类
      typedef _Tp        value_type;          // 迭代器所指对象的类型
      typedef _Distance  difference_type;     // 两两迭代器之间，距离的类型
      typedef _Pointer   pointer;             // 迭代器所指对象的指针
      typedef _Reference reference;           // 迭代器所指对象的引用
  };
  ```

  ii.迭代器类型

  ```c++
  // 迭代器的tag。对应类型的迭代器中，其iterator_category就是对应的tag
  struct input_iterator_tag {};
  struct output_iterator_tag {};
  struct forward_iterator_tag : public input_iterator_tag {};
  struct bidirectional_iterator_tag : public forward_iterator_tag {};
  struct random_access_iterator_tag : public bidirectional_iterator_tag {};
  
  // 输入迭代器
  template <class _Tp, class _Distance> struct input_iterator {
      typedef input_iterator_tag iterator_category;
      typedef _Tp                value_type;
      typedef _Distance          difference_type;
      typedef _Tp* pointer;
      typedef _Tp& reference;
  };
  // 输出迭代器
  struct output_iterator {
      typedef output_iterator_tag iterator_category;
      typedef void                value_type;
      typedef void                difference_type;
      typedef void                pointer;
      typedef void                reference;
  };
  // 正向迭代器
  template <class _Tp, class _Distance> struct forward_iterator {
      typedef forward_iterator_tag iterator_category;
      typedef _Tp                  value_type;
      typedef _Distance            difference_type;
      typedef _Tp* pointer;
      typedef _Tp& reference;
  };
  // 双向迭代器
  template <class _Tp, class _Distance> struct bidirectional_iterator {
      typedef bidirectional_iterator_tag iterator_category;
      typedef _Tp                        value_type;
      typedef _Distance                  difference_type;
      typedef _Tp* pointer;
      typedef _Tp& reference;
  };
  // 随机访问迭代器
  template <class _Tp, class _Distance> struct random_access_iterator {
      typedef random_access_iterator_tag iterator_category;
      typedef _Tp                        value_type;
      typedef _Distance                  difference_type;
      typedef _Tp* pointer;
      typedef _Tp& reference;
  };
  ```

  iii.`Traits`技术（重点）

  ​	a.`iterator_traits`

  ​		a.`iterator_traits`用于萃取出iterator对应类型，例如`value_type`、`iterator_category`等等。

  ​			iterator本身就可以使用对应宏定义(例如`__VALUE_TYPE`)来取出对应类型。但原生指针并没有这   		    些方法，所以需要套一层`iterator_traits`，让`iterator_traits`"帮助"原生指针，将所需的相关 	        信息返回给调用者。

  ​		b.`iterator_traits`相关源码

  ```c++
  template <class _Iterator>  //iterator
  struct iterator_traits {
  typedef typename _Iterator::iterator_category iterator_category;
  typedef typename _Iterator::value_type        value_type;
  typedef typename _Iterator::difference_type   difference_type;
  typedef typename _Iterator::pointer           pointer;
  typedef typename _Iterator::reference         reference;
  };
  
  template <class _Tp>        //原生指针
  struct iterator_traits<_Tp*> {
  typedef random_access_iterator_tag iterator_category;
  typedef _Tp                         value_type;
  typedef ptrdiff_t                   difference_type;
  typedef _Tp*                        pointer;
  typedef _Tp&                        reference;
  };
  ```

  ​	b.`type_traits`

  ​		a.iterator_traits用来萃取迭代器的特性，type_traits用来萃取型别的特性。

  ​			__type_traits相关类型说明：

  - `has_trivial_default_constructor` —— 是否使用默认构造函数
  - `has_trivial_copy_constructor` —— 是否使用默认拷贝构造函数
  - `has_trivial_assignment_operator` —— 是否使用默认赋值运算符
  - `has_trivial_destructor` —— 是否使用默认析构函数
  - `is_POD_type` —— 是否是`POD`类型 返回的是`__true_type`或`__false_type`结构

  ​		c.type_traits相关源码

  ```c++
  struct __true_type {};
  struct __false_type {};
  
  template <class _Tp>
  struct __type_traits {
      typedef __true_type     this_dummy_member_must_be_first;
                      /* Do not remove this member. It informs a compiler which
                          automatically specializes __type_traits that this
                          __type_traits template is special. It just makes sure that
                          things work if an implementation is using a template
                          called __type_traits for something unrelated. */
  
      /* The following restrictions should be observed for the sake of
          compilers which automatically produce type specific specializations
          of this class:
              - You may reorder the members below if you wish
              - You may remove any of the members below if you wish
              - You must not rename members without making the corresponding
                  name change in the compiler
              - Members you add will be treated like regular members unless
                  you add the appropriate support in the compiler. */
  
      typedef __false_type    has_trivial_default_constructor;
      typedef __false_type    has_trivial_copy_constructor;
      typedef __false_type    has_trivial_assignment_operator;
      typedef __false_type    has_trivial_destructor;
      typedef __false_type    is_POD_type;
  };
  
  __STL_TEMPLATE_NULL struct __type_traits<int> {
      typedef __true_type    has_trivial_default_constructor;
      typedef __true_type    has_trivial_copy_constructor;
      typedef __true_type    has_trivial_assignment_operator;
      typedef __true_type    has_trivial_destructor;
      typedef __true_type    is_POD_type;
  };
  ```

  ​	c.应用

  ​		为什么实现多种类型的迭代器，是为了提高STL的效率与速度

  ​		根据迭代器的类型来做出不同且最合适的操作。例如，如果对char*类型的iterator执行copy操作，那	    么copy函数就可以直接使用memcpy来完成操作，而不是遍历复制再构造。如此以提高算法的效率。

  ```c++
  vector<_Tp, _Alloc>::_M_insert_aux(iterator __position, const _Tp& __x)
  {
      /* ....  */
      __STL_TRY{
          // 当vector的成员函数调用 uninitialized_copy 时，程序会根据迭代器类型执行特定的 uninitialized_copy操作
          __new_finish = uninitialized_copy(_M_start, __position, __new_start);
          construct(__new_finish, __x);
          ++__new_finish;
          __new_finish = uninitialized_copy(__position, _M_finish, __new_finish);
      } __STL_UNWIND((destroy(__new_start, __new_finish),
              _M_deallocate(__new_start, __len)));
      /* ....  */
  }
  
  // 如果是char*类型的迭代器就直接调用memmove。如果不是，则调用使用迭代器的uninitialized_copy函数
  template <class _InputIter, class _ForwardIter>
  inline _ForwardIter
  uninitialized_copy(_InputIter __first, _InputIter __last,
      _ForwardIter __result)
  {
      return __uninitialized_copy(__first, __last, __result,
          // 注意到这里获取了迭代器所指向对象的类型
          __VALUE_TYPE(__result));
  }
  
  // 如果迭代器是一个指向某个对象的指针，则调用内含_Tp*类型参数的__uninitialized_copy
  template <class _InputIter, class _ForwardIter, class _Tp>
  inline _ForwardIter
  __uninitialized_copy(_InputIter __first, _InputIter __last,
                      _ForwardIter __result, _Tp*)
  {
      // 注意这里获取了迭代器所指对象的is_POD信息
      typedef typename __type_traits<_Tp>::is_POD_type _Is_POD;
      return __uninitialized_copy_aux(__first, __last, __result, _Is_POD());
  }
  
  // 如果这个对象类型是POD的，直接copy以提高效率
  template <class _InputIter, class _ForwardIter>`
  inline _ForwardIter
  __uninitialized_copy_aux(_InputIter __first, _InputIter __last,
                          _ForwardIter __result,
                          __true_type)
  {
      return copy(__first, __last, __result);
  }
  // 否则，只能一个个的遍历并执行构造函数
  template <class _InputIter, class _ForwardIter>
  _ForwardIter
  __uninitialized_copy_aux(_InputIter __first, _InputIter __last,
                          _ForwardIter __result,
                          __false_type)
  {
      _ForwardIter __cur = __result;
      __STL_TRY {
          for ( ; __first != __last; ++__first, ++__cur)
          _Construct(&*__cur, *__first);
          return __cur;
      }
      __STL_UNWIND(_Destroy(__result, __cur));
  }
  ```

iv.容器

​	a.vector

​		vector的本质就是一个内存空间连续的数组，与普通数组的区别就是他的长度能变化。vector的核心成员		就是三个迭代器。

- `_Tp* _M_start` —— 指向所分配数组的起始位置
- `_Tp* _M_finish` —— 指向已使用空间的末端位置+1
- `_Tp* _M_end_of_storage` —— 指向所分配数组的末尾位置+1





