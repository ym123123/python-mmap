python tornado 调用 linux c 共享内存池
1：linux c 基于mmap bitmap 的一个简易内存池, 通过修改align 可改变对齐字节数
2：tornado 处理IO请求
3：模拟一个输入流， 多个输出流的场景， 如果内存用完， 则强制删除空闲内存， 如果仍然没有内存， 则必须抛弃部分数据

