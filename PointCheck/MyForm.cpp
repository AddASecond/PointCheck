#include "MyForm.h"

using namespace PointCheck;//ʹ��MyForm.h�ж���������ռ�

[STAThread]//���߳����ԣ���Щ���Ҫ���߳�

int main(array<System::String^>^args) {
	Application::EnableVisualStyles();//���ÿ��ӻ�����
	Application::Run(gcnew MyForm());//����һ�����壬��������Ϣѭ��
	return 0;
}