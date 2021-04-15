#pragma once
//Created by c7 on 2020/5/19

#ifndef MYSTL_CONSTRUCT_H
#define MYSTL_CONSTRUCT_H

#include <new>
#include "stl_config.h"
#include "type_traits.h"

_STL_BEGIN_NAMESPACE


//构造函数和析构函数

    template<class _T1, class _T2>
    inline void _Construct(_T1* __p, const _T2& __value) {
        new((void*)__p) _T1(__value);
    }

    template<class _T1>
    inline void _Construct(_T1* __p) {
        new((void*)__p) _T1();
    }

    template<class _Tp>
    void destroy_one(_Tp*, __true_type) {}

    template<class _Tp>
    void destroy_one(_Tp* pointer, __false_type) {
        if (pointer != nullptr) {
            pointer->~Ty();
        }
    }

    template<class _Tp>
    inline void _Destroy(_Tp* __pointer) {
        destroy_one(__pointer, typename __type_traits<_Tp>::has_trivial_destructor());
        //        __pointer->~Tp();
    }

    template<class _ForwardIterator>
    inline void __destroy_aux(_ForwardIterator __first, _ForwardIterator __last, __false_type) {
        for(;__first!=__last,++__first)
            _Destroy(&*__first)//指针解引用后就是值
    }

    template<class _ForwardIterator>
    inline void _destroy_aux(_ForwardIterator,_ForwardIterator,__true_type){}

    template<class _ForwardIterator,class _Tp>
    inline void __destroy(_ForwardIterator __first, _ForwardIterator __last, _Tp*) {
        typedef typename __type_traits<_Tp>::has_trivial_destructor _Trivial_destructor;
        __destroy_aux(__first, __last, _Trivial_destructor());
    }

    template<class _ForwardIterator>
    inline void _Destroy(_ForwardIterator __first, _ForwardIterator __last) {
        __destroy(__first, __last, __VALUE_TYPE(_first));
    }

    inline void _Destroy(char*, char*) {}

    inline void _Destroy(int*, int*) {}

    inline void _Destroy(float*, float*) {}

    inline void _Destroy(long*, long*) {}

    inline void _Destroy(double*, double*) {}

    template<class _T1,class _T2>
    inline void construct(_T1* __p, const _T2* __value) {
        _Construct(_p, __value);
    }

    template<class _T1>
    inline void construct(_T1* _p) {
        _Construct(_p);
    }

    template<class _Tp>
    inline void destroy(_Tp* _pointer) {
        _Destroy(_pointer);
    }

    template<class _ForwardIterator>
    inline void destroy(_ForwardIterator __first, _ForwardIterator __last) {
        _Destroy(__first, __last);
    }

_STL_END_NAMESPACE
#endif