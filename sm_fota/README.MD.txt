
1.linux下编译：
	需开启arch_interface.c里面的宏定义RUN_ON_PC，执行make命令即可
	
2. 嵌入式平台上编译
	关闭RUN_ON_PC并且把代码移植到嵌入式的工程（一般是bootloader）,与工程一起编译


3. porting
3.1　配置，非常重要，开发者需要配置相关的flash地址和大小，详细参考arch_interface.h里面的定义，同时需要在sm_flash.h中配置固件存放的地址等信息
3.2　开发者需要自己实现arch_write_flash, arch_read_flash, arch_erase_flash, 以及sm_system.c跟sm_platform.c中的相关接口函数；
3.3　开发者需要自己选择的平台实现sm_socket_recv_api，sm_socket_recv_api等socket接口
3.4　开发者调用firmware_patch函数实现在本地patch
3.5　开发者需要自己实现固件下载／验证

4. 流程
4.1　通过https（或其他）下载差分固件；
4.2　下载完成以后验证固件完整性和正确性
4.3　设置一个升级标志，重启进入bootloader
4.4　在bootloader里面检查升级标志，如果有，则调用firmware_patch函数开始升级
4.5　升级完成以后，应清除升级标志，防止下次误进入


5. 在linux机器上测试：
./bsdiff oldfile newfile patch
./bspatch oldfile newfile patch

6. 其他
	关闭调试后，编译的代码应该不超过10KB，RAM需求<64KB