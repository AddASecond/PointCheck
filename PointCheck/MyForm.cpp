#include "MyForm.h"
#include "ver_vas.h"

using namespace PointCheck;//ʹ��MyForm.h�ж���������ռ�

[STAThread]//���߳����ԣ���Щ���Ҫ���߳�

int main(array<System::String^>^args) 
{
	if (!VerifyAuthorityVasTest(2018,4,24,730))
	{
		printf("Please get your license............\n");
		return 1;
	}
	Application::EnableVisualStyles();//���ÿ��ӻ�����
	Application::Run(gcnew MyForm());//����һ�����壬��������Ϣѭ��
	return 0;
}