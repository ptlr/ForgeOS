# <h1 align="center">Debug方法总结</h1>

开发一个操作系统意味要从零开始，需要自己写编程语言的库。需要自己加载loader、kernel到内存。

这意味着开发者自己写的程序，可能存在某种未知的BUG，遇到问题需要一步一步排查，为了解决这些问题，归纳总结了一些经验，如果代码达不到想要的效果，可以按以下步骤检查：

### 1、检查编译出来的二进制文件

这个步骤保证代码中没有明显的错误。

### 2、检查二进制文件是否正确写入镜像

每次添加功能，内核的二进制文件的大小都会改变，需要保证需要的二进制文件都被正确写入。

### 3、检查二进制文件是否正确加载到内存中

如果二进制大小发生了改变，除了需要正确写入镜像之外，还需要保证正确加载到内存中。

此时，出现问题的原因就很多了：

* 2中的写入不正确

* 加载代码有误（代码BUG）

### 4、跳转和调用是否正常

由于添加了新的代码，可能会在无意中破坏了代码的调用逻辑、栈空间、忘记重新初始化段寄存器、甚至需要调整偏移数据。

### 5、对比运行时结果和二进制（ELF或其他可以读取的格式）

编译时没有问题，加载没有问题，甚至一些前置代码运行正确，但代码无法达到自己想要的结果。

这种对比可以看出代码中隐藏的难以发现的问题,一般是代码BUG，此时可能伴随着第6、7部中提到的原因。

### 6、RFM:如果前6步都没有问题了，请阅读用到的工具手册，

还解决不了问题，问题可能出在工具的使用方法上，阅读相关的说明书，可能会解决大部分问题。

到这里，需要同时关注自己的代码。

### 7、学到的知识不全或知识错误

到这一步出大问题了，如果代码在某种巧合下可用，但一定存在BUG,最难排查，对此，提出如下建议：

* 重新阅读书籍

* 查找对比资料

* 与朋友讨论

* 重构代码

举例：软盘的CHS读取
