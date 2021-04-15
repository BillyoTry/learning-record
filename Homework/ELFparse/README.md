效果如下

```csharp
linux> ./parse hello

function1:	deregister_tm_clones
function2:	register_tm_clones
function3:	__do_global_dtors_aux
function4:	frame_dummy
function5:	__libc_csu_fini
function6:	puts@@GLIBC_2.2.5
function7:	write@@GLIBC_2.2.5
function8:	_fini
function9:	printf@@GLIBC_2.2.5
function10:	__libc_start_main@@GLIBC_2.2.5
function11:	__libc_csu_init
function12:	_start
function13:	main
function14:	_init

There are 14 functions in total
```

值得注意的是并没有按照函数的调用顺序来输出。 