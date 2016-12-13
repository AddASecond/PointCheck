#include<opencv\cv.h>
#include<opencv\highgui.h>
#include<string>

#pragma once

namespace PointCheck {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::IO;

	//必须写在托管类外面，方便调用cvRelease释放图片
	IplImage* img; //openCV image 
	IplImage* img_output; //输出时用的图

	/// <summary>
	/// Summary for MyForm
	/// </summary>
	public ref class MyForm : public System::Windows::Forms::Form
	{
#pragma region Variables and Functions
	public:
		MyForm(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}



		// Variables and Functions Defined by chenqu 20161126 START 
		// add my vars here 2016
		int PicCount = 0;	//total number of pictures
		int SelectedRow = 0;	//current index of picture, usually SelectedRow = PicCount - 1
		int ChangeCount = 0;// to save how many pictures are changed
		int L;	//	To save Length of Text
		int ResizeOrNot = 0; //to indicate whether MyForm is resized or not
		int tmpLocX;
		int tmpLocY;
		int OrderIndicate = 0;	// 用于初次加载listview1时提示是否有顺序错误
		String^ DefaultColor = "蓝";	// 默认蓝，0,1,2,3,4对应黑，白，蓝，黄，绿色车牌
		String^ DefaultType = "单层";	// 默认单层，0,1,2对应单层、双层和假（虚警）车牌
		String^ ColorList = "未知,白,黑,蓝,黄";
		String^ TypeList = "单层,双层,假(虚警)";
		System::String^ directory = "";// directory to save the tmpLogfile to prevent 
		//CvMemStorage* storage = 0;

		array<int, 2>^ LPLoc = gcnew array<int, 2>(200000, 10); //to save license plate location
		array<int^, 2>^ LPLocBox = gcnew array<int^, 2>(200000, 10); //to save license plate location on picturebox
		array<int>^ LocNum = gcnew array <int>(200000);//to memory the number of painted points in each image
	private: System::Windows::Forms::GroupBox^  groupBox7;
	public:
		array<int>^ ChangedOrNot = gcnew array<int>(200000);//记录这次挑图那些图更改了，更改为1；

		//add my functions here 2016
		//function1: Copy the content of System::String to string
		std::string& MarshalString(String ^ s, std::string& os) {
			using namespace Runtime::InteropServices;
			const char* chars =
				(const char*)(Marshal::StringToHGlobalAnsi(s)).ToPointer();
			os = chars;
			Marshal::FreeHGlobal(IntPtr((void*)chars));
			return os;
		}

		///function2: 删除指定文件夹下指定文件名的文件  
		/// </summary>  
		/// <param name="url">文件夹地址</param>  
		/// <param name="name">要删除的文件名</param>--自带去除扩展名  
		/// <returns></returns>  
		bool delLicenseFiles(String^ url, String^ name) {
			try {
				DirectoryInfo^ Folder = gcnew DirectoryInfo(url);
				for each(FileInfo^ file in Folder->GetFiles()) {
					if (name == file->Name->Substring(0, 7)) {
						file->Delete();
					}
				}
				//Folder->Delete();
				return true;
			}
			catch (Exception^ e) {
				return false;
			}
		}

		//function3: Check the order of points
		//input String of keypoints and return the preflight check results
		String^ PreflightCheck(String^ Text,int Length_Text) { 
			array<int>^ TJPoint = gcnew array<int>(10);	//save the points into RAM
			String^ StrPrePoints = Text->Split('\\')[Length_Text - 1]->Split('[')[1]->Split(']')[0];
			for (int i = 0; i < 5; i++) {
				TJPoint[2 * i] = int::Parse(StrPrePoints->Split('.')[i]->Split(',')[0]->Split('(')[1]);
				TJPoint[2 * i + 1] = int::Parse(StrPrePoints->Split('.')[i]->Split(',')[1]->Split(')')[0]);
			}
			//Preflight check if 
			//axis x:1>4>0	2>4>3 axis y: 3>4>0 2>4>1
			if (TJPoint[2] > TJPoint[8] && TJPoint[8] > TJPoint[0] &&
				TJPoint[4] > TJPoint[8] && TJPoint[8] > TJPoint[6] &&
				TJPoint[7] > TJPoint[9] && TJPoint[9] > TJPoint[1] &&
				TJPoint[5] > TJPoint[9] && TJPoint[9] > TJPoint[3]) {
				return "对";
			}
			else {
				return "错错错错错错";
			}
		}

		//function4: Save Points Location from filename to LocNum, LPLoc,LPLocBox
		int LocSave(String^ Text, int Length_Text, IplImage* img) {
			String^ StrPrePoints = Text->Split('\\')[Length_Text - 1]->Split('[')[1]->Split(']')[0];
			//Prior knowledge: all 5 points exist in filename
			LocNum[SelectedRow] = 5;
			for (int i = 0; i < 5; i++) {

				// Save Points into LPLoc
				LPLoc->SetValue(int::Parse(StrPrePoints->Split('.')[i]->Split(',')[0]->Split('(')[1]), SelectedRow, 2 * i);
				LPLoc->SetValue(int::Parse(StrPrePoints->Split('.')[i]->Split(',')[1]->Split(')')[0]), SelectedRow, 2 * i + 1);

				// Save Points into LPLocBox
				// Compute the resize factor between LPLoc and LPLocBox
				double wfactor = (double)img->width / pictureBox1->ClientSize.Width;
				double hfactor = (double)img->height / pictureBox1->ClientSize.Height;
				double resizeFactor = 1 / Math::Max(wfactor, hfactor);
				double wdiff = (pictureBox1->ClientSize.Width - (double)img->width*resizeFactor) / 2;
				double hdiff = (pictureBox1->ClientSize.Height - (double)img->height*resizeFactor) / 2;

				LPLocBox->SetValue((int)(int::Parse(StrPrePoints->Split('.')[i]->Split(',')[0]->Split('(')[1])*resizeFactor + wdiff), SelectedRow, 2 * i);
				LPLocBox->SetValue((int)(int::Parse(StrPrePoints->Split('.')[i]->Split(',')[1]->Split(')')[0])*resizeFactor + hdiff), SelectedRow, 2 * i+1);
			}
			return 0;
		}

		// function5: Output modified items into D:\PointCheck+护眼版_Log\history.log
		int OutHistory() {
			// 若改动过且现在所有点都点满
			if (ChangedOrNot[SelectedRow] == 1 && LocNum[SelectedRow] == 5) {
				//找到最后一个ChangedOrNot的
				int LCInd = 199999;	//LastChangedIndex
				while (ChangedOrNot[LCInd] == 0 && LCInd > 0) {
					LCInd--;
				}

				//刷新history.log
				FileStream^ history = gcnew FileStream("D:\\PointCheck+护眼版_Log\\history.log", FileMode::Create, FileAccess::ReadWrite);
				StreamWriter^ tmphistory = gcnew StreamWriter(history, System::Text::Encoding::Unicode);
				ChangeCount = 0; // reset ChangeCount
				for (int i = 0; i <= LCInd; i++) {
					if (ChangedOrNot[i] == 1) {
						ChangeCount += 1;
						String^ OneLine = "";
						OneLine += listView1->Items[i]->SubItems[0]->Text->ToString() + " " +     	//图片序号
							listView1->Items[i]->SubItems[7]->Text->ToString() + " " + 				//原图片名称
							listView1->Items[i]->SubItems[5]->Text->ToString() + " " +	            //图片新关键点位置
							listView1->Items[i]->SubItems[8]->Text->ToString() + " " +              //车牌号码
							listView1->Items[i]->SubItems[9]->Text->ToString() + " " +              //图片颜色
							listView1->Items[i]->SubItems[10]->Text->ToString();                     //图片类型
						tmphistory->WriteLine(OneLine);

						// change the current path in listview1
						String^ OldPath_item = listView1->Items[i]->SubItems[6]->Text;
						String^ NewPath_item = OldPath_item->Split('[')[0] + "[" + listView1->Items[i]->SubItems[5]->Text + "]" + OldPath_item->Split(']')[1];
						listView1->Items[i]->SubItems[6]->Text = NewPath_item;

						// refresh the PreflightCheck in listview and richTextBox5
						array<int>^ TJPoint = gcnew array<int>(10);	//save the points into RAM
						String^ StrPrePoints = listView1->Items[i]->SubItems[5]->Text;
						for (int i = 0; i < 5; i++) {
							TJPoint[2 * i] = int::Parse(StrPrePoints->Split('.')[i]->Split(',')[0]->Split('(')[1]);
							TJPoint[2 * i + 1] = int::Parse(StrPrePoints->Split('.')[i]->Split(',')[1]->Split(')')[0]);
						}
						//Preflight check if 
						//axis x:1>4>0	2>4>3 axis y: 3>4>0 2>4>1
						if (TJPoint[2] > TJPoint[8] && TJPoint[8] > TJPoint[0] &&
							TJPoint[4] > TJPoint[8] && TJPoint[8] > TJPoint[6] &&
							TJPoint[7] > TJPoint[9] && TJPoint[9] > TJPoint[1] &&
							TJPoint[5] > TJPoint[9] && TJPoint[9] > TJPoint[3]) {
							listView1->Items[i]->SubItems[4]->Text = "对";
							richTextBox5->Text = "对";
						}
						else {
							listView1->Items[i]->SubItems[4]->Text = "错错错错错错";
							richTextBox5->Text = "错";
						}
					}
				}
				tmphistory->Close();
				history->Close();
				richTextBox4->Text = ChangeCount.ToString();	//refresh the ChangeCount in rechTextBox4

																// MessageBox mention richTextBox5 content
				if (richTextBox5->Text == "错") {
					MessageBox::Show("本张 标点顺序错误 或 中心点偏出了边框！");
				}
			}	// end of modified before save
			return 0;
		}

		// function 6: Color,Type Convert to Index
		int ColorTypeConvert(String^ ColorOrType) {
			int NumOut = 0;
			for (int i = 0; i < 5; i++) {
				if (ColorList->Split(',')[i] == ColorOrType) {
					NumOut = i;
				}
			}
			for (int i = 0; i < 3; i++) {
				if (TypeList->Split(',')[i] == ColorOrType) {
					NumOut = i;
				}
			}
			return NumOut;
		}

		// Variables and Functions Defined by chenqu 20161126 END


	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~MyForm()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::Label^  label9;
	private: System::Windows::Forms::RichTextBox^  richTextBox9;
	private: System::Windows::Forms::GroupBox^  groupBox6;
	private: System::Windows::Forms::RichTextBox^  richTextBox10;
	private: System::Windows::Forms::ColumnHeader^  columnHeader11;
	private: System::Windows::Forms::Label^  label6;
	private: System::Windows::Forms::RichTextBox^  richTextBox8;
	private: System::Windows::Forms::Label^  label5;
	private: System::Windows::Forms::RichTextBox^  richTextBox7;
	private: System::Windows::Forms::GroupBox^  groupBox5;
	private: System::Windows::Forms::Button^  button16;
	private: System::Windows::Forms::Button^  button15;
	private: System::Windows::Forms::Button^  button14;
	private: System::Windows::Forms::Button^  button13;
	private: System::Windows::Forms::Button^  button12;
	private: System::Windows::Forms::Button^  button11;
	private: System::Windows::Forms::Button^  button10;
	private: System::Windows::Forms::Button^  button9;
	private: System::Windows::Forms::Button^  button8;
	private: System::Windows::Forms::Label^  label7;
	private: System::Windows::Forms::Label^  label8;
	private: System::Windows::Forms::ColumnHeader^  columnHeader9;
	private: System::Windows::Forms::ColumnHeader^  columnHeader10;
	private: System::Windows::Forms::RichTextBox^  richTextBox6;
	private: System::Windows::Forms::ColumnHeader^  columnHeader8;
	private: System::Windows::Forms::OpenFileDialog^  openFileDialog1;
	private: System::Windows::Forms::StatusStrip^  statusStrip1;
	protected:
	private: System::Windows::Forms::TabControl^  tabControl1;
	private: System::Windows::Forms::TabPage^  tabPage1;
	private: System::Windows::Forms::TabPage^  tabPage2;
	private: System::Windows::Forms::GroupBox^  groupBox2;
	private: System::Windows::Forms::Label^  label2;
	private: System::Windows::Forms::RichTextBox^  richTextBox2;
	private: System::Windows::Forms::Label^  label1;
	private: System::Windows::Forms::RichTextBox^  richTextBox1;
	private: System::Windows::Forms::Button^  button2;
	private: System::Windows::Forms::Button^  button1;
	private: System::Windows::Forms::GroupBox^  groupBox3;
	private: System::Windows::Forms::Button^  button5;
	private: System::Windows::Forms::Button^  button3;
	private: System::Windows::Forms::RichTextBox^  richTextBox3;
	private: System::Windows::Forms::Button^  button4;
	private: System::Windows::Forms::GroupBox^  groupBox1;
	private: System::Windows::Forms::Button^  button7;
	private: System::Windows::Forms::GroupBox^  groupBox4;
	private: System::Windows::Forms::Button^  button6;
	private: System::Windows::Forms::TabPage^  tabPage3;
	private: System::Windows::Forms::Label^  label3;
	private: System::Windows::Forms::RichTextBox^  richTextBox4;
	private: System::Windows::Forms::Label^  label4;
	private: System::Windows::Forms::RichTextBox^  richTextBox5;
	private: System::Windows::Forms::PictureBox^  pictureBox1;
	private: System::Windows::Forms::ListView^  listView1;
	private: System::Windows::Forms::ColumnHeader^  columnHeader1;
	private: System::Windows::Forms::ColumnHeader^  columnHeader2;
	private: System::Windows::Forms::ColumnHeader^  columnHeader3;
	private: System::Windows::Forms::ColumnHeader^  columnHeader4;
	private: System::Windows::Forms::ColumnHeader^  columnHeader5;
	private: System::Windows::Forms::ColumnHeader^  columnHeader6;
	private: System::Windows::Forms::ColumnHeader^  columnHeader7;

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;
#pragma endregion



#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(MyForm::typeid));
			this->statusStrip1 = (gcnew System::Windows::Forms::StatusStrip());
			this->tabControl1 = (gcnew System::Windows::Forms::TabControl());
			this->tabPage1 = (gcnew System::Windows::Forms::TabPage());
			this->pictureBox1 = (gcnew System::Windows::Forms::PictureBox());
			this->tabPage2 = (gcnew System::Windows::Forms::TabPage());
			this->listView1 = (gcnew System::Windows::Forms::ListView());
			this->columnHeader1 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader2 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader3 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader4 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader5 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader6 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader7 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader8 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader9 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader10 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader11 = (gcnew System::Windows::Forms::ColumnHeader());
			this->tabPage3 = (gcnew System::Windows::Forms::TabPage());
			this->richTextBox6 = (gcnew System::Windows::Forms::RichTextBox());
			this->groupBox2 = (gcnew System::Windows::Forms::GroupBox());
			this->label4 = (gcnew System::Windows::Forms::Label());
			this->label9 = (gcnew System::Windows::Forms::Label());
			this->richTextBox5 = (gcnew System::Windows::Forms::RichTextBox());
			this->richTextBox9 = (gcnew System::Windows::Forms::RichTextBox());
			this->label6 = (gcnew System::Windows::Forms::Label());
			this->richTextBox8 = (gcnew System::Windows::Forms::RichTextBox());
			this->label5 = (gcnew System::Windows::Forms::Label());
			this->richTextBox7 = (gcnew System::Windows::Forms::RichTextBox());
			this->label3 = (gcnew System::Windows::Forms::Label());
			this->richTextBox4 = (gcnew System::Windows::Forms::RichTextBox());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->richTextBox2 = (gcnew System::Windows::Forms::RichTextBox());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->richTextBox1 = (gcnew System::Windows::Forms::RichTextBox());
			this->button2 = (gcnew System::Windows::Forms::Button());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->groupBox3 = (gcnew System::Windows::Forms::GroupBox());
			this->button5 = (gcnew System::Windows::Forms::Button());
			this->button3 = (gcnew System::Windows::Forms::Button());
			this->richTextBox3 = (gcnew System::Windows::Forms::RichTextBox());
			this->button4 = (gcnew System::Windows::Forms::Button());
			this->groupBox1 = (gcnew System::Windows::Forms::GroupBox());
			this->button7 = (gcnew System::Windows::Forms::Button());
			this->groupBox4 = (gcnew System::Windows::Forms::GroupBox());
			this->button6 = (gcnew System::Windows::Forms::Button());
			this->openFileDialog1 = (gcnew System::Windows::Forms::OpenFileDialog());
			this->groupBox5 = (gcnew System::Windows::Forms::GroupBox());
			this->label8 = (gcnew System::Windows::Forms::Label());
			this->label7 = (gcnew System::Windows::Forms::Label());
			this->button16 = (gcnew System::Windows::Forms::Button());
			this->button15 = (gcnew System::Windows::Forms::Button());
			this->button14 = (gcnew System::Windows::Forms::Button());
			this->button13 = (gcnew System::Windows::Forms::Button());
			this->button12 = (gcnew System::Windows::Forms::Button());
			this->button11 = (gcnew System::Windows::Forms::Button());
			this->button10 = (gcnew System::Windows::Forms::Button());
			this->button9 = (gcnew System::Windows::Forms::Button());
			this->button8 = (gcnew System::Windows::Forms::Button());
			this->groupBox6 = (gcnew System::Windows::Forms::GroupBox());
			this->richTextBox10 = (gcnew System::Windows::Forms::RichTextBox());
			this->groupBox7 = (gcnew System::Windows::Forms::GroupBox());
			this->tabControl1->SuspendLayout();
			this->tabPage1->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox1))->BeginInit();
			this->tabPage2->SuspendLayout();
			this->tabPage3->SuspendLayout();
			this->groupBox2->SuspendLayout();
			this->groupBox3->SuspendLayout();
			this->groupBox1->SuspendLayout();
			this->groupBox4->SuspendLayout();
			this->groupBox5->SuspendLayout();
			this->groupBox6->SuspendLayout();
			this->groupBox7->SuspendLayout();
			this->SuspendLayout();
			// 
			// statusStrip1
			// 
			this->statusStrip1->Location = System::Drawing::Point(0, 916);
			this->statusStrip1->Name = L"statusStrip1";
			this->statusStrip1->Size = System::Drawing::Size(1194, 22);
			this->statusStrip1->TabIndex = 0;
			this->statusStrip1->Text = L"statusStrip1";
			// 
			// tabControl1
			// 
			this->tabControl1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->tabControl1->Controls->Add(this->tabPage1);
			this->tabControl1->Controls->Add(this->tabPage2);
			this->tabControl1->Controls->Add(this->tabPage3);
			this->tabControl1->Location = System::Drawing::Point(12, 66);
			this->tabControl1->Name = L"tabControl1";
			this->tabControl1->SelectedIndex = 0;
			this->tabControl1->Size = System::Drawing::Size(950, 847);
			this->tabControl1->TabIndex = 1;
			// 
			// tabPage1
			// 
			this->tabPage1->Controls->Add(this->pictureBox1);
			this->tabPage1->Location = System::Drawing::Point(4, 22);
			this->tabPage1->Name = L"tabPage1";
			this->tabPage1->Padding = System::Windows::Forms::Padding(3);
			this->tabPage1->Size = System::Drawing::Size(942, 821);
			this->tabPage1->TabIndex = 0;
			this->tabPage1->Text = L"车辆截图和标点";
			this->tabPage1->UseVisualStyleBackColor = true;
			// 
			// pictureBox1
			// 
			this->pictureBox1->BackColor = System::Drawing::Color::LightGreen;
			this->pictureBox1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->pictureBox1->Location = System::Drawing::Point(3, 3);
			this->pictureBox1->Name = L"pictureBox1";
			this->pictureBox1->Size = System::Drawing::Size(936, 815);
			this->pictureBox1->SizeMode = System::Windows::Forms::PictureBoxSizeMode::Zoom;
			this->pictureBox1->TabIndex = 0;
			this->pictureBox1->TabStop = false;
			this->pictureBox1->Paint += gcnew System::Windows::Forms::PaintEventHandler(this, &MyForm::pictureBox1_Paint);
			this->pictureBox1->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::pictureBox1_MouseDown);
			// 
			// tabPage2
			// 
			this->tabPage2->Controls->Add(this->listView1);
			this->tabPage2->Location = System::Drawing::Point(4, 22);
			this->tabPage2->Name = L"tabPage2";
			this->tabPage2->Padding = System::Windows::Forms::Padding(3);
			this->tabPage2->Size = System::Drawing::Size(906, 821);
			this->tabPage2->TabIndex = 1;
			this->tabPage2->Text = L"图片详情列表";
			this->tabPage2->UseVisualStyleBackColor = true;
			// 
			// listView1
			// 
			this->listView1->BackColor = System::Drawing::Color::LightGreen;
			this->listView1->Columns->AddRange(gcnew cli::array< System::Windows::Forms::ColumnHeader^  >(11) {
				this->columnHeader1, this->columnHeader2,
					this->columnHeader3, this->columnHeader4, this->columnHeader5, this->columnHeader6, this->columnHeader7, this->columnHeader8,
					this->columnHeader9, this->columnHeader10, this->columnHeader11
			});
			this->listView1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->listView1->Location = System::Drawing::Point(3, 3);
			this->listView1->Name = L"listView1";
			this->listView1->Size = System::Drawing::Size(900, 815);
			this->listView1->TabIndex = 0;
			this->listView1->UseCompatibleStateImageBehavior = false;
			this->listView1->View = System::Windows::Forms::View::Details;
			// 
			// columnHeader1
			// 
			this->columnHeader1->Text = L"帧序号";
			// 
			// columnHeader2
			// 
			this->columnHeader2->Text = L"图片名称（修改前）";
			this->columnHeader2->Width = 420;
			// 
			// columnHeader3
			// 
			this->columnHeader3->Text = L"图片类型";
			this->columnHeader3->Width = 120;
			// 
			// columnHeader4
			// 
			this->columnHeader4->Text = L"图片大小";
			this->columnHeader4->Width = 120;
			// 
			// columnHeader5
			// 
			this->columnHeader5->Text = L"顺序预检结果";
			this->columnHeader5->Width = 120;
			// 
			// columnHeader6
			// 
			this->columnHeader6->Text = L"关键点位置";
			this->columnHeader6->Width = 360;
			// 
			// columnHeader7
			// 
			this->columnHeader7->Text = L"文件路径（当前）";
			this->columnHeader7->Width = 720;
			// 
			// columnHeader8
			// 
			this->columnHeader8->Text = L"文件路径（修改前）";
			this->columnHeader8->Width = 720;
			// 
			// columnHeader9
			// 
			this->columnHeader9->Text = L"车牌号码";
			this->columnHeader9->Width = 120;
			// 
			// columnHeader10
			// 
			this->columnHeader10->Text = L"车牌颜色";
			this->columnHeader10->Width = 120;
			// 
			// columnHeader11
			// 
			this->columnHeader11->Text = L"车牌类型";
			// 
			// tabPage3
			// 
			this->tabPage3->Controls->Add(this->richTextBox6);
			this->tabPage3->Location = System::Drawing::Point(4, 22);
			this->tabPage3->Name = L"tabPage3";
			this->tabPage3->Size = System::Drawing::Size(906, 821);
			this->tabPage3->TabIndex = 2;
			this->tabPage3->Text = L"使用帮助";
			this->tabPage3->UseVisualStyleBackColor = true;
			// 
			// richTextBox6
			// 
			this->richTextBox6->BackColor = System::Drawing::Color::LightGreen;
			this->richTextBox6->Dock = System::Windows::Forms::DockStyle::Fill;
			this->richTextBox6->Location = System::Drawing::Point(0, 0);
			this->richTextBox6->Name = L"richTextBox6";
			this->richTextBox6->Size = System::Drawing::Size(906, 821);
			this->richTextBox6->TabIndex = 0;
			this->richTextBox6->Text = resources->GetString(L"richTextBox6.Text");
			// 
			// groupBox2
			// 
			this->groupBox2->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->groupBox2->Controls->Add(this->label4);
			this->groupBox2->Controls->Add(this->label9);
			this->groupBox2->Controls->Add(this->richTextBox5);
			this->groupBox2->Controls->Add(this->richTextBox9);
			this->groupBox2->Controls->Add(this->label6);
			this->groupBox2->Controls->Add(this->richTextBox8);
			this->groupBox2->Controls->Add(this->label5);
			this->groupBox2->Controls->Add(this->richTextBox7);
			this->groupBox2->Controls->Add(this->label3);
			this->groupBox2->Controls->Add(this->richTextBox4);
			this->groupBox2->Controls->Add(this->label2);
			this->groupBox2->Controls->Add(this->richTextBox2);
			this->groupBox2->Controls->Add(this->label1);
			this->groupBox2->Controls->Add(this->richTextBox1);
			this->groupBox2->Location = System::Drawing::Point(970, 12);
			this->groupBox2->Margin = System::Windows::Forms::Padding(2);
			this->groupBox2->Name = L"groupBox2";
			this->groupBox2->Padding = System::Windows::Forms::Padding(2);
			this->groupBox2->Size = System::Drawing::Size(213, 245);
			this->groupBox2->TabIndex = 2;
			this->groupBox2->TabStop = false;
			this->groupBox2->Text = L"状态";
			// 
			// label4
			// 
			this->label4->AutoSize = true;
			this->label4->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->label4->Location = System::Drawing::Point(-3, 18);
			this->label4->Margin = System::Windows::Forms::Padding(2, 0, 2, 0);
			this->label4->Name = L"label4";
			this->label4->Size = System::Drawing::Size(73, 40);
			this->label4->TabIndex = 11;
			this->label4->Text = L"本张标点\r\n顺序预检";
			// 
			// label9
			// 
			this->label9->AutoSize = true;
			this->label9->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->label9->Location = System::Drawing::Point(4, 213);
			this->label9->Margin = System::Windows::Forms::Padding(2, 0, 2, 0);
			this->label9->Name = L"label9";
			this->label9->Size = System::Drawing::Size(73, 20);
			this->label9->TabIndex = 15;
			this->label9->Text = L"车牌号码";
			// 
			// richTextBox5
			// 
			this->richTextBox5->Enabled = false;
			this->richTextBox5->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 13.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->richTextBox5->Location = System::Drawing::Point(90, 29);
			this->richTextBox5->Margin = System::Windows::Forms::Padding(2);
			this->richTextBox5->Multiline = false;
			this->richTextBox5->Name = L"richTextBox5";
			this->richTextBox5->Size = System::Drawing::Size(108, 26);
			this->richTextBox5->TabIndex = 10;
			this->richTextBox5->Text = L"";
			// 
			// richTextBox9
			// 
			this->richTextBox9->Enabled = false;
			this->richTextBox9->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 13.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->richTextBox9->Location = System::Drawing::Point(90, 208);
			this->richTextBox9->Margin = System::Windows::Forms::Padding(2);
			this->richTextBox9->Multiline = false;
			this->richTextBox9->Name = L"richTextBox9";
			this->richTextBox9->Size = System::Drawing::Size(108, 26);
			this->richTextBox9->TabIndex = 14;
			this->richTextBox9->Text = L"";
			// 
			// label6
			// 
			this->label6->AutoSize = true;
			this->label6->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->label6->Location = System::Drawing::Point(4, 183);
			this->label6->Margin = System::Windows::Forms::Padding(2, 0, 2, 0);
			this->label6->Name = L"label6";
			this->label6->Size = System::Drawing::Size(73, 20);
			this->label6->TabIndex = 13;
			this->label6->Text = L"车牌类型";
			// 
			// richTextBox8
			// 
			this->richTextBox8->Enabled = false;
			this->richTextBox8->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 13.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->richTextBox8->Location = System::Drawing::Point(90, 178);
			this->richTextBox8->Margin = System::Windows::Forms::Padding(2);
			this->richTextBox8->Multiline = false;
			this->richTextBox8->Name = L"richTextBox8";
			this->richTextBox8->Size = System::Drawing::Size(108, 26);
			this->richTextBox8->TabIndex = 12;
			this->richTextBox8->Text = L"";
			// 
			// label5
			// 
			this->label5->AutoSize = true;
			this->label5->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->label5->Location = System::Drawing::Point(4, 153);
			this->label5->Margin = System::Windows::Forms::Padding(2, 0, 2, 0);
			this->label5->Name = L"label5";
			this->label5->Size = System::Drawing::Size(73, 20);
			this->label5->TabIndex = 11;
			this->label5->Text = L"车牌颜色";
			// 
			// richTextBox7
			// 
			this->richTextBox7->Enabled = false;
			this->richTextBox7->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 13.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->richTextBox7->Location = System::Drawing::Point(90, 148);
			this->richTextBox7->Margin = System::Windows::Forms::Padding(2);
			this->richTextBox7->Multiline = false;
			this->richTextBox7->Name = L"richTextBox7";
			this->richTextBox7->Size = System::Drawing::Size(108, 26);
			this->richTextBox7->TabIndex = 10;
			this->richTextBox7->Text = L"";
			// 
			// label3
			// 
			this->label3->AutoSize = true;
			this->label3->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->label3->Location = System::Drawing::Point(4, 123);
			this->label3->Margin = System::Windows::Forms::Padding(2, 0, 2, 0);
			this->label3->Name = L"label3";
			this->label3->Size = System::Drawing::Size(73, 20);
			this->label3->TabIndex = 9;
			this->label3->Text = L"改点张数";
			// 
			// richTextBox4
			// 
			this->richTextBox4->Enabled = false;
			this->richTextBox4->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 13.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->richTextBox4->Location = System::Drawing::Point(90, 118);
			this->richTextBox4->Margin = System::Windows::Forms::Padding(2);
			this->richTextBox4->Multiline = false;
			this->richTextBox4->Name = L"richTextBox4";
			this->richTextBox4->Size = System::Drawing::Size(108, 26);
			this->richTextBox4->TabIndex = 8;
			this->richTextBox4->Text = L"";
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->label2->Location = System::Drawing::Point(4, 93);
			this->label2->Margin = System::Windows::Forms::Padding(2, 0, 2, 0);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(73, 20);
			this->label2->TabIndex = 7;
			this->label2->Text = L"已绘点数";
			// 
			// richTextBox2
			// 
			this->richTextBox2->Enabled = false;
			this->richTextBox2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 13.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->richTextBox2->Location = System::Drawing::Point(90, 90);
			this->richTextBox2->Margin = System::Windows::Forms::Padding(2);
			this->richTextBox2->Multiline = false;
			this->richTextBox2->Name = L"richTextBox2";
			this->richTextBox2->Size = System::Drawing::Size(108, 26);
			this->richTextBox2->TabIndex = 6;
			this->richTextBox2->Text = L"";
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->label1->Location = System::Drawing::Point(18, 64);
			this->label1->Margin = System::Windows::Forms::Padding(2, 0, 2, 0);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(57, 20);
			this->label1->TabIndex = 5;
			this->label1->Text = L"当前帧";
			// 
			// richTextBox1
			// 
			this->richTextBox1->Enabled = false;
			this->richTextBox1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 13.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->richTextBox1->Location = System::Drawing::Point(90, 61);
			this->richTextBox1->Margin = System::Windows::Forms::Padding(2);
			this->richTextBox1->Multiline = false;
			this->richTextBox1->Name = L"richTextBox1";
			this->richTextBox1->Size = System::Drawing::Size(108, 26);
			this->richTextBox1->TabIndex = 2;
			this->richTextBox1->Text = L"";
			// 
			// button2
			// 
			this->button2->Location = System::Drawing::Point(55, 16);
			this->button2->Margin = System::Windows::Forms::Padding(2);
			this->button2->Name = L"button2";
			this->button2->Size = System::Drawing::Size(452, 32);
			this->button2->TabIndex = 1;
			this->button2->Text = L"从.set导入";
			this->button2->UseVisualStyleBackColor = true;
			this->button2->Click += gcnew System::EventHandler(this, &MyForm::button2_Click);
			// 
			// button1
			// 
			this->button1->Location = System::Drawing::Point(7, 19);
			this->button1->Margin = System::Windows::Forms::Padding(2);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(44, 26);
			this->button1->TabIndex = 0;
			this->button1->Text = L"清空";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &MyForm::button1_Click);
			// 
			// groupBox3
			// 
			this->groupBox3->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->groupBox3->Controls->Add(this->button5);
			this->groupBox3->Controls->Add(this->button3);
			this->groupBox3->Controls->Add(this->richTextBox3);
			this->groupBox3->Controls->Add(this->button4);
			this->groupBox3->Location = System::Drawing::Point(970, 530);
			this->groupBox3->Margin = System::Windows::Forms::Padding(2);
			this->groupBox3->Name = L"groupBox3";
			this->groupBox3->Padding = System::Windows::Forms::Padding(2);
			this->groupBox3->Size = System::Drawing::Size(213, 134);
			this->groupBox3->TabIndex = 3;
			this->groupBox3->TabStop = false;
			this->groupBox3->Text = L"帧操作";
			// 
			// button5
			// 
			this->button5->Location = System::Drawing::Point(112, 100);
			this->button5->Margin = System::Windows::Forms::Padding(2);
			this->button5->Name = L"button5";
			this->button5->Size = System::Drawing::Size(62, 25);
			this->button5->TabIndex = 8;
			this->button5->Text = L"跳帧";
			this->button5->UseVisualStyleBackColor = true;
			this->button5->Click += gcnew System::EventHandler(this, &MyForm::button5_Click);
			// 
			// button3
			// 
			this->button3->Location = System::Drawing::Point(35, 16);
			this->button3->Margin = System::Windows::Forms::Padding(2);
			this->button3->Name = L"button3";
			this->button3->Size = System::Drawing::Size(138, 38);
			this->button3->TabIndex = 3;
			this->button3->Text = L"上一帧(F1)";
			this->button3->UseVisualStyleBackColor = true;
			this->button3->Click += gcnew System::EventHandler(this, &MyForm::button3_Click);
			// 
			// richTextBox3
			// 
			this->richTextBox3->Location = System::Drawing::Point(35, 100);
			this->richTextBox3->Margin = System::Windows::Forms::Padding(2);
			this->richTextBox3->Multiline = false;
			this->richTextBox3->Name = L"richTextBox3";
			this->richTextBox3->Size = System::Drawing::Size(73, 26);
			this->richTextBox3->TabIndex = 7;
			this->richTextBox3->Text = L"";
			this->richTextBox3->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &MyForm::richTextBox3_KeyDown);
			// 
			// button4
			// 
			this->button4->Location = System::Drawing::Point(35, 58);
			this->button4->Margin = System::Windows::Forms::Padding(2);
			this->button4->Name = L"button4";
			this->button4->Size = System::Drawing::Size(138, 38);
			this->button4->TabIndex = 6;
			this->button4->Text = L"下一帧(F2)";
			this->button4->UseVisualStyleBackColor = true;
			this->button4->Click += gcnew System::EventHandler(this, &MyForm::button4_Click);
			// 
			// groupBox1
			// 
			this->groupBox1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->groupBox1->Controls->Add(this->button7);
			this->groupBox1->Location = System::Drawing::Point(545, 12);
			this->groupBox1->Name = L"groupBox1";
			this->groupBox1->Size = System::Drawing::Size(192, 70);
			this->groupBox1->TabIndex = 4;
			this->groupBox1->TabStop = false;
			this->groupBox1->Text = L"附加功能";
			// 
			// button7
			// 
			this->button7->Location = System::Drawing::Point(22, 16);
			this->button7->Margin = System::Windows::Forms::Padding(2);
			this->button7->Name = L"button7";
			this->button7->Size = System::Drawing::Size(150, 41);
			this->button7->TabIndex = 5;
			this->button7->Text = L"开启防误触\r\n（开启后图像不能点）";
			this->button7->UseVisualStyleBackColor = true;
			this->button7->Click += gcnew System::EventHandler(this, &MyForm::button7_Click);
			// 
			// groupBox4
			// 
			this->groupBox4->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->groupBox4->Controls->Add(this->button6);
			this->groupBox4->Location = System::Drawing::Point(742, 12);
			this->groupBox4->Margin = System::Windows::Forms::Padding(2);
			this->groupBox4->Name = L"groupBox4";
			this->groupBox4->Padding = System::Windows::Forms::Padding(2);
			this->groupBox4->Size = System::Drawing::Size(213, 70);
			this->groupBox4->TabIndex = 5;
			this->groupBox4->TabStop = false;
			this->groupBox4->Text = L" 导出";
			// 
			// button6
			// 
			this->button6->Location = System::Drawing::Point(35, 18);
			this->button6->Margin = System::Windows::Forms::Padding(2);
			this->button6->Name = L"button6";
			this->button6->Size = System::Drawing::Size(136, 37);
			this->button6->TabIndex = 2;
			this->button6->Text = L"导出修改图片\r\n（替换原有）";
			this->button6->UseVisualStyleBackColor = true;
			this->button6->Click += gcnew System::EventHandler(this, &MyForm::button6_Click);
			// 
			// openFileDialog1
			// 
			this->openFileDialog1->FileName = L"openFileDialog1";
			// 
			// groupBox5
			// 
			this->groupBox5->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->groupBox5->Controls->Add(this->label8);
			this->groupBox5->Controls->Add(this->label7);
			this->groupBox5->Controls->Add(this->button16);
			this->groupBox5->Controls->Add(this->button15);
			this->groupBox5->Controls->Add(this->button14);
			this->groupBox5->Controls->Add(this->button13);
			this->groupBox5->Controls->Add(this->button12);
			this->groupBox5->Controls->Add(this->button11);
			this->groupBox5->Controls->Add(this->button10);
			this->groupBox5->Controls->Add(this->button9);
			this->groupBox5->Controls->Add(this->button8);
			this->groupBox5->Location = System::Drawing::Point(970, 340);
			this->groupBox5->Name = L"groupBox5";
			this->groupBox5->Size = System::Drawing::Size(215, 185);
			this->groupBox5->TabIndex = 6;
			this->groupBox5->TabStop = false;
			this->groupBox5->Text = L"车牌颜色/类型设置";
			// 
			// label8
			// 
			this->label8->AutoSize = true;
			this->label8->Location = System::Drawing::Point(8, 20);
			this->label8->Name = L"label8";
			this->label8->Size = System::Drawing::Size(77, 12);
			this->label8->TabIndex = 10;
			this->label8->Text = L"车牌颜色设置";
			// 
			// label7
			// 
			this->label7->AutoSize = true;
			this->label7->Location = System::Drawing::Point(8, 80);
			this->label7->Name = L"label7";
			this->label7->Size = System::Drawing::Size(77, 12);
			this->label7->TabIndex = 9;
			this->label7->Text = L"车牌类型设置";
			// 
			// button16
			// 
			this->button16->Location = System::Drawing::Point(21, 139);
			this->button16->Name = L"button16";
			this->button16->Size = System::Drawing::Size(164, 40);
			this->button16->TabIndex = 8;
			this->button16->Text = L"设当前颜色和类型为默认\r\n（重开软件生效）";
			this->button16->UseVisualStyleBackColor = true;
			this->button16->Click += gcnew System::EventHandler(this, &MyForm::button16_Click);
			// 
			// button15
			// 
			this->button15->Location = System::Drawing::Point(90, 104);
			this->button15->Name = L"button15";
			this->button15->Size = System::Drawing::Size(96, 30);
			this->button15->TabIndex = 7;
			this->button15->Text = L"假（虚警）";
			this->button15->UseVisualStyleBackColor = true;
			this->button15->Click += gcnew System::EventHandler(this, &MyForm::button15_Click);
			// 
			// button14
			// 
			this->button14->Location = System::Drawing::Point(56, 104);
			this->button14->Name = L"button14";
			this->button14->Size = System::Drawing::Size(28, 30);
			this->button14->TabIndex = 6;
			this->button14->Text = L"双";
			this->button14->UseVisualStyleBackColor = true;
			this->button14->Click += gcnew System::EventHandler(this, &MyForm::button14_Click);
			// 
			// button13
			// 
			this->button13->Location = System::Drawing::Point(22, 104);
			this->button13->Name = L"button13";
			this->button13->Size = System::Drawing::Size(28, 30);
			this->button13->TabIndex = 5;
			this->button13->Text = L"单";
			this->button13->UseVisualStyleBackColor = true;
			this->button13->Click += gcnew System::EventHandler(this, &MyForm::button13_Click);
			// 
			// button12
			// 
			this->button12->Location = System::Drawing::Point(157, 20);
			this->button12->Name = L"button12";
			this->button12->Size = System::Drawing::Size(35, 46);
			this->button12->TabIndex = 4;
			this->button12->Text = L"未知";
			this->button12->UseVisualStyleBackColor = true;
			this->button12->Click += gcnew System::EventHandler(this, &MyForm::button12_Click);
			// 
			// button11
			// 
			this->button11->Location = System::Drawing::Point(55, 38);
			this->button11->Name = L"button11";
			this->button11->Size = System::Drawing::Size(28, 28);
			this->button11->TabIndex = 3;
			this->button11->Text = L"黄";
			this->button11->UseVisualStyleBackColor = true;
			this->button11->Click += gcnew System::EventHandler(this, &MyForm::button11_Click);
			// 
			// button10
			// 
			this->button10->Location = System::Drawing::Point(21, 38);
			this->button10->Name = L"button10";
			this->button10->Size = System::Drawing::Size(28, 28);
			this->button10->TabIndex = 2;
			this->button10->Text = L"蓝";
			this->button10->UseVisualStyleBackColor = true;
			this->button10->Click += gcnew System::EventHandler(this, &MyForm::button10_Click);
			// 
			// button9
			// 
			this->button9->Location = System::Drawing::Point(89, 38);
			this->button9->Name = L"button9";
			this->button9->Size = System::Drawing::Size(28, 28);
			this->button9->TabIndex = 1;
			this->button9->Text = L"白";
			this->button9->UseVisualStyleBackColor = true;
			this->button9->Click += gcnew System::EventHandler(this, &MyForm::button9_Click);
			// 
			// button8
			// 
			this->button8->Location = System::Drawing::Point(123, 38);
			this->button8->Name = L"button8";
			this->button8->Size = System::Drawing::Size(28, 28);
			this->button8->TabIndex = 0;
			this->button8->Text = L"黑";
			this->button8->UseVisualStyleBackColor = true;
			this->button8->Click += gcnew System::EventHandler(this, &MyForm::button8_Click);
			// 
			// groupBox6
			// 
			this->groupBox6->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->groupBox6->Controls->Add(this->richTextBox10);
			this->groupBox6->Location = System::Drawing::Point(970, 262);
			this->groupBox6->Name = L"groupBox6";
			this->groupBox6->Size = System::Drawing::Size(211, 72);
			this->groupBox6->TabIndex = 7;
			this->groupBox6->TabStop = false;
			this->groupBox6->Text = L"车牌号码设置(离开当前图片会自动确认）";
			// 
			// richTextBox10
			// 
			this->richTextBox10->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 13.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->richTextBox10->Location = System::Drawing::Point(18, 29);
			this->richTextBox10->Margin = System::Windows::Forms::Padding(2);
			this->richTextBox10->MaxLength = 16;
			this->richTextBox10->Multiline = false;
			this->richTextBox10->Name = L"richTextBox10";
			this->richTextBox10->Size = System::Drawing::Size(173, 26);
			this->richTextBox10->TabIndex = 15;
			this->richTextBox10->Text = L"1234567";
			this->richTextBox10->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &MyForm::richTextBox10_KeyDown);
			// 
			// groupBox7
			// 
			this->groupBox7->Controls->Add(this->button2);
			this->groupBox7->Controls->Add(this->button1);
			this->groupBox7->Location = System::Drawing::Point(12, 12);
			this->groupBox7->Name = L"groupBox7";
			this->groupBox7->Size = System::Drawing::Size(512, 55);
			this->groupBox7->TabIndex = 8;
			this->groupBox7->TabStop = false;
			this->groupBox7->Text = L"预处理";
			// 
			// MyForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 12);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->AutoSizeMode = System::Windows::Forms::AutoSizeMode::GrowAndShrink;
			this->BackColor = System::Drawing::Color::LightGreen;
			this->ClientSize = System::Drawing::Size(1194, 938);
			this->Controls->Add(this->groupBox7);
			this->Controls->Add(this->groupBox6);
			this->Controls->Add(this->groupBox4);
			this->Controls->Add(this->groupBox5);
			this->Controls->Add(this->groupBox1);
			this->Controls->Add(this->groupBox3);
			this->Controls->Add(this->groupBox2);
			this->Controls->Add(this->tabControl1);
			this->Controls->Add(this->statusStrip1);
			this->ForeColor = System::Drawing::SystemColors::ActiveCaptionText;
			this->KeyPreview = true;
			this->Name = L"MyForm";
			this->Text = L"PointCheck+护眼版";
			this->WindowState = System::Windows::Forms::FormWindowState::Maximized;
			this->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(this, &MyForm::MyForm_FormClosing);
			this->Load += gcnew System::EventHandler(this, &MyForm::MyForm_Load);
			this->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &MyForm::MyForm_KeyDown);
			this->tabControl1->ResumeLayout(false);
			this->tabPage1->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox1))->EndInit();
			this->tabPage2->ResumeLayout(false);
			this->tabPage3->ResumeLayout(false);
			this->groupBox2->ResumeLayout(false);
			this->groupBox2->PerformLayout();
			this->groupBox3->ResumeLayout(false);
			this->groupBox1->ResumeLayout(false);
			this->groupBox4->ResumeLayout(false);
			this->groupBox5->ResumeLayout(false);
			this->groupBox5->PerformLayout();
			this->groupBox6->ResumeLayout(false);
			this->groupBox7->ResumeLayout(false);
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion



	//清空键执行的功能
	private: System::Void button1_Click(System::Object^  sender, System::EventArgs^  e) {
		OrderIndicate = 0;	//复位顺序预检结果
		button6_Click(sender, e);	//模拟点击保存图片按钮
		if (LocNum[SelectedRow] != 5 && LocNum[SelectedRow] != 0) {
			MessageBox::Show("当前图片标注点数不为0或5时不能清空！");
		}
		else {
			listView1->Items->Clear();
			PicCount = 0;
			SelectedRow = 0;
			pictureBox1->Image = nullptr;	//清空显示图片
			richTextBox1->Text = "";	//当前帧显示清空
			richTextBox5->Text = "";	//当前预检显示清空
		}
		button2->Enabled = true;	//清空后才能继续导入.set
	}



	//正常导入.set时执行的操作
	private: System::Void button2_Click(System::Object^  sender, System::EventArgs^  e) {
		button2->Enabled = false; //只允许导入一次.set，除非点清空后才能继续导入(因为读完的已经导出图片，文件名已变)

		//打开.set文件的对话框
		if (openFileDialog1->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
			StreamReader^ sr = gcnew StreamReader(openFileDialog1->FileName, System::Text::Encoding::Default);
			String^ Text = sr->ReadLine();
			Text->Replace("\\", "\\\\");	//c++中要求双斜杠
			L = Text->Split('\\')->Length;	//双斜杠分割后的长度

			directory = "";		//清空内存中的当前目录记录
			for (int index = 0; index < L - 1; index++) {
				directory += Text->Split('\\')[index] + "\\";	//求出当前directory
			}

			if (!File::Exists(Text)) {
				MessageBox::Show("该.set中的图片文件路径不存在！");
				return;
			}

			//创建历史记录目录
			if (!Directory::Exists("D:\\PointCheck+护眼版_Log")) {
				Directory::CreateDirectory("D:\\PointCheck+护眼版_Log");
			}

			// load image to img
			// Convert System::String to string
			std::string tmp;
			MarshalString(Text, tmp);
			cvReleaseImage(&img);
			img = cvLoadImage(tmp.c_str()); //load img
			
			// Save Points Location to RAM
			LocSave(Text, L, img);

			//SelectedRow = PicCount + 1;
			//toolStripStatusLabel1->Text = "加载中";
			PicCount += 1;	// total image number plus 1
			richTextBox1->Text = (PicCount).ToString();	// show the current image index number
			richTextBox5->Text = PreflightCheck(Text, L)->Substring(0,1);	//show the Preflight Check result in box
			if (richTextBox5->Text == "错") {
				OrderIndicate = 1;
			}
			richTextBox2->Text = "5";
			richTextBox4->Text = "0";
			ListViewItem^ item1 = gcnew ListViewItem(PicCount.ToString(), 0);

			// add info of current image to the listview1
			item1->SubItems->Add(Text->Split('\\')[L - 1]);
			int L_dot = 0;
			L_dot = Text->Split('.')->Length;
			item1->SubItems->Add(Text->Split('.')[L_dot - 1]);
			item1->SubItems->Add(img->width.ToString() + "*" + img->height.ToString());
			if (PreflightCheck(Text, L)->Substring(0, 1) == "错") {
				OrderIndicate = 1;
			}
			item1->SubItems->Add(PreflightCheck(Text,L));
			item1->SubItems->Add(Text->Split('\\')[L - 1]->Split('[')[1]->Split(']')[0]);
			item1->SubItems->Add(Text);
			item1->SubItems->Add(Text);
			if (Text->Split('\\')[L - 1]->Split('_')->Length == 4) { //如果存在车牌号、颜色、类型标志
				item1->SubItems->Add(Text->Split('\\')[L - 1]->Split('_')[1]);	// 车牌号
				item1->SubItems->Add(ColorList->Split(',')[int::Parse(Text->Split('\\')[L - 1]->Split('_')[2])]); // 颜色
				item1->SubItems->Add(TypeList->Split(',')[int::Parse(Text->Split('\\')[L - 1]->Split('_')[3]->Split('.')[0])]); // 类型
			}
			else {
				item1->SubItems->Add("1234567");	// 默认1234567
				item1->SubItems->Add(DefaultColor);	// 默认蓝色
				item1->SubItems->Add(DefaultType);	// 默认单层
			}
			listView1->Items->Add(item1);
			try {
					do {
						Text = sr->ReadLine();
						PicCount += 1;
						item1 = gcnew ListViewItem(PicCount.ToString(), 0);
						item1->SubItems->Add(Text->Split('\\')[L - 1]);
						int L_dot = 0;
						L_dot = Text->Split('.')->Length;
						item1->SubItems->Add(Text->Split('.')[L_dot - 1]);
						item1->SubItems->Add("");
						item1->SubItems->Add(PreflightCheck(Text, L));
						if (PreflightCheck(Text, L)->Substring(0, 1) == "错") {
							OrderIndicate = 1;
						}
						item1->SubItems->Add(Text->Split('\\')[L - 1]->Split('[')[1]->Split(']')[0]);
						item1->SubItems->Add(Text);
						item1->SubItems->Add(Text);
						if (Text->Split('\\')[L - 1]->Split('_')->Length == 4) { //如果存在颜色、类型标志
							item1->SubItems->Add(Text->Split('\\')[L - 1]->Split('_')[1]);	// 车牌号
							item1->SubItems->Add(ColorList->Split(',')[int::Parse(Text->Split('\\')[L - 1]->Split('_')[2])]);	
							item1->SubItems->Add(TypeList->Split(',')[int::Parse(Text->Split('\\')[L - 1]->Split('_')[3]->Split('.')[0])]);
						}
						else {
							item1->SubItems->Add("1234567");	// 默认1234567
							item1->SubItems->Add(DefaultColor);	// 默认蓝色
							item1->SubItems->Add(DefaultType);	// 默认单层
						}
						listView1->Items->Add(item1);
					} while (Text);
					//toolStripStatusLabel1->Text = "加载完成";
				}
			catch (Exception^ e) {
					PicCount -= 1;
					Console::WriteLine("The file could not be read:");
					Console::WriteLine(e->Message);
				}

			// Show img in pictureBox1
			pictureBox1->Image = gcnew System::Drawing::Bitmap(img->width, img->height, img->widthStep,
				System::Drawing::Imaging::PixelFormat::Format24bppRgb, (System::IntPtr) img->imageData);
			pictureBox1->Refresh();

			richTextBox7->Text = listView1->Items[0]->SubItems[9]->Text;	// 显示颜色
			richTextBox8->Text = listView1->Items[0]->SubItems[10]->Text;	// 显示类型
			richTextBox9->Text = listView1->Items[0]->SubItems[8]->Text;	// 显示车牌号码
			richTextBox10->Text = listView1->Items[0]->SubItems[8]->Text;	// 显示车牌号码

			// MessageBox mention richTextBox5 content
			if (richTextBox5->Text == "错") {
				MessageBox::Show("本张 标点顺序错误 或 中心点偏出了边框！");
			}
			if (OrderIndicate == 0) {
				MessageBox::Show("恭喜！所有图片通过顺序预检\n只检查标点位置即可！");
			}
			// Draw points on picture


		}//end of openfile dialog
	}



	//上一帧执行的功能
	private: System::Void button3_Click(System::Object^  sender, System::EventArgs^  e) {
		if (listView1->Items[SelectedRow]->SubItems[8]->Text != richTextBox10->Text) {	// 如果有改动
			listView1->Items[SelectedRow]->SubItems[8]->Text = richTextBox10->Text;	// 执行之前把当前帧的车牌号码存入listview中
			ChangedOrNot[SelectedRow] = 1;
			OutHistory();
		}		
		if (SelectedRow == 0) {
			MessageBox::Show("已经到第一帧！");
		}
		else if (LocNum[SelectedRow] != 5) { //当前图片开始标注但未标注完（标注点数1~4）
			MessageBox::Show("请在 点完5个点 的情况下切换帧！");
		}
		else {
			SelectedRow = SelectedRow - 1;
			richTextBox1->Text = (SelectedRow + 1).ToString();
			//richTextBox2->Text = LocNum[SelectedRow].ToString();

			// Convert System::String to string
			String^ Text_ori = listView1->Items[SelectedRow]->SubItems[7]->Text;
			Text_ori->Replace("\\", "\\\\");
			std::string tmp;
			MarshalString(Text_ori, tmp);
			cvReleaseImage(&img);
			img = cvLoadImage(tmp.c_str()); //load img

			String^ Text = listView1->Items[SelectedRow]->SubItems[6]->Text;
			Text->Replace("\\", "\\\\");
			// Save Points Location to RAM (must update because jump is allowed)
			L = Text->Split('\\')->Length;	//双斜杠分割后的长度
			richTextBox5->Text = PreflightCheck(Text, L)->Substring(0, 1);	//show the Preflight Check result in box
			LocSave(Text, L, img);

			if (listView1->Items[SelectedRow]->SubItems[3]->Text == "") {
				listView1->Items[SelectedRow]->SubItems[3]->Text = img->width.ToString() + "*" + img->height.ToString();
			}

			// Show img_small in pictureBox1
			pictureBox1->Image = gcnew System::Drawing::Bitmap(img->width, img->height, img->widthStep,
				System::Drawing::Imaging::PixelFormat::Format24bppRgb, (System::IntPtr) img->imageData);
			pictureBox1->Refresh();

			richTextBox7->Text = listView1->Items[SelectedRow]->SubItems[9]->Text;	// 显示颜色
			richTextBox8->Text = listView1->Items[SelectedRow]->SubItems[10]->Text;	// 显示类型
			richTextBox9->Text = listView1->Items[SelectedRow]->SubItems[8]->Text;	// 显示车牌号码
			richTextBox10->Text = listView1->Items[SelectedRow]->SubItems[8]->Text;	// 显示车牌号码

			// MessageBox mention richTextBox5 content
			if (richTextBox5->Text == "错") {
				MessageBox::Show("本张 标点顺序错误 或 中心点偏出了边框！");
			}
		}
	}



	//下一帧执行的功能
	private: System::Void button4_Click(System::Object^  sender, System::EventArgs^  e) {
		if (listView1->Items[SelectedRow]->SubItems[8]->Text != richTextBox10->Text) {	// 如果有改动
			listView1->Items[SelectedRow]->SubItems[8]->Text = richTextBox10->Text;	// 执行之前把当前帧的车牌号码存入listview中
			ChangedOrNot[SelectedRow] = 1;
			OutHistory();
		}
		if (SelectedRow == PicCount - 1 || PicCount == 0) {
			MessageBox::Show("已经到最后一帧！");
		}
		else if (LocNum[SelectedRow] != 5) { //当前图片开始标注但未标注完（标注点数1~4）
			MessageBox::Show("请在 点完5个点 的情况下切换帧！");
		}
		else {
			SelectedRow = SelectedRow + 1;
			richTextBox1->Text = (SelectedRow + 1).ToString();
			//richTextBox2->Text = LocNum[SelectedRow].ToString();	

			// Convert System::String to string
			String^ Text_ori = listView1->Items[SelectedRow]->SubItems[7]->Text;
			Text_ori->Replace("\\", "\\\\");
			std::string tmp;
			MarshalString(Text_ori, tmp);
			cvReleaseImage(&img);
			img = cvLoadImage(tmp.c_str()); //load img

			String^ Text = listView1->Items[SelectedRow]->SubItems[6]->Text;
			Text->Replace("\\", "\\\\");
			// Save Points Location to RAM
			L = Text->Split('\\')->Length;	//双斜杠分割后的长度
			richTextBox5->Text = PreflightCheck(Text, L)->Substring(0, 1);	//show the Preflight Check result in box
			LocSave(Text, L, img);
			
			// Update the img size in listview
			if (listView1->Items[SelectedRow]->SubItems[3]->Text == "") {
				listView1->Items[SelectedRow]->SubItems[3]->Text = img->width.ToString() + "*" + img->height.ToString();
			}
			// Show img in pictureBox1
			pictureBox1->Image = gcnew System::Drawing::Bitmap(img->width, img->height, img->widthStep,
				System::Drawing::Imaging::PixelFormat::Format24bppRgb, (System::IntPtr) img->imageData);
			pictureBox1->Refresh();

			richTextBox7->Text = listView1->Items[SelectedRow]->SubItems[9]->Text;	// 显示颜色
			richTextBox8->Text = listView1->Items[SelectedRow]->SubItems[10]->Text;	// 显示类型
			richTextBox9->Text = listView1->Items[SelectedRow]->SubItems[8]->Text;	// 显示车牌号码
			richTextBox10->Text = listView1->Items[SelectedRow]->SubItems[8]->Text;	// 显示车牌号码

			// MessageBox mention richTextBox5 content
			if (richTextBox5->Text == "错") {
				MessageBox::Show("本张 标点顺序错误 或 中心点偏出了边框！");
			}
		}
	}



	//跳帧键执行的功能
	private: System::Void button5_Click(System::Object^  sender, System::EventArgs^  e) {
		if (listView1->Items[SelectedRow]->SubItems[8]->Text != richTextBox10->Text) {	// 如果有改动
			listView1->Items[SelectedRow]->SubItems[8]->Text = richTextBox10->Text;	// 执行之前把当前帧的车牌号码存入listview中
			ChangedOrNot[SelectedRow] = 1;
			OutHistory();
		}		
		std::string tmpJump;
		MarshalString(richTextBox3->Text, tmpJump);
		double jumpCount = atoi(tmpJump.c_str());	//richTextBox3里写的想要跳往的张数
		if (PicCount == 0) {
			MessageBox::Show("未导入图片！");
		}
		else {
			if (jumpCount == (int)jumpCount && jumpCount>0 && jumpCount <= PicCount) {
				if (LocNum[SelectedRow] != 5) { //当前图片开始标注但未标注完（标注点数1~4）
					MessageBox::Show("请在 点完5个点 的情况下切换帧！");
				}
				else {
					//SelectedRow = int::Parse(richTextBox2->Text);
					SelectedRow = jumpCount - 1;
					richTextBox1->Text = richTextBox3->Text;

					// Convert System::String to string
					String^ Text_ori = listView1->Items[SelectedRow]->SubItems[7]->Text;
					Text_ori->Replace("\\", "\\\\");
					std::string tmp;
					MarshalString(Text_ori, tmp);
					cvReleaseImage(&img);
					img = cvLoadImage(tmp.c_str()); //load img

					String^ Text = listView1->Items[SelectedRow]->SubItems[6]->Text;
					Text->Replace("\\", "\\\\");
					// Save Points Location to RAM
					L = Text->Split('\\')->Length;	//双斜杠分割后的长度
					richTextBox5->Text = PreflightCheck(Text, L)->Substring(0, 1);	//show the Preflight Check result in box
					LocSave(Text, L, img);
					
					//update the size
					if (listView1->Items[SelectedRow]->SubItems[3]->Text == "") {
						listView1->Items[SelectedRow]->SubItems[3]->Text = img->width.ToString() + "*" + img->height.ToString();
					}

					// Show img in pictureBox1
					pictureBox1->Image = gcnew System::Drawing::Bitmap(img->width, img->height, img->widthStep,
						System::Drawing::Imaging::PixelFormat::Format24bppRgb, (System::IntPtr) img->imageData);
					pictureBox1->Refresh();

					richTextBox7->Text = listView1->Items[SelectedRow]->SubItems[9]->Text;	// 显示颜色
					richTextBox8->Text = listView1->Items[SelectedRow]->SubItems[10]->Text;	// 显示类型
					richTextBox9->Text = listView1->Items[SelectedRow]->SubItems[8]->Text;	// 显示车牌号码
					richTextBox10->Text = listView1->Items[SelectedRow]->SubItems[8]->Text;	// 显示车牌号码

					// MessageBox mention richTextBox5 content
					if (richTextBox5->Text == "错") {
						MessageBox::Show("本张 标点顺序错误 或 中心点偏出了边框！");
					}
				}
			}
			else {
				MessageBox::Show("输入不合法！请重新输入！");
			}
		}
	}



	//导出修改过的图片（会替换原有）
	private: System::Void button6_Click(System::Object^  sender, System::EventArgs^  e) {
		if (PicCount!=0){ 
			// 存车牌号
			if (listView1->Items[SelectedRow]->SubItems[8]->Text != richTextBox10->Text) {	// 如果有改动
				listView1->Items[SelectedRow]->SubItems[8]->Text = richTextBox10->Text;	// 执行之前把当前帧的车牌号码存入listview中
				ChangedOrNot[SelectedRow] = 1;
				OutHistory();
			}
		}	// end of if (PicCount!=0)

		// 导出history.log里面记录的更改过的图片
		if (File::Exists("D:\\PointCheck+护眼版_Log\\history.log")) {
			// open "D:\\PointCheck+护眼版_Log\\history.log"
			FileStream^ history = gcnew FileStream("D:\\PointCheck+护眼版_Log\\history.log", FileMode::Open, FileAccess::Read);
			StreamReader^ tmphistory = gcnew StreamReader(history, System::Text::Encoding::Unicode);
			try {
				String^ CurrentLine = "start";
				do {
					// read modified img context from history.log line by line
					CurrentLine = tmphistory->ReadLine();
					int Len_curr = CurrentLine->Split(' ')->Length;
					String^ ImgPath = "";
					for (int i = 1; i < Len_curr - 4; i++) {
						ImgPath += CurrentLine->Split(' ')[i];
						if (i != Len_curr - 4) {
							ImgPath += " ";
						}
					}
					String^ NewKeyPoints = CurrentLine->Split(' ')[Len_curr - 4];
					String^ NewPath = "";
					// 都是改过的图片，即使原来有类标也忽略
					String^ tmpLicense = CurrentLine->Split(' ')[Len_curr - 3]; // 车牌号码
					int tmpColor = ColorTypeConvert(CurrentLine->Split(' ')[Len_curr - 2]);	// 颜色标志
					int tmpType = ColorTypeConvert(CurrentLine->Split(' ')[Len_curr - 1]);	// 类别标志
					NewPath = ImgPath->Split('[')[0] + "[" + NewKeyPoints + "]" + "_" + tmpLicense + "_" + tmpColor +"_"+ tmpType 
						+ "." + ImgPath->Split(']')[1]->Split('.')[1];
			

					if (!File::Exists(NewPath)) {	//如果点的点正好一样就不存了
						// save the new image to the directory
						std::string NewPathStr = "";
						MarshalString(NewPath, NewPathStr);
						std::string tmpOut;
						MarshalString(ImgPath, tmpOut);
						img_output = cvLoadImage(tmpOut.c_str());
						cvSaveImage(NewPathStr.c_str(), img_output, 0);	//存储图片
						cvReleaseImage(&img_output);
						Console::WriteLine("File saved: {0}", NewPath);

						// cut the olg img file to "D:\\PointCheck+护眼版_Log\\OldImg"
						String^ ImgName = "";
						int len_path = ImgPath->Split('\\')->Length;
						ImgName = ImgPath->Split('\\')[len_path - 1];
						if (!Directory::Exists("D:\\PointCheck+护眼版_Log\\OldImg")) {
							Directory::CreateDirectory("D:\\PointCheck+护眼版_Log\\OldImg");
						}
						File::Copy(ImgPath, "D:\\PointCheck+护眼版_Log\\OldImg\\" + ImgName);
						File::Delete(ImgPath);
					}	//end of file NewPath exists

				} while (CurrentLine);
			}
			catch (Exception^ e) {	//	may be completed
				Console::WriteLine("The file could not be read:");
				Console::WriteLine(e->Message);
			}
			tmphistory->Close();
			history->Close();
			//File::Copy("D:\\PointCheck+护眼版_Log\\history.log", "D:\\PointCheck+护眼版_Log\\history" + System::DateTime().ToString() + ".log");
			File::Copy("D:\\PointCheck+护眼版_Log\\history.log", "D:\\PointCheck+护眼版_Log\\history" + DateTime::Now.ToString("yyyy-MM-dd HH_mm_ss") + ".log");
			File::Delete("D:\\PointCheck+护眼版_Log\\history.log");
		}	//End of file exists
	}


	//防误触（开启后图像不能点）
	private: System::Void button7_Click(System::Object^  sender, System::EventArgs^  e) {
		if (pictureBox1->Enabled == true) {
			button7->Text = "关闭防误触\n（关闭后可以修改点）";
			pictureBox1->Enabled = false;
		}
		else if(pictureBox1->Enabled == false) {
			button7->Text = "开启防误触\n（开启后图像不能点）";
			pictureBox1->Enabled = true;
		}
	}



	//每次绘制pictureBox1时候的操作
	private: System::Void pictureBox1_Paint(System::Object^  sender, System::Windows::Forms::PaintEventArgs^  e) {
		if (pictureBox1->Image != nullptr) {
			SolidBrush^ RedBrush = gcnew SolidBrush(Color::Red);	// Create a new pen
			int CircleWidth = 8;
			for (int m = 0; m < LocNum[SelectedRow]; m++) {
				e->Graphics->FillEllipse(RedBrush, (int)LPLocBox->GetValue(SelectedRow, 2 * m) - CircleWidth / 2, 
					(int)LPLocBox->GetValue(SelectedRow, 2 * m + 1) - CircleWidth / 2, CircleWidth, CircleWidth);
			}
			delete RedBrush;	//Dispose of the pen.
		}
	}



	//鼠标在pictureBox1上点下后的的操作
	private: System::Void pictureBox1_MouseDown(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
		if (pictureBox1->Image) {

			//Compute the of license plate location
			double wfactor = (double)img->width / pictureBox1->ClientSize.Width;
			double hfactor = (double)img->height / pictureBox1->ClientSize.Height;
			double resizeFactor = 1 / Math::Max(wfactor, hfactor);
			double wdiff = (pictureBox1->ClientSize.Width - (double)img->width*resizeFactor) / 2;
			double hdiff = (pictureBox1->ClientSize.Height - (double)img->height*resizeFactor) / 2;

			//Convert screen mouse location to pictureBox1 mouse location
			Point^ tmpPoint = pictureBox1->PointToClient(Control::MousePosition);
			if (Math::Max(wfactor, hfactor) == wfactor) {
				if (tmpPoint->Y<hdiff || tmpPoint->Y>(hdiff + (double)img->height*resizeFactor)) {
					MessageBox::Show("请在图片高度范围内画点！");
				}
				else {	//在图片范围之内
					if (e->Button == System::Windows::Forms::MouseButtons::Left) {	//点击左键
						//save the point location
						if (LocNum[SelectedRow] < 5) {

							// Save the Original point location
							tmpLocX = tmpPoint->X*wfactor;
							tmpLocY = (tmpPoint->Y - hdiff)*wfactor;
							LPLoc->SetValue(tmpLocX, SelectedRow, 2 * LocNum[SelectedRow]); //change value of x
							LPLoc->SetValue(tmpLocY, SelectedRow, 2 * LocNum[SelectedRow] + 1); //change value of y
							listView1->Items[SelectedRow]->SubItems[5]->Text += "(" + tmpLocX.ToString() + "," + tmpLocY.ToString() + ")" + ".";
							LocNum[SelectedRow]++;

							//draw the point
							LPLocBox->SetValue(tmpPoint->X, SelectedRow, 2 * (LocNum[SelectedRow] - 1));
							LPLocBox->SetValue(tmpPoint->Y, SelectedRow, 2 * (LocNum[SelectedRow] - 1) + 1);
							ChangedOrNot[SelectedRow] = 1;//标注图片改动
							pictureBox1->Invalidate();
						}
						else {
							MessageBox::Show("已点完5个点，如需重绘，请点击右键逐个取消！");
						}
						richTextBox2->Text = LocNum[SelectedRow].ToString();
					}
					else if (e->Button == System::Windows::Forms::MouseButtons::Right) {
						//draw the point
						pictureBox1->Invalidate();
						//save the point location
						if (LocNum[SelectedRow] > 0) {
							LPLoc->SetValue(nullptr, SelectedRow, 2 * (LocNum[SelectedRow] - 1));//change value of x
							LPLoc->SetValue(nullptr, SelectedRow, 2 * (LocNum[SelectedRow] - 1) + 1);//change value of y
							String^ tmpText = "";
							for (int j = 0; j < LocNum[SelectedRow] - 1; j++) {
								tmpText += listView1->Items[SelectedRow]->SubItems[5]->Text->Split('.')[j] + ".";
							}
							listView1->Items[SelectedRow]->SubItems[5]->Text = tmpText;
							LocNum[SelectedRow]--;
							ChangedOrNot[SelectedRow] = 1;//标注图片改动
						}
						else {
							MessageBox::Show("已没有点可以取消！");
						}
						richTextBox2->Text = LocNum[SelectedRow].ToString();
					}
				}
			}
			else if (Math::Max(wfactor, hfactor) == hfactor) {    //一般是这种情况
				if (tmpPoint->X<wdiff || tmpPoint->X>(wdiff + (double)img->width*resizeFactor)) {
					MessageBox::Show("请在图片宽度范围内画点！");
				}
				else {
					if (e->Button == System::Windows::Forms::MouseButtons::Left) {
						if (LocNum[SelectedRow] < 5) {
							//MessageBox::Show("sfg");
							tmpLocX = (tmpPoint->X - wdiff)*hfactor;
							tmpLocY = tmpPoint->Y*hfactor;
							LPLoc->SetValue(tmpLocX, SelectedRow, 2 * LocNum[SelectedRow]); //change value of x
							LPLoc->SetValue(tmpLocY, SelectedRow, 2 * LocNum[SelectedRow] + 1); //change value of y
							listView1->Items[SelectedRow]->SubItems[5]->Text += "(" + tmpLocX.ToString() + "," + tmpLocY.ToString() + ")" + ".";
							//ControlPaint::DrawReversibleLine(Control::MousePosition, Control::MousePosition, System::Drawing::Color::Red);
							LocNum[SelectedRow]++;
							//draw the point
							LPLocBox->SetValue(tmpPoint->X, SelectedRow, 2 * (LocNum[SelectedRow] - 1));
							LPLocBox->SetValue(tmpPoint->Y, SelectedRow, 2 * (LocNum[SelectedRow] - 1) + 1);
							ChangedOrNot[SelectedRow] = 1;//标注图片改动
							pictureBox1->Invalidate();
						}
						else {
							MessageBox::Show("已点完5个点，如需重绘，请点击右键逐个取消！");
						}
						richTextBox2->Text = LocNum[SelectedRow].ToString();
					}
					else if (e->Button == System::Windows::Forms::MouseButtons::Right) {
						//draw the point
						pictureBox1->Invalidate();
						if (LocNum[SelectedRow] > 0) {
							LPLoc->SetValue(nullptr, SelectedRow, 2 * (LocNum[SelectedRow] - 1));//change value of x
							LPLoc->SetValue(nullptr, SelectedRow, 2 * (LocNum[SelectedRow] - 1) + 1);//change value of y
							String^ tmpText = "";
							for (int j = 0; j < LocNum[SelectedRow] - 1; j++) {
								tmpText += listView1->Items[SelectedRow]->SubItems[5]->Text->Split('.')[j] + ".";
							}
							listView1->Items[SelectedRow]->SubItems[5]->Text = tmpText;
							LocNum[SelectedRow]--;
							ChangedOrNot[SelectedRow] = 1;//标注图片改动
						}
						else {
							MessageBox::Show("已没有点可以取消！");
						}
						richTextBox2->Text = LocNum[SelectedRow].ToString();
					}
				}
			}

			OutHistory();	//检查、输出到history.log并更新listview和richTextBox
		}	//end of picturebox1->image exists
	}


	//窗口加载时做的操作
	private: System::Void MyForm_Load(System::Object^  sender, System::EventArgs^  e) {

		//若没有D:\PointCheck+护眼版_Log\history.log则创建
		if (!Directory::Exists("D:\\PointCheck+护眼版_Log")) {
			Directory::CreateDirectory("D:\\PointCheck+护眼版_Log");
		}

		//从"D:\\PointCheck+护眼版_Log\\default.log"读取默认的车牌颜色和种类设置
		if (File::Exists("D:\\PointCheck+护眼版_Log\\Default.log")) {	// 如果Default.log文件存在
			// open "D:\\PointCheck+护眼版_Log\\Default.log"
			FileStream^ default = gcnew FileStream("D:\\PointCheck+护眼版_Log\\Default.log", FileMode::Open, FileAccess::Read);
			StreamReader^ tmpDefault = gcnew StreamReader(default, System::Text::Encoding::Unicode);
			DefaultColor = tmpDefault->ReadLine();	// 第一行是颜色
			DefaultType = tmpDefault->ReadLine();	// 第二行是类型
			tmpDefault->Close();
			default->Close();
		}
		else {
			DefaultColor = "蓝";	// 默认蓝，0,1,2,3,4对应黑，白，蓝，黄，绿色车牌
			DefaultType = "单层";	// 默认单层，0,1,2对应单层、双层和假（虚警）车牌
		}
	}


	// actions when MyForm Closing
	private: System::Void MyForm_FormClosing(System::Object^  sender, System::Windows::Forms::FormClosingEventArgs^  e) {
		button6_Click(sender, e); //模拟点击保存history.log中图片按键
	}



	// 使在跳帧时能够使用Enter（回车）键
	private: System::Void richTextBox3_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e) {
		if (e->KeyCode == Keys::Enter) {
			button5_Click(sender, e); //simulate keydown of button 5 跳帧
			button5->Focus();
		}
	}



	// MyForm窗体所定义的快捷键
	private: System::Void MyForm_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e) {
		if (e->KeyCode == Keys::F1) {
			button3->Focus();
			button3_Click(sender, e); //simulate keydown of button 3 上一帧
		}
		else if (e->KeyCode == Keys::F2) {
			button4->Focus();
			button4_Click(sender, e); //simulate keydown of button 4 下一帧
		}
	}


	
	// 在输入车牌号时按回车键
	private: System::Void richTextBox10_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e) {
		if (e->KeyCode == Keys::Enter) {
			// 保存改动到listview，history.log
			if (listView1->Items[SelectedRow]->SubItems[8]->Text != richTextBox10->Text) {	// 如果有改动
				listView1->Items[SelectedRow]->SubItems[8]->Text = richTextBox10->Text;	// 执行之前把当前帧的车牌号码存入listview中
				ChangedOrNot[SelectedRow] = 1;
				OutHistory();
			}
		}
	}



	// 设置当前颜色和类型为默认值
	private: System::Void button16_Click(System::Object^  sender, System::EventArgs^  e) {
		if (richTextBox7->Text != "" && richTextBox8->Text != "") {	// 如果richtextbox7和richtextbox8不为空
			// Rewrite "D:\\PointCheck+护眼版_Log\\Default.log"
			FileStream^ default = gcnew FileStream("D:\\PointCheck+护眼版_Log\\Default.log", FileMode::Create, FileAccess::Write);
			StreamWriter^ tmpDefault = gcnew StreamWriter(default, System::Text::Encoding::Unicode);
			tmpDefault->WriteLine(richTextBox7->Text);	// 第一行写颜色
			tmpDefault->WriteLine(richTextBox8->Text);  // 第二行写类型
			tmpDefault->Close();
			default->Close();
		}
	}


	
	// 点击其他、白色、黑色、蓝色、黄色
#pragma region ColorSettings
	private: System::Void button8_Click(System::Object^  sender, System::EventArgs^  e) {
		listView1->Items[SelectedRow]->SubItems[9]->Text = "黑";
		richTextBox7->Text = "黑";	// 显示颜色
		ChangedOrNot[SelectedRow] = 1;	// 标注图片改动
		OutHistory();	//检查、输出到history.log并更新listview和richTextBox	
	}
	private: System::Void button9_Click(System::Object^  sender, System::EventArgs^  e) {
		listView1->Items[SelectedRow]->SubItems[9]->Text = "白";
		richTextBox7->Text = "白";	// 显示颜色
		ChangedOrNot[SelectedRow] = 1;	// 标注图片改动
		OutHistory();	//检查、输出到history.log并更新listview和richTextBox
	}
	private: System::Void button10_Click(System::Object^  sender, System::EventArgs^  e) {
		listView1->Items[SelectedRow]->SubItems[9]->Text = "蓝";
		richTextBox7->Text = "蓝";	// 显示颜色
		ChangedOrNot[SelectedRow] = 1;	// 标注图片改动
		OutHistory();	//检查、输出到history.log并更新listview和richTextBox
	}
	private: System::Void button11_Click(System::Object^  sender, System::EventArgs^  e) {
		listView1->Items[SelectedRow]->SubItems[9]->Text = "黄";
		richTextBox7->Text = "黄";	// 显示颜色
		ChangedOrNot[SelectedRow] = 1;	// 标注图片改动
		OutHistory();	//检查、输出到history.log并更新listview和richTextBox
	}
	private: System::Void button12_Click(System::Object^  sender, System::EventArgs^  e) {
		listView1->Items[SelectedRow]->SubItems[9]->Text = "未知";
		richTextBox7->Text = "未知";	// 显示颜色
		ChangedOrNot[SelectedRow] = 1;	// 标注图片改动
		OutHistory();	//检查、输出到history.log并更新listview和richTextBox
	}
#pragma endregion



	// 点击单、双、假（虚警）
#pragma region TypeSettings
	private: System::Void button13_Click(System::Object^  sender, System::EventArgs^  e) {
		listView1->Items[SelectedRow]->SubItems[10]->Text = "单层";
		richTextBox8->Text = "单层";	// 显示类型
		ChangedOrNot[SelectedRow] = 1;	// 标注图片改动
		OutHistory();	//检查、输出到history.log并更新listview和richTextBox
	}
	private: System::Void button14_Click(System::Object^  sender, System::EventArgs^  e) {
		listView1->Items[SelectedRow]->SubItems[10]->Text = "双层";
		richTextBox8->Text = "双层";	// 显示类型
		ChangedOrNot[SelectedRow] = 1;	// 标注图片改动
		OutHistory();	//检查、输出到history.log并更新listview和richTextBox
	}
	private: System::Void button15_Click(System::Object^  sender, System::EventArgs^  e) {
		listView1->Items[SelectedRow]->SubItems[10]->Text = "假(虚警)";
		richTextBox8->Text = "假(虚警)";	// 显示类型
		ChangedOrNot[SelectedRow] = 1;	// 标注图片改动
		OutHistory();	//检查、输出到history.log并更新listview和richTextBox
	}
#pragma endregion



};// end of public ref class MyForm : public System::Windows::Forms::Form
}// end of namespace PointCheck
