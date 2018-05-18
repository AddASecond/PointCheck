#include "MyForm.h"
#include "ver_vas.h"

using namespace PointCheck;//使用MyForm.h中定义的命名空间

[STAThread]//单线程特性，有些组件要求单线程

int main(array<System::String^>^args) 
{
	if (!VerifyAuthorityVasTest(2018,4,24,730))
	{
		printf("Please get your license............\n");
		return 1;
	}
	Application::EnableVisualStyles();//启用可视化界面
	Application::Run(gcnew MyForm());//创建一个窗体，并进入消息循环
	return 0;
}