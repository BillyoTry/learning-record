#pragma once
//Created by c7 on 2020/5/20

#ifndef MYSTL_UNINITIALIZED_H
#define MYSTL_UNINITIALIZED_H

#include "stl_config.h"
#include "stl_iterator_base.h"
#include "type_traits"
#include "stl_construct.h"
#include "stl_algobase.h"

_STL_BEGIN_NAMESPACE


template<class _InputIter, class _ForwardIter>
inline _ForwardIter uninitialized_copy(_InputIter __first, _InputIter __last, _ForwardIter __result) {
	return __uninitialized_copy(__first, __last, __VALUE_TYPE(__result));
}
template<class _InputIter,class _ForwardIter,class _Tp>
inline _ForwardIter uninitialized_copy(_InputIter __first, _InputIter __last, _ForwardIter __result, _Tp*) {
	typedef typaname __type_traits<_Tp>::is_POD_type _Is_POD;
	return __uninitialized_copy_aux(__first, __last, __result, _Is_POD());
}
template<class _InputIter,class _ForwardIter>
inline _ForwardIter __uninitialized_copy_aux(_InputIter __first, _InputIter __last, _ForwardIter __result, __false_type) {
	_ForwardIter __cur = __result;
	try { //可能抛出异常的语句
		for (;__first != __last, ++__first) {
			_Construct(&*__cur, *__first);
		}
		return __cur;
	}
	catch (...) { _Destroy(__result, __cur);throw }  //异常处理语句
}
template<class _InputIter,class _ForwardIter>
inline _ForwardIter __uninitialized_copy_aux(_InputIter __first, _InputIter __last, _ForwardIter __result, __true_type) {
	return copy(__first, __last, __result);
}

template<class _ForwardIter,class _Tp>
inline void uninitialized_fill(_ForwardIter __first, _ForwardIter __last, const _Tp& __x) {
	__uninitialized_fill(__first, __last, __x, __VALUE_TYPE(__first));
}
template<class _ForwardIter,class _Tp,class _Tp1>
inline void __uninitialized_fill(_ForwardIter __first, _ForwardIter __last, const _Tp& __x, _Tp1*) {
	typedef typename __type_traits<_Tp1>::is_POD_type _Is_POD;
	return __uninitialized_fill_aux(__first, __last, __x, _Is_POD());
}
template<class _ForwardIter,class _Tp>
inline void __uninitialized_fill_aux(_ForwardIter __first, _ForwardIter __last, const _Tp& __x, __false_type) {
	_ForwardIter __cur = __first;
	try {
		for (;__cur != __last;++_cur) {
			construct(&*__cur, __x);
		}
	}
	catch (...) {
		destroy(__first, __cur);
	}
}


template<class _ForwardIter,class _Tp>
inline void __uninitialized_fill_aux(_ForwardIter __first, _ForwardIter __last, const _Tp& __x, __true_type) {
	fill(__fist, __last, __x);
}

template<class _ForwardIter,class _Size,class _Tp>
inline _ForwardIter __uninitialized_fill_n_aux(_ForwardIter __first, _ForwardIter __last, _Size _n, const _Tp& __x, __false_type) {
	_ForwardIter __cur = __first;
	try {
		for (;__n > 0;--__n, ++__cur) {
			construct(&*__cur, __x);
		}
		return __cur;
	}
	catch (...) {
		destroy(__first, __cur);
	}
}
template<class _ForwardIter,class _Size,class _Tp>
inline _ForwardIter __uninitialized_fill_n_aux(_ForwardIter __first, _Size __n, const _Tp& __x, __true_type) {
	return fill_n(__first, __n, __x);
}
template<class _ForwardIter,class _Size,class _Tp,class _Tp1>
inline _ForwardIter __uninitialized_fill_n(_ForwardIter __first, _Size __n, const _Tp& __x, _Tp1*) {
	typedef typename __type_traits<_Tp1>::is_POD_type _Is_POD;
	return __uninitialized_fill_n_aux(__first, __n, __x, _Is_POD());
}
template<class _ForwardIter,class _Size,class _Tp>
inline _ForwardIter uninitialized_fill_n(_ForwardIter __first, _Size __n, const _Tp& __x) {
	return __uninitialized_fill_n(__first, __n, __x, __VALUE_TYPE(__first));
}
_STL_END_NAMESPACE
#endif