
1.linux�±��룺
	�迪��arch_interface.c����ĺ궨��RUN_ON_PC��ִ��make�����
	
2. Ƕ��ʽƽ̨�ϱ���
	�ر�RUN_ON_PC���ҰѴ�����ֲ��Ƕ��ʽ�Ĺ��̣�һ����bootloader��,�빤��һ�����


3. porting
3.1�����ã��ǳ���Ҫ����������Ҫ������ص�flash��ַ�ʹ�С����ϸ�ο�arch_interface.h����Ķ��壬ͬʱ��Ҫ��sm_flash.h�����ù̼���ŵĵ�ַ����Ϣ
3.2����������Ҫ�Լ�ʵ��arch_write_flash, arch_read_flash, arch_erase_flash, �Լ�sm_system.c��sm_platform.c�е���ؽӿں�����
3.3����������Ҫ�Լ�ѡ���ƽ̨ʵ��sm_socket_recv_api��sm_socket_recv_api��socket�ӿ�
3.4�������ߵ���firmware_patch����ʵ���ڱ���patch
3.5����������Ҫ�Լ�ʵ�ֹ̼����أ���֤

4. ����
4.1��ͨ��https�������������ز�ֹ̼���
4.2����������Ժ���֤�̼������Ժ���ȷ��
4.3������һ��������־����������bootloader
4.4����bootloader������������־������У������firmware_patch������ʼ����
4.5����������Ժ�Ӧ���������־����ֹ�´������


5. ��linux�����ϲ��ԣ�
./bsdiff oldfile newfile patch
./bspatch oldfile newfile patch

6. ����
	�رյ��Ժ󣬱���Ĵ���Ӧ�ò�����10KB��RAM����<64KB