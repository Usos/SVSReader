#include "openslidecpp.h"
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <windows.h>
#include <io.h>


using namespace std;
using namespace WSIAnno;
using namespace cv;



struct annoationPiece
{
	cv::Rect area;   //Annoation area position in lv0 Image
	std::string describe; //Description of the area
};

struct imageDataTrans
{
	Mat  outputImg1; //thumbnail
	Mat  outputImg2; //Raw image of the selected area
	Mat  outputImg2Temp;//image show on the ¡°Selected Area¡± Window
	Rect selectArea; //select box on thumbnail
	Rect selectArea2;//select box on the ¡°Selected Area¡± Window
	bool isDraw;  //flag of selecting area on thumbnail
	bool isShown; //flag of window ¡°Selected Area¡± is active
	bool isReDraw; //flag of being Re-select area on Real Image
	bool isAnnoating; //flag of being annoating
	OpenSlide& object;
	double thumbRate;
	vector<annoationPiece> annos;
	HWND  conHandle; //handle of console window, used to hide and show the window, used in windows only,should be replaced in ubuntu environment

	imageDataTrans(OpenSlide& t):object(t),selectArea(-1,-1,0,0), selectArea2(-1,-1,0,0),isDraw(false),isShown(false), isReDraw(false), isAnnoating(false)
	{
		outputImg1 = object.readAssoImg("thumbnail").clone();
		sizepair lv0 = object.getLvDims(0);
		sizepair thumb = object.getAssoImgDims("thumbnail");
		thumbRate = lv0 / thumb;	
		//conHandle = GetForegroundWindow();
	}
};


//FUNCS OR VARS BASED ON WINDOWS API
const sizepair maxWind(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));  //resolution of this time
void getFileList(string path, vector<string>& files, const string extension); //get *.svs file list from given folder path
//FUNCS OR VARS ABOVE SHOULD BE REPLACED IN UBUNTU

void mouseHandleThumb(int event, int x, int y, int flags, void* param);  //message handle function of thumbnail window
void drawRectByTrans(imageDataTrans &trans, Scalar color, int thickness, Scalar annoColor = Scalar(0, 255, 0)); //draw box on thumbnail window
void areaTrans(const imageDataTrans &trans, Point &tl, uint32_t &selLv, sizepair &areaO, sizepair &areaR); // coordinate transformation of different image
void realImgProc(imageDataTrans &trans);   //Preprocess of generate selected area window
void mouseHandleRealImg(int event, int x, int y, int flags, void* param); //message handle function of selected area window
void drawRectByTrans2(imageDataTrans &trans, Scalar, int); //draw box on selected area window




int main()
{
	cout << "SVS File Reader and Marker -CUI ver.- v1.0" << endl;
	cout << "Code by Lys in HEU" << endl;
	cout << "This version compiled in Aug 27, 2018 " << endl << endl;

	cout << "Please insert the path of a *.svs file or a folder contains a set of it.";
	ifstream readfile;
	ofstream outfile;
	string path, histpath;
	readfile.open(".history");
	if (readfile.is_open())
	{
		cout << endl << "If you want to recover your last work,please insert\"last\".: " << endl;
		getline(readfile,histpath);
		readfile.close();
	}
	else
	{
		cout << ":" << endl;
	}

	getline(cin,path);

	HWND conHandle = GetForegroundWindow(); //get console window's handle, used in windows only
	//bool isFolder(false);

	string testval(path.substr(path.length() - 4));
	vector<string> filelist;

	if (path == "last") //if insert "last", load the history file.
	{
		path = histpath;
	}

	if (testval != ".svs")  //if insert a folder, check how many *.svs file in this folder 
	{
		//isFolder = true;
		getFileList(path, filelist, ".svs");
		cout << "There "<< ((filelist.size() > 1) ? "are " : "is ") << filelist.size() << " *.svs file" << ((filelist.size() > 1) ? "s " : " ") << "in this folder." << endl;
	}
	else//if insert a file ,just push it
	{
		filelist.push_back(path);
	}

	//write history file
	outfile.open(".history");
	outfile << path << endl;
	outfile.close();


	OpenSlide svsfile;
	int retval;

	for (auto iter = filelist.begin(); iter != filelist.end(); iter++)
	{
		retval=svsfile.loadFile(*iter);
		if (retval != 0)
		{
			cout << *iter << "open failed!" << endl;
			continue;
		}

		imageDataTrans trans(svsfile);
		trans.conHandle = conHandle;

		annoationPiece annoTemp;
		stringstream outstr;

		//check if there have a marker file
		if (_access((*iter + ".mark").c_str(), 0) != -1)
		{
			cout << "There is a marker file for "<<iter->substr(iter->find_last_of('\\')+1)<<" exist, you can load it to (C)ontinue your last mark operation, (R)emark the image or skip this image(Press any key)?" << endl;
			cin >> testval;
			if (testval == "C" || testval == "c")
			{
				readfile.open(*iter + ".mark");
				getline(readfile, testval);
				while (!testval.empty())
				{
					outstr.clear();
					outstr.str(testval);
					outstr >> annoTemp.area.x >> annoTemp.area.y >> annoTemp.area.width >> annoTemp.area.height >> annoTemp.describe;
					trans.annos.push_back(annoTemp);
					getline(readfile, testval);
				}
				readfile.close();
			}
			else if (testval == "R" || testval == "r")
			{//Do nothing
			}
			else
				continue; // if want skip, just do it
		}

		//initialize the Thumbnail window
		namedWindow("Thumbnail", WINDOW_AUTOSIZE);
		setMouseCallback("Thumbnail", mouseHandleThumb, reinterpret_cast<void*> (&trans));
		drawRectByTrans(trans, Scalar(0, 0, 255), 3);
		
		//WINDOWS API CODE START
		ShowWindow(trans.conHandle, SW_HIDE); //hide the console window
		//WINDOWS API CODE END
		
		//main loop of the Thumbnail window
		while (1)
		{
			if (trans.isDraw) //if user is drwaing box 
				drawRectByTrans(trans, Scalar(0, 0, 255), 2);//print it out at once
			imshow("Thumbnail", trans.outputImg1);

			waitKey(10); //start the message processing 
			
			if (NULL == cvGetWindowHandle("Thumbnail")) //when thumbnail window is closed
				break; // jump out the main loop
				
			if (trans.isShown&&NULL == cvGetWindowHandle("Selected Area")) //when selected area window is closed
			{
				trans.selectArea = Rect(-1, -1, 0, 0); 
				drawRectByTrans(trans, Scalar(0, 0, 255), 3); //make the selected box on thumbnail disappear and draw the annotation box 
				trans.isShown = false; //and set the flag
			}
			else if (trans.isShown)//when selected area window is displaying
			{
				//calculate the zoom ratio of current window 
				outstr.clear();
				outstr.str("");
				//WINDOWS API CODE START 
				HWND realImgHandle = (HWND)cvGetWindowHandle("Selected Area");  //get handle of the selected area window,this function can be used in ubuntu
				RECT sizeRealImgWnd;
				GetClientRect(realImgHandle, &sizeRealImgWnd); //get client area size of the selected area window, this is a windows API, should be replaced in Ubuntu environment
				//WINDOWS API CODE END
				double realW = sizeRealImgWnd.right - sizeRealImgWnd.left;
				double realH = sizeRealImgWnd.bottom - sizeRealImgWnd.top;

				int rawW = trans.outputImg2.cols;
				int rawH = trans.outputImg2.rows;
				
				//set parameter and print the text on image
				outstr << "Zoom Ratio: W-" << realW / rawW << " H-" << realH / rawH;
				int font_face = FONT_HERSHEY_COMPLEX;
				double font_scale = 0.5;
				int baseline;
				Size text_size = getTextSize(outstr.str(), font_face, font_scale, 1, &baseline);
				Point origin(0, text_size.height + 10);
				Mat realImg = trans.outputImg2Temp;
				putText(realImg, outstr.str(), origin * 4, font_face, font_scale, Scalar(255, 0, 0), 1);
				
				//if change the selected area, draw a red box, if annotating, draw a green box
				if (trans.isReDraw)
					drawRectByTrans2(trans, Scalar(0, 0, 255), 2);
				else if (trans.isAnnoating)
					drawRectByTrans2(trans, Scalar(0, 255, 0), 2);

				imshow("Selected Area", realImg);

			}
		}
		//WINDOWS API CODE START
		ShowWindow(trans.conHandle, SW_SHOW);//display the console window
		SetForegroundWindow(trans.conHandle);
		//WINDOWS API CODE END
		cvDestroyAllWindows();  //destory selected area window if not closed

		//save marker file
		outfile.open(*iter + ".mark");
		for (auto iter2 = trans.annos.begin(); iter2 != trans.annos.end(); iter2++)
		{
			outfile << iter2->area.x << "\t" << iter2->area.y << "\t" << iter2->area.width << "\t" << iter2->area.height << "\t" << iter2->describe << endl;
		}
		outfile.close();
			
	}


	//system("pause"); //Used to stop the program before end in debug stage
	return 0;
}

void mouseHandleThumb(int event, int x, int y, int flags, void* param)
{
	imageDataTrans& trans = *reinterpret_cast<imageDataTrans*>(param);
	//static thread realImg(bind(realImgProc, trans));
	//static bool runThd(false);
	switch (event)
	{
	case EVENT_MOUSEMOVE:
		if (trans.isDraw)
		{

			trans.selectArea.width = x - trans.selectArea.x;
			trans.selectArea.height = y - trans.selectArea.y;
		}
		break;
	case EVENT_LBUTTONDOWN:
		trans.isDraw = true;
		//runThd = true;
		trans.selectArea=Rect(x, y, 0, 0);
		break;
	case EVENT_LBUTTONUP:
		if (trans.selectArea.width < 0)
		{
			trans.selectArea.x += trans.selectArea.width;
			trans.selectArea.width *= -1;
		}
		if (trans.selectArea.height < 0)
		{
			trans.selectArea.y += trans.selectArea.height;
			trans.selectArea.height *= -1;
		}
		drawRectByTrans(trans, Scalar(0, 0, 255), 2);
		trans.isDraw = false;
		if (trans.selectArea.area() != 0)
		{
			realImgProc(trans);
		}
		break;
	default:
		break;
	}
}

void drawRectByTrans(imageDataTrans &trans, Scalar color, int thickness,Scalar annoColor)
{
	trans.outputImg1= trans.object.readAssoImg("thumbnail").clone();
	rectangle(trans.outputImg1, trans.selectArea, color,thickness);

	stringstream outstr;
    
	string textDSR("Thumbnail Downsample Ratio: ");

	outstr << textDSR<< trans.thumbRate;
	getline(outstr, textDSR);

	
	int font_face = FONT_HERSHEY_COMPLEX;
	double font_scale = 0.5;
	int baseline;

	Size text_size = getTextSize(textDSR, font_face, font_scale, 1, &baseline);
	Point origin(0, text_size.height);

	putText(trans.outputImg1, textDSR, origin, font_face, font_scale, Scalar(255, 0, 0));

	Rect annoBox;

	if (!trans.annos.empty())
	{
		for (auto iter = trans.annos.begin(); iter != trans.annos.end(); iter++)
		{
			annoBox = iter->area;
			annoBox.x /= trans.thumbRate;
			annoBox.y /= trans.thumbRate;
			annoBox.width /= trans.thumbRate;
			annoBox.height /= trans.thumbRate;
			rectangle(trans.outputImg1, annoBox, annoColor, 1);
		}
	}


	string pos;
	if ((trans.isDraw||trans.isShown)&&trans.selectArea.area()!=0)
	{
		outstr.clear();
		outstr.str("");
		outstr << trans.selectArea.tl() << "->" << trans.selectArea.tl()*trans.thumbRate;
		putText(trans.outputImg1, outstr.str(), trans.selectArea.tl()+Point(0,-10), font_face, font_scale, Scalar(0, 0, 255));
		outstr.clear();
		outstr.str("");
		outstr << trans.selectArea.br() << "->" << trans.selectArea.br()*trans.thumbRate;
		putText(trans.outputImg1, outstr.str(), trans.selectArea.br()+Point(0, text_size.height), font_face, font_scale, Scalar(0, 0, 255));
	}

}

void areaTrans(const imageDataTrans & trans, Point & tl, uint32_t &selLv, sizepair & areaO, sizepair &areaR)
{
	sizepair areaT(trans.selectArea.width, trans.selectArea.height);
	areaR = areaT * trans.thumbRate;                     //area in level0
	selLv = trans.object.getBestLv(areaR / maxWind);
	areaO = areaR / trans.object.getLvDownSample(selLv);//area in best level
	tl = trans.selectArea.tl()*trans.thumbRate; //top-left point of selected area
}

void realImgProc(imageDataTrans & trans)
{
	
	sizepair areaR,areaO;
	uint32_t selLv;
	Point tl;
	areaTrans(trans, tl, selLv, areaO, areaR);
	Point br = trans.selectArea.br()*trans.thumbRate;

	int wndWidth(areaO.width()), wndHeight(areaO.height());
	if (areaO > maxWind)
	{
		if (areaO.height() > areaO.width())
		{
			wndHeight = maxWind.height() *0.6;
			wndWidth = areaO.width()*((double)wndHeight / areaO.height());
		}
		else
		{
			wndWidth = maxWind.width() *0.6;
			wndHeight = areaO.height()*((double)wndWidth / areaO.width());
		}

	}


	Mat realImg = trans.object.readRegion(tl, selLv, areaO).clone();
	trans.outputImg2 = realImg;

	stringstream outstr;
	outstr << "Selected Area: " << tl << "->" << br;
	int font_face = FONT_HERSHEY_COMPLEX;
	double font_scale = 0.5;
	int baseline;
	Size text_size = getTextSize(outstr.str(), font_face, font_scale, 1, &baseline);
	Point origin(0, text_size.height+10);
	putText(realImg, outstr.str(), origin, font_face, font_scale, Scalar(255, 0, 0),1);
	outstr.str("");
	outstr << "Selected Level: " << selLv;
	putText(realImg, outstr.str(), origin * 2, font_face, font_scale, Scalar(255, 0, 0),1);
	outstr.str("");
	outstr << "Downsample Ratio of Selected Level: " << trans.object.getLvDownSample(selLv);
	putText(realImg, outstr.str(), origin * 3, font_face, font_scale, Scalar(255, 0, 0),1);

	namedWindow("Selected Area", WINDOW_NORMAL);
	setMouseCallback("Selected Area", mouseHandleRealImg, reinterpret_cast<void*> (&trans));

	trans.outputImg2Temp = trans.outputImg2.clone();
	trans.selectArea2 = Rect(-1, -1, 0, 0);


	cvResizeWindow("Selected Area",wndWidth,wndHeight);
	imshow("Selected Area", trans.outputImg2Temp);
	trans.isShown = true;

	//waitKey(0);
	
}

void mouseHandleRealImg(int event, int x, int y, int flags, void * param)
{
	imageDataTrans& trans = *reinterpret_cast<imageDataTrans*>(param);
	sizepair areaR, areaO;
	uint32_t selLv;
	Point tl;
	areaTrans(trans, tl, selLv, areaO, areaR);
	Point br = trans.selectArea.br()*trans.thumbRate;

	//coordinate transformation

	int rawW = trans.outputImg2.cols;
	int rawH = trans.outputImg2.rows;

	Point tlReal = trans.selectArea2.tl()*trans.object.getLvDownSample(selLv) + trans.selectArea.tl()*trans.thumbRate;
	Point brReal = trans.selectArea2.br()*trans.object.getLvDownSample(selLv) + trans.selectArea.tl()*trans.thumbRate;

	annoationPiece annoTemp;

	switch (event)
	{
	case EVENT_MOUSEMOVE:
		if (trans.isReDraw||trans.isAnnoating)
		{

			trans.selectArea2.width = x - trans.selectArea2.x;
			trans.selectArea2.height = y - trans.selectArea2.y;
		}

		//cout << "[" << x << "," << y << "]" << endl;
		break;
	case EVENT_LBUTTONDOWN:
		trans.isReDraw = true;

		trans.selectArea2 = Rect(x, y, 0, 0);
		break;
	case EVENT_LBUTTONUP:
		if (trans.selectArea2.width < 0)
		{
			trans.selectArea2.x += trans.selectArea2.width;
			trans.selectArea2.width *= -1;
		}
		if (trans.selectArea2.height < 0)
		{
			trans.selectArea2.y += trans.selectArea2.height;
			trans.selectArea2.height *= -1;
		}

		trans.isReDraw = false;
		if (trans.selectArea2.area() != 0)
		{
			drawRectByTrans2(trans, Scalar(0, 0, 255), 2);
			trans.selectArea = Rect(tlReal / trans.thumbRate, brReal / trans.thumbRate);
			drawRectByTrans(trans, Scalar(0, 0, 255), 2);

			realImgProc(trans);
		}

		break;
	case EVENT_RBUTTONDOWN:
		trans.isAnnoating = true;
		trans.selectArea2 = Rect(x, y, 0, 0);
		break;
	case EVENT_RBUTTONUP:
		if (trans.selectArea2.width < 0)
		{
			trans.selectArea2.x += trans.selectArea2.width;
			trans.selectArea2.width *= -1;
		}
		if (trans.selectArea2.height < 0)
		{
			trans.selectArea2.y += trans.selectArea2.height;
			trans.selectArea2.height *= -1;
		}

		if (trans.selectArea2.area() != 0)
		{
			drawRectByTrans2(trans, Scalar(0, 255, 0), 2);
			//WINDOWS API CODE START
			HWND thiswnd = GetForegroundWindow();
			ShowWindow(trans.conHandle, SW_SHOW);
			SetForegroundWindow(trans.conHandle);
			//WINDOWS API CODE END
			annoTemp.area = Rect(tlReal, brReal);
			cout << "Please insert description of area" <<annoTemp.area<<" :"<< endl;
			cin >> annoTemp.describe;
			trans.annos.push_back(annoTemp);
			//WINDOWS API CODE START
			SetForegroundWindow(thiswnd);
			ShowWindow(trans.conHandle, SW_HIDE);
			//WINDOWS API CODE END
			
		}
		trans.isAnnoating = false;
		break;


	default:
		break;
	}






}

void drawRectByTrans2(imageDataTrans &trans, Scalar color, int thickness)
{
	trans.outputImg2Temp = trans.outputImg2.clone();
	rectangle(trans.outputImg2Temp, trans.selectArea2, color, thickness);
	sizepair areaR, areaO;
	uint32_t selLv;
	Point tl;
	areaTrans(trans, tl, selLv, areaO, areaR);

	stringstream outstr;

	int font_face = FONT_HERSHEY_COMPLEX;
	double font_scale = 0.5;
	int baseline;
	outstr << "test";
	Size text_size = getTextSize(outstr.str(), font_face, font_scale, 1, &baseline);

	string pos;
	if ((trans.isReDraw || trans.isAnnoating) && trans.selectArea2.area() != 0)
	{
		outstr.clear();
		outstr.str("");
		outstr << trans.selectArea2.tl()*trans.object.getLvDownSample(selLv)+trans.selectArea.tl()*trans.thumbRate;
		putText(trans.outputImg2Temp, outstr.str(), trans.selectArea2.tl() + Point(0, -10), font_face, font_scale, color);
		outstr.clear();
		outstr.str("");
		outstr <<  trans.selectArea2.br()*trans.object.getLvDownSample(selLv) + trans.selectArea.tl()*trans.thumbRate;
		putText(trans.outputImg2Temp, outstr.str(), trans.selectArea2.br() + Point(0, text_size.height), font_face, font_scale, color);
	}
}

void getFileList(string path, vector<string>& files, const string extension)
{
	_finddata_t file;
	string tempFileName;

	path += "\\*";

	intptr_t lf(0);

	if ((lf = _findfirst(path.c_str(), &file)) == -1) 
	{
		cout << path << " not found!!!" << endl;
	}
	else 
	{
		path.pop_back();
		while (_findnext(lf, &file) == 0) 
		{

			if (strcmp(file.name, ".") == 0 || strcmp(file.name, "..") == 0)
				continue;
			
			tempFileName = file.name;
			if (tempFileName.substr(tempFileName.length() - 4) == ".svs")
			{
				files.push_back(path + file.name);
			}
				
		}
	}
	_findclose(lf);
}

