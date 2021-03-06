
#include "MprojectDefine.h"

#include "gdal_priv.h"
#include "gdal.h"
#include "ogr_srs_api.h"
#include "cpl_string.h"
#include "cpl_conv.h"
#include "cpl_multiproc.h"
#include "ogrsf_frmts.h"
#include <math.h>

#include <fstream>
#include <ossim\imaging\ossimImageRenderer.h>
#include <ossim\imaging\ossimGeoPolyCutter.h>
#include <ossim\parallel\ossimMultiThreadSequencer.h>
using namespace std;
using namespace mylib;
#pragma comment(lib, "mlpack.lib")

MyProject::MyProject()
{  m_ModelType=OrbitType;//默认卫星轨道模型
	m_ProjFileName="MyProjection.prj";     //工程文件名
	m_ImgFileNameUnc="";     //待校正影像文件名
	m_ImgFileNameRef="";     //参考影像文件名
	m_ImgFileNameout="";    //校正结果影像文件名
	m_SensorName=modelLandsat5;    //默认 传感器名称为Landsat5
	m_SampleType="BILINEAR";        //1125
	m_FileOutType="tiff";					//1125
//m_PolyDegree="1 x y x2 xy y2 x3 y3 xy2 x2y z xz yz";
m_PolyDegree="1 x y x2 xy y2 x3 y3 xy2 x2y";

	m_ImgFileNameUncHistogram=""; //1124
	m_ImgFileNameRefHistogram="";//1124
	m_RefBandList.push_back(0);
	m_RefBandList.push_back(1);
	m_RefBandList.push_back(2);

	m_UncBandList.push_back(1);
	m_UncBandList.push_back(2);
	m_UncBandList.push_back(3);

	m_OutBandList.clear();

	// m_MapPar=NULL;       //投影名称（包含投影参数和输出分辨率）
	m_DemPath="";           //DEM路径
	m_HomoPointsFilename="";  //同名点文件路径

	m_CtrlGptSet=new ossimTieGptSet;  //用于控制点选取界面中 控制点的保存
	m_ChkGptSet=new ossimTieGptSet;   //用于控制点选取界面中 检查点的保存
	m_OptCtrlGptSet=new ossimTieGptSet;//用于控制点优化界面中 控制点的保存
	m_OptChkGptSet=new ossimTieGptSet; //用于控制点优化界面中 检查点的保存
	m_OptRejectGptSet=new ossimTieGptSet;//用于控制点优化界面中 剔除点的保存

	m_MapPar = NULL;
	theMgr = NULL;
	m_progress = NULL;

	geom.clear();				//记录优化后 轨道模型参数信息
	m_MapProjection.clear();    //结果文件投影信息，应与控制点一致


	starline = 0;   ///以下四个变量表明子区大小，默认为全图
	starpixel = 0;
	endpixel = 0;
	endline = 0;


}
MyProject::~MyProject()
{
	//if (m_BandSelectUnc) delete m_BandSelectUnc;
	//if (m_BandSelectRef) delete m_BandSelectRef;

	geom.clear();
	m_MapProjection.clear();
	m_UncBandList.clear();
	m_RefBandList.clear();
	m_CtrlGptSet->clearTiePoints();
	m_ChkGptSet->clearTiePoints();
	m_OptCtrlGptSet->clearTiePoints();
	m_OptRejectGptSet->clearTiePoints();

	starline = 0;   ///以下四个变量表明子区大小，默认为全图
	starpixel = 0;
	endpixel = 0;
	endline = 0;

}
SensorType MyProject::getSensorType(ossimFilename imgFileName)
{
	ossimString strFile = imgFileName.file();
	ossimString strPath = imgFileName.path();
	
	if(ossimFilename(imgFileName).isDir())
	{//文件夹
		//QStringList tmpFiles;
		//QFindFile(imgFileName.c_str(), QStringList("HDR-ALAV2*_O1B2*.txt"), tmpFiles);
		//if(tmpFiles.size() > 0)
		//	return modelAlosAVNIR2_1B2;

		//tmpFiles.clear();
		//QFindFile(imgFileName.c_str(), QStringList("HDR-ALPSMW*_O1B2*.txt"), tmpFiles);
		//if(tmpFiles.size() > 0)
		//	return modelAlosPRISM_1B2;
	}
	else
	{
		// landsat
		if(0 == _stricmp(strFile.c_str(),"header.dat"))
			return modelLandsat5;
		if(strFile.upcase().contains("_HRF.FST") || strFile.upcase().contains("_HPN.FST"))
			return modelLandsat7;

		// SPOT and Theos
		ossimFilename dimFilename = strPath + "\\metadata.dim";
		if (dimFilename.exists())
		{
			fstream inf;
			inf.open(dimFilename.c_str(), ios_base::in);
			int i = 0;
			while(i < 10)
			{
				string strtmp;
				getline(inf,strtmp);
				ossimString str(strtmp.c_str());
				if(str.upcase().contains("THEOS1_1A_SCENE"))
				{
					// Theos
					inf.close();
					return modelTheos;
				}
				else if(str.upcase().contains("SPOTSCENE_1A"))
				{
					// Spot
					inf.close();
					return modelSpot5;
				}
			}
			inf.close();
		}

		// HJ1
		ossimFilename descFilename = strPath + "\\desc.xml";
		if (descFilename.exists())
		{
			fstream inf;
			inf.open(descFilename.c_str(), ios_base::in);
			int i = 0;
			while(i < 10)
			{
				string strtmp;
				getline(inf,strtmp);
				ossimString str(strtmp.c_str());
				if(str.upcase().contains("HJ-1A"))
				{
					inf.close();
					return modelHJ1;
				}
				else if(str.upcase().contains("HJ-1B"))
				{
					inf.close();
					return modelHJ1;
				}
			}
			inf.close();
		}

		//// ALOS
		//QStringList tmpFiles;
		//QFindFile(strPath, tmpFiles, QStringList("HDR-ALAV2*_O1B2*.txt"));
		//if(tmpFiles.size() > 0)
		//	return modelAlosAVNIR2_1B2;
		//tmpFiles.clear();
		//QFindFile(strPath, tmpFiles, QStringList("HDR-ALPSMW*_O1B2*.txt"));
		//if(tmpFiles.size() > 0)
		//	return modelAlosPRISM_1B2;
		
	}
	return UnknowMole;
}
ModelType MyProject::getModelType()
{
		return m_ModelType;	
}

void MyProject::ReadGcpAndProjection(ossimFilename strFilename,ossimTieGptSet* &m_gptset,ossimTieGptSet* &mcheck_gptset)
{
	ossimGpt tieg;
	ossimDpt tied;
	ossimTieGpt *aTiePt;
	m_gptset->clearTiePoints();
	mcheck_gptset->clearTiePoints();


	//////////////////////1116加入控制点的坐标系////////////
	ossimTempFilename pp;
	bool flag=true;
	ossimString str;

	char strtmp[255];
	std::vector<ossimString> gcpcon;
	const char* ellipse_code;
	pp.generateRandomFile();

	fstream os;
	os.open(strFilename.c_str(), ios::in);
	fstream oss;
	oss.open(pp.c_str(), ios::out|ios_base::app);
	os>>str;
	while(os.getline(strtmp,255) ) {
		str=strtmp;
		oss<< str.c_str();oss<<endl;
		if(str.contains("MAP_PROJECTION_END")) {flag=false;break;}

	}
	oss.close();

	if (!flag) {////////判断控制点文件中是否有投影信息
		m_MapProjection.clear();
		m_MapPar=NULL;
		m_MapProjection.addFile(pp.c_str());
		m_MapPar=PTR_CAST(ossimMapProjection,
			ossimMapProjectionFactory::instance()->createProjection(m_MapProjection));
		ellipse_code=m_MapProjection.find(ossimKeywordNames::DATUM_KW);

	}
	else
	{
		if(m_MapPar ==NULL)  {
			//wxMessageBox(wxT("请您先设置投影信息！"));
			return;
		}

		ellipse_code=m_MapProjection.find(ossimKeywordNames::DATUM_KW);
		os.clear();
		//os.seekg(0,ios::beg);
		os.close();
		os.open(strFilename.c_str());

	}
	mcheck_gptset->clearTiePoints();
	m_gptset->clearTiePoints();
	while(os.getline(strtmp,255))
	{
		int star;
		str=strtmp;
		if (str.empty()) continue;
		gcpcon=str.split("	", true);
		if (gcpcon.size()==1) {
			str=gcpcon[0];
			gcpcon=str.split(" ", true);
		}
		if (gcpcon.size()<5) continue;
		aTiePt=new ossimTieGpt(tieg,tied,0);
		if(gcpcon[0].empty()) {
			aTiePt->GcpNumberID=gcpcon[1];
			star=1;
		}
		else
		{
			aTiePt->GcpNumberID=gcpcon[0];
			star=0;
		}

		tied.x=gcpcon[star+1].toDouble();tied.y=gcpcon[star+2].toDouble();
		tieg.lat=gcpcon[star+3].toDouble();tieg.lon=gcpcon[star+4].toDouble();
		tieg.hgt=gcpcon[star+5].toDouble();
		tieg.datum(ossimDatumFactory::instance()->create(ossimString(ellipse_code))); 

		aTiePt->setGroundPoint(tieg);
		aTiePt->setImagePoint(tied);
		if (0 == aTiePt->GcpNumberID.substr(0,1).compare("-"))
		{
			m_gptset->addTiePoint(aTiePt);
		}
		else
		{
			aTiePt->GcpNumberID = ossimSAfterFirst(aTiePt->GcpNumberID, '-');
			mcheck_gptset->addTiePoint(aTiePt);
		}

	}
	os.close();

}

void MyProject::ReadGcpAndProjection(ossimFilename strFilename)
{
	readGcpFile(strFilename, m_CtrlGptSet, m_ChkGptSet, &m_MapProjection);
	m_MapPar=PTR_CAST(ossimMapProjection,
		ossimMapProjectionFactory::instance()->createProjection(m_MapProjection));
	return;

	ossimGpt tieg;
	ossimDpt tied;
	ossimTieGpt *aTiePt;
	m_CtrlGptSet->clearTiePoints();
	m_ChkGptSet->clearTiePoints();

	//////////////////////1116加入控制点的坐标系////////////
	ossimTempFilename pp;
	bool flag=true;
	ossimString str;

	char strtmp[255];
	std::vector<ossimString> gcpcon;
	const char* ellipse_code;
	pp.generateRandomFile();

	fstream os;
	os.open(strFilename.c_str(), ios_base::in);
	if(!os) return;
	fstream oss;
	oss.open(pp.c_str(),ios::out|ios_base::app);
	os>>str;
	while(os.getline(strtmp,255) ) {
		str=strtmp;
		oss<< str.c_str();oss<<endl;
		if(str.contains("MAP_PROJECTION_END")) {flag=false;break;}

	}
	oss.close();

	if (!flag) {////////判断控制点文件中是否有投影信息
		m_MapProjection.clear();
		m_MapPar=NULL;
		m_MapProjection.addFile(pp.c_str());
		m_MapPar=PTR_CAST(ossimMapProjection,
			ossimMapProjectionFactory::instance()->createProjection(m_MapProjection));
		ellipse_code=m_MapProjection.find(ossimKeywordNames::DATUM_KW);

	}
	else
	{
		if(m_MapPar ==NULL)  {
			//wxMessageBox(wxT("请您先设置投影信息！"));
			//return;

		}

		ellipse_code=m_MapProjection.find(ossimKeywordNames::DATUM_KW);
		os.clear();
		//os.seekg(0,ios::beg);
		os.close();
		os.open(strFilename.c_str());

	}
	while(os.getline(strtmp,255))
	{
		int star;
		str=strtmp;
		if (str.empty()) continue;
		gcpcon=str.split("	", true);
		if (gcpcon.size()==1) {
			str=gcpcon[0];
			gcpcon=str.split(" ", true);
		}
		if (gcpcon.size()<5) continue;
		aTiePt=new ossimTieGpt(tieg,tied,0);
		if(gcpcon[0].empty()) {
			aTiePt->GcpNumberID=gcpcon[1];
			star=1;
		}
		else
		{
			aTiePt->GcpNumberID=gcpcon[0];
			star=0;
		}

		tied.x=gcpcon[star+1].toDouble();tied.y=gcpcon[star+2].toDouble();
		tieg.lat=gcpcon[star+3].toDouble();tieg.lon=gcpcon[star+4].toDouble();
		tieg.hgt=gcpcon[star+5].toDouble();
		tieg.datum(ossimDatumFactory::instance()->create(ossimString(ellipse_code))); 

		aTiePt->setGroundPoint(tieg);
		aTiePt->setImagePoint(tied);
		if (0 != aTiePt->GcpNumberID.substr(0, 1).compare("-")) m_CtrlGptSet->addTiePoint(aTiePt);
		else
		{
			aTiePt->GcpNumberID = aTiePt->GcpNumberID.after("-");
			m_ChkGptSet->addTiePoint(aTiePt);
		}
	}
	os.close();
}



/*
void MyProject::ReadLineAndProjection(ossimFilename strFilename, vector < ossimTieLine > &tieLineList)
{
	ossimGpt tieg;
	ossimDpt tied;
	wxString index;
	tieLineList.clear();

	//////////////////////1116加入控制点的坐标系////////////
	ossimTempFilename pp;
	bool flag=true;
	ossimString str;

	char strtmp[255];
	std::vector<ossimString> gcpcon;
	const char* ellipse_code;
	pp.generateRandomFile();

	ifstream os(strFilename.c_str());
	ofstream oss(pp.c_str(),ios::out|ios_base::app);
	os>>str;
	while(os.getline(strtmp,255) ) {
		str=strtmp;
		oss<< str.c_str();oss<<endl;
		if(str.contains("MAP_PROJECTION_END")) {flag=false;break;}

	}
	oss.close();

	if (!flag) {////////判断控制点文件中是否有投影信息
		m_MapProjection.clear();
		m_MapPar=NULL;
		m_MapProjection.addFile(pp.c_str());
		m_MapPar=PTR_CAST(ossimMapProjection,
			ossimMapProjectionFactory::instance()->createProjection(m_MapProjection));
		ellipse_code=m_MapProjection.find(ossimKeywordNames::DATUM_KW);

	}
	else
	{
		if(m_MapPar ==NULL)  {
			//wxMessageBox(wxT("请您先设置投影信息！"));
			return;

		}

		ellipse_code=m_MapProjection.find(ossimKeywordNames::DATUM_KW);
		os.clear();
		//os.seekg(0,ios::beg);
		os.close();
		os.open(strFilename.c_str());

	}

	while(os.getline(strtmp,255))
	{
		int star;

		ossimTieLine tieLine;
		str=strtmp;
		if (str.empty()) continue;
		gcpcon=str.split("	");
		if (gcpcon.size()==1) {
			str=gcpcon[0];
			gcpcon=str.split(" ");
		}
		if (gcpcon.size() < 5) continue;
		//tieLine.first = new ossimTieGpt(tieg,tied,0);
		//tieLine.second = new ossimTieGpt(tieg,tied,0);
		if(gcpcon[0].empty()) {
			tieLine.GcpNumberID = gcpcon[1];
			star=1;
		}
		else
		{
			tieLine.GcpNumberID = gcpcon[0];
			star=0;
		}

		int pos = star + 1;
		int num = atoi(gcpcon[pos++]);
		int i,j;
		vector<ossimDpt> dptSet;
		vector<ossimGpt> gptSet;
		for(i = 0;i < num;i++)
		{
			tied.x=gcpcon[pos++].toDouble();
			tied.y=gcpcon[pos++].toDouble();
			dptSet.push_back(tied);
		}
		num = atoi(gcpcon[pos++]);
		for(i = 0;i < num;i++)
		{
			tieg.lat=gcpcon[pos++].toDouble();
			tieg.lon=gcpcon[pos++].toDouble();
			tieg.hgt=gcpcon[pos++].toDouble();
			gptSet.push_back(tieg);
		}

		double xmin, xmax, ymin, ymax;
		//像素坐标
		if(dptSet.size() > 2)
		{// 如果多余两个点，则拟合直线
			//regression of the line from dpts
			double slope, intercept;
			ossim2dLinearRegression theLineFit;
			theLineFit.clear();
			for(j = 0;j < (int)dptSet.size();j++)
				theLineFit.addPoint(dptSet[j]);
			theLineFit.solve();
			theLineFit.getEquation(slope, intercept);
			xmax = dptSet[0].x;
			xmin = xmax;
			ymax = dptSet[0].y;
			ymin = ymax;
			ossimDpt dpt1, dpt2;
			for(j = 0;j < (int)dptSet.size();j++)
			{
				xmax = xmax >= dptSet[j].x ? xmax : dptSet[j].x;
				xmin = xmin <= dptSet[j].x ? xmin : dptSet[j].x;
				ymax = ymax >= dptSet[j].y ? ymax : dptSet[j].y;
				ymin = ymin <= dptSet[j].y ? ymin : dptSet[j].y;
			}
			if((xmax - xmin) >= (ymax - ymin))
			{
				dpt1 = ossimDpt(xmin, slope * xmin + intercept);
				dpt2 = ossimDpt(xmax, slope * xmax + intercept);
			}
			else
			{
				dpt1 = ossimDpt((ymin - intercept) / slope, ymin);
				dpt2 = ossimDpt((ymax - intercept) / slope, ymax);
			}

			tieLine.first.setImagePoint(dpt1);
			tieLine.second.setImagePoint(dpt2);
		}
		else if(2 == dptSet.size())
		{//如果是2个点
			tieLine.first.setImagePoint(dptSet[0]);
			tieLine.second.setImagePoint(dptSet[1]);
		}
		else
		{
			continue;
		}

		// 大地坐标

		if(gptSet.size() > 2)
		{// 如果多余两个点，则拟合直线
			//regression of the line from gpts
			double slopeY, interceptY, slopeZ, interceptZ;
			ossim2dLinearRegression theLineFitY;
			ossim2dLinearRegression theLineFitZ;
			theLineFitY.clear();
			theLineFitZ.clear();
			for(j = 0;j < (int)gptSet.size();j++)
			{
				theLineFitY.addPoint(ossimDpt(gptSet[j].lat, gptSet[j].lon));
				theLineFitZ.addPoint(ossimDpt(gptSet[j].lat, gptSet[j].hgt));
			}
			theLineFitY.solve();
			theLineFitZ.solve();
			theLineFitY.getEquation(slopeY, interceptY);
			theLineFitZ.getEquation(slopeZ, interceptZ);
			xmax = gptSet[0].lat;
			xmin = xmax;
			ymax = gptSet[0].lon;
			ymin = ymax;
			ossimGpt newGpt1, newGpt2;
			for(j = 0;j < (int)gptSet.size();j++)
			{
				xmax = xmax >= gptSet[j].lat ? xmax : gptSet[j].lat;
				xmin = xmin <= gptSet[j].lat ? xmin : gptSet[j].lat;
				ymax = ymax >= gptSet[j].lon ? ymax : gptSet[j].lon;
				ymin = ymin <= gptSet[j].lon ? ymin : gptSet[j].lon;
			}
			if((xmax - xmin) >= (ymax - ymin))
			{
				newGpt1 = ossimGpt(xmin, slopeY * xmin + interceptY, slopeZ * xmin + interceptZ);
				newGpt2 = ossimGpt(xmax, slopeY * xmax + interceptY, slopeZ * xmin + interceptZ);
			}
			else
			{
				newGpt1 = ossimGpt((ymin - interceptY) / slopeY, ymin, slopeZ * (ymin - interceptY) / slopeY + interceptZ);
				newGpt2 = ossimGpt((ymax - interceptY) / slopeY, ymax, slopeZ * (ymax - interceptY) / slopeY + interceptZ);
			}

			tieLine.first.setGroundPoint(newGpt1);
			tieLine.second.setGroundPoint(newGpt2);

		}
		else if(2 == gptSet.size())
		{//如果是2个点
			tieLine.first.setGroundPoint(gptSet[0]);
			tieLine.second.setGroundPoint(gptSet[1]);
		}
		else
		{
			continue;
		}
		tieLineList.push_back(tieLine);
	}
	os.close();
}


*/

void MyProject::ReadImagePoints(ossimFilename strFilename, vector < ossimDFeature > & dptPointList)
{
	char strtmp[1024];
	std::vector<ossimString> gcpcon;
	ossimString str;

	fstream os;
	os.open(strFilename.c_str(), ios_base::in);
	while(os.getline(strtmp,1024))
	{
		//int star;
		str=strtmp;
		if (str.empty()) continue;
		gcpcon=str.split("	");
		if (gcpcon.size()==1) {
			str=gcpcon[0];
			gcpcon=str.split(" ");
		}
		if (gcpcon.size()<3) continue;
		ossimDFeature dptPoint(ossimFeatureType::ossimPointType);
		dptPoint.strId = gcpcon[0];
		int pos = 0;
		if(gcpcon[0].empty()) {
			dptPoint.strId=gcpcon[1];
			pos += 2;
		}
		else
		{
			dptPoint.strId=gcpcon[0];
			pos++;
		}
		int num = gcpcon[pos++].toInt();
		if((int)gcpcon.size() < num * 2 + pos)
		{
			continue;
		}
		for(int i = 0;i < num;i++)
		{
			ossimDpt pt;
			pt.x = gcpcon[pos++].toDouble();
			pt.y = gcpcon[pos++].toDouble();
			dptPoint.m_Points.push_back(pt);
		}
		dptPointList.push_back(dptPoint);
	}
}
void MyProject::ReadGroundPoints(ossimFilename strFilename, vector < ossimGFeature > & gptPointList)
{
	char strtmp[1024];
	std::vector<ossimString> gcpcon;
	ossimString str;

	fstream os;
	os.open(strFilename.c_str(), ios_base::in);
	while(os.getline(strtmp,1024))
	{
		//int star;
		str=strtmp;
		if (str.empty()) continue;
		gcpcon=str.split("	");
		if (gcpcon.size()==1) {
			str=gcpcon[0];
			gcpcon=str.split(" ");
		}
		if (gcpcon.size()<4) continue;
		ossimGFeature gptPoint(ossimFeatureType::ossimPointType);
		gptPoint.strId = gcpcon[0];
		int pos = 0;
		if(gcpcon[0].empty()) {
			gptPoint.strId=gcpcon[1];
			pos += 2;
		}
		else
		{
			gptPoint.strId=gcpcon[0];
			pos++;
		}
		int num = gcpcon[pos++].toInt();
		if((int)gcpcon.size() < num * 3 + pos)
		{
			continue;
		}
		for(int i = 0;i < num;i++)
		{
			ossimGpt pt;
			pt.lat = gcpcon[pos++].toDouble();
			pt.lon = gcpcon[pos++].toDouble();
			pt.hgt = gcpcon[pos++].toDouble();
			gptPoint.m_Points.push_back(pt);
		}
		gptPointList.push_back(gptPoint);
	}
}

//void MyProject::ReadImageLines(ossimFilename strFilename, vector < ossimDFeature > & dptLineList)
//{
//	char strtmp[1024];
//	std::vector<ossimString> gcpcon;
//	ossimString str;
//
//	ifstream os(strFilename.c_str());
//	while(os.getline(strtmp,1024))
//	{
//		//int star;
//		str=strtmp;
//		if (str.empty()) continue;
//		gcpcon=str.split("	");
//		if (gcpcon.size()==1) {
//			str=gcpcon[0];
//			gcpcon=str.split(" ");
//		}
//		if (gcpcon.size()<5) continue;
//
//		ossimDFeature dptLine(ossimDFeature::ossimDFeatureType::ossimDLineType);
//		vector<ossimDpt> dptSet;
//		dptLine.strId = gcpcon[0];
//		int pos = 0;
//		if(gcpcon[0].empty()) {
//			dptLine.strId=gcpcon[1];
//			pos += 2;
//		}
//		else
//		{
//			dptLine.strId=gcpcon[0];
//			pos++;
//		}
//		int num = gcpcon[pos++].toInt();
//		if((int)gcpcon.size() < num * 2 + pos)
//		{
//			continue;
//		}
//		for(int i = 0;i < num;i++)
//		{
//			ossimDpt pt;
//			pt.x = gcpcon[pos++].toDouble();
//			pt.y = gcpcon[pos++].toDouble();
//			dptSet.push_back(pt);
//		}
//
//		double xmin, xmax, ymin, ymax;
//		if(dptSet.size() > 2)
//		{// 如果多余两个点，则拟合直线
//			//regression of the line from dpts
//			double slope, intercept;
//			ossim2dLinearRegression theLineFit;
//			theLineFit.clear();
//			for(unsigned int j = 0;j < (int)dptSet.size();j++)
//				theLineFit.addPoint(dptSet[j]);
//			theLineFit.solve();
//			theLineFit.getEquation(slope, intercept);
//			xmax = dptSet[0].x;
//			xmin = xmax;
//			ymax = dptSet[0].y;
//			ymin = ymax;
//			ossimDpt dpt1, dpt2;
//			for(unsigned int j = 0;j < (int)dptSet.size();j++)
//			{
//				xmax = xmax >= dptSet[j].x ? xmax : dptSet[j].x;
//				xmin = xmin <= dptSet[j].x ? xmin : dptSet[j].x;
//				ymax = ymax >= dptSet[j].y ? ymax : dptSet[j].y;
//				ymin = ymin <= dptSet[j].y ? ymin : dptSet[j].y;
//			}
//			if((xmax - xmin) >= (ymax - ymin))
//			{
//				dpt1 = ossimDpt(xmin, slope * xmin + intercept);
//				dpt2 = ossimDpt(xmax, slope * xmax + intercept);
//			}
//			else
//			{
//				dpt1 = ossimDpt((ymin - intercept) / slope, ymin);
//				dpt2 = ossimDpt((ymax - intercept) / slope, ymax);
//			}
//
//			dptLine.m_Points.push_back(dpt1);
//			dptLine.m_Points.push_back(dpt2);
//		}
//		else if(2 == dptSet.size())
//		{//如果是2个点
//			dptLine.m_Points.push_back(dptSet[0]);
//			dptLine.m_Points.push_back(dptSet[1]);
//		}
//		else
//		{
//			continue;
//		}
//
//		dptLineList.push_back(dptLine);
//	}
//}
//void MyProject::ReadGroundLines(ossimFilename strFilename, vector < ossimGFeature > & gptLineList)
//{
//	char strtmp[1024];
//	std::vector<ossimString> gcpcon;
//	ossimString str;
//
//	ifstream os(strFilename.c_str());
//	while(os.getline(strtmp,1024))
//	{
//		//int star;
//		str=strtmp;
//		if (str.empty()) continue;
//		gcpcon=str.split("	");
//		if (gcpcon.size()==1) {
//			str=gcpcon[0];
//			gcpcon=str.split(" ");
//		}
//		if (gcpcon.size()<5) continue;
//		ossimGFeature gptLine(ossimGFeature::ossimGFeatureType::ossimGLineType);
//		vector<ossimGpt> gptSet;
//		gptLine.strId = gcpcon[0];
//		int pos = 0;
//		if(gcpcon[0].empty()) {
//			gptLine.strId=gcpcon[1];
//			pos += 2;
//		}
//		else
//		{
//			gptLine.strId=gcpcon[0];
//			pos++;
//		}
//		int num = gcpcon[pos++].toInt();
//		if((int)gcpcon.size() < num * 3 + pos)
//		{
//			continue;
//		}
//		for(int i = 0;i < num;i++)
//		{
//			ossimGpt pt;
//			pt.lat = gcpcon[pos++].toDouble();
//			pt.lon = gcpcon[pos++].toDouble();
//			pt.hgt = gcpcon[pos++].toDouble();
//			gptSet.push_back(pt);
//		}
//		double xmin, xmax, ymin, ymax;
//		if(gptSet.size() > 2)
//		{// 如果多余两个点，则拟合直线
//			//regression of the line from gpts
//			double slopeY, interceptY, slopeZ, interceptZ;
//			ossim2dLinearRegression theLineFitY;
//			ossim2dLinearRegression theLineFitZ;
//			theLineFitY.clear();
//			theLineFitZ.clear();
//			for(unsigned int j = 0;j < (int)gptSet.size();j++)
//			{
//				theLineFitY.addPoint(ossimDpt(gptSet[j].lat, gptSet[j].lon));
//				theLineFitZ.addPoint(ossimDpt(gptSet[j].lat, gptSet[j].hgt));
//			}
//			theLineFitY.solve();
//			theLineFitZ.solve();
//			theLineFitY.getEquation(slopeY, interceptY);
//			theLineFitZ.getEquation(slopeZ, interceptZ);
//			xmax = gptSet[0].lat;
//			xmin = xmax;
//			ymax = gptSet[0].lon;
//			ymin = ymax;
//			ossimGpt newGpt1, newGpt2;
//			for(unsigned int j = 0;j < (int)gptSet.size();j++)
//			{
//				xmax = xmax >= gptSet[j].lat ? xmax : gptSet[j].lat;
//				xmin = xmin <= gptSet[j].lat ? xmin : gptSet[j].lat;
//				ymax = ymax >= gptSet[j].lon ? ymax : gptSet[j].lon;
//				ymin = ymin <= gptSet[j].lon ? ymin : gptSet[j].lon;
//			}
//			if((xmax - xmin) >= (ymax - ymin))
//			{
//				newGpt1 = ossimGpt(xmin, slopeY * xmin + interceptY, slopeZ * xmin + interceptZ);
//				newGpt2 = ossimGpt(xmax, slopeY * xmax + interceptY, slopeZ * xmin + interceptZ);
//			}
//			else
//			{
//				newGpt1 = ossimGpt((ymin - interceptY) / slopeY, ymin, slopeZ * (ymin - interceptY) / slopeY + interceptZ);
//				newGpt2 = ossimGpt((ymax - interceptY) / slopeY, ymax, slopeZ * (ymax - interceptY) / slopeY + interceptZ);
//			}
//
//			gptLine.m_Points.push_back(newGpt1);
//			gptLine.m_Points.push_back(newGpt2);
//		}
//		else if(2 == gptSet.size())
//		{//如果是2个点
//			gptLine.m_Points.push_back(gptSet[0]);
//			gptLine.m_Points.push_back(gptSet[1]);
//		}
//		else
//		{
//			continue;
//		}
//		gptLineList.push_back(gptLine);
//	}
//}
//
//void MyProject::ReadImageAreas(ossimFilename strFilename, vector < ossimDFeature > & dptAreaList)
//{
//	char strtmp[1024];
//	std::vector<ossimString> gcpcon;
//	ossimString str;
//
//	ifstream os(strFilename.c_str());
//	while(os.getline(strtmp,1024))
//	{
//		//int star;
//		str=strtmp;
//		if (str.empty()) continue;
//		gcpcon=str.split("	");
//		if (gcpcon.size()==1) {
//			str=gcpcon[0];
//			gcpcon=str.split(" ");
//		}
//		if (gcpcon.size()<5) continue;
//		ossimDFeature dptArea(ossimDFeature::ossimDAreaType);
//		dptArea.strId = gcpcon[0];
//		int pos = 0;
//		if(gcpcon[0].empty()) {
//			dptArea.strId=gcpcon[1];
//			pos += 2;
//		}
//		else
//		{
//			dptArea.strId=gcpcon[0];
//			pos++;
//		}
//		int num = gcpcon[pos++].toInt();
//		if((int)gcpcon.size() < num * 2 + pos)
//		{
//			continue;
//		}
//		for(int i = 0;i < num;i++)
//		{
//			ossimDpt pt;
//			pt.x = gcpcon[pos++].toDouble();
//			pt.y = gcpcon[pos++].toDouble();
//			dptArea.m_Points.push_back(pt);
//		}
//		dptAreaList.push_back(dptArea);
//	}
//}
//void MyProject::ReadGroundAreas(ossimFilename strFilename, vector < ossimGFeature > & gptAreaList)
//{
//	char strtmp[1024];
//	std::vector<ossimString> gcpcon;
//	ossimString str;
//
//	ifstream os(strFilename.c_str());
//	while(os.getline(strtmp,1024))
//	{
//		//int star;
//		str=strtmp;
//		if (str.empty()) continue;
//		gcpcon=str.split("	");
//		if (gcpcon.size()==1) {
//			str=gcpcon[0];
//			gcpcon=str.split(" ");
//		}
//		if (gcpcon.size()<5) continue;
//		ossimGFeature gptArea(ossimGFeature::ossimGAreaType);
//		gptArea.strId = gcpcon[0];
//		int pos = 0;
//		if(gcpcon[0].empty()) {
//			gptArea.strId=gcpcon[1];
//			pos += 2;
//		}
//		else
//		{
//			gptArea.strId=gcpcon[0];
//			pos++;
//		}
//		int num = gcpcon[pos++].toInt();
//		if((int)gcpcon.size() < num * 3 + pos)
//		{
//			continue;
//		}
//		for(int i = 0;i < num;i++)
//		{
//			ossimGpt pt;
//			pt.lat = gcpcon[pos++].toDouble();
//			pt.lon = gcpcon[pos++].toDouble();
//			pt.hgt = gcpcon[pos++].toDouble();
//			gptArea.m_Points.push_back(pt);
//		}
//		gptAreaList.push_back(gptArea);
//	}
//}

void  MyProject::read(ossimFilename m_filename)
{
	fstream os;
	os.open(m_filename.c_str(), ios_base::in);
	std::vector<ossimString> temp;
	ossimString temp1;

	os>>m_ImgFileNameUnc;//待校正影像文件名
	temp1=m_ImgFileNameUnc;
	temp=temp1.split("=");
	if (temp.size()==2)  m_ImgFileNameUnc=temp[1];
	else
		m_ImgFileNameUnc="";


	os>>m_ImgFileNameRef;//参考影像文件名
	temp1=m_ImgFileNameRef;
	temp=temp1.split("=");
	if (temp.size()==2) m_ImgFileNameRef=temp[1];
	else
		m_ImgFileNameRef="";

	os>>m_ImgFileNameout;//校正结果影像文件名
	temp1=m_ImgFileNameout;
	temp=temp1.split("=");
	if (temp.size()==2) m_ImgFileNameout=temp[1];
	else
		m_ImgFileNameout="";

	os>>m_DemPath;//高程路径
	temp1=m_DemPath;
	temp=temp1.split("=");
	if (temp.size()==2) {
		m_DemPath=temp[1];
		theMgr = ossimElevManager::instance();
		theMgr->loadElevationPath(m_DemPath.c_str());//
	}
	else
		m_DemPath="";

	os>>temp1;//传感器名称
	temp=temp1.split("=");
	if (temp.size()==2) {
		if(temp[1].toInt()==0) m_SensorName=modelLandsat5;
		if(temp[1].toInt()==1) m_SensorName=modelSpot5;
	}
	else
		m_SensorName=UnknowMole;

		os>>temp1;//模型名称
	temp=temp1.split("=");
	if (temp.size()==2) {
		if(temp[1].toInt()==0) m_ModelType=OrbitType;
		if(temp[1].toInt()==1) m_ModelType=RPCType;
		if(temp[1].toInt()==2) m_ModelType=PolynomialType;
		if(temp[1].toInt()==3) m_ModelType=CombinedAdjustmentType;
	}
	else
		m_ModelType=UnknowType;


		os>>temp1;//采样方法
	temp=temp1.split("=");
	if (temp.size()==2) {m_SampleType=temp[1];
	}
	os>>temp1;//输出文件格式
	temp=temp1.split("=");
	if (temp.size()==2) {   m_FileOutType=temp[1];
	}


	ossim_uint32 bL1,bL2,bL3;
	ossimString str;
	os>>str;
	os>>bL1>>bL2>>bL3;
	m_UncBandList.clear();
	m_UncBandList.push_back(bL1);
	m_UncBandList.push_back(bL2);
	m_UncBandList.push_back(bL3);
	os>>str;
	os>>bL1>>bL2>>bL3;
	m_RefBandList.clear();
	m_RefBandList.push_back(bL1);
	m_RefBandList.push_back(bL2);
	m_RefBandList.push_back(bL3);

	
	os>>temp1;//输出波段
	temp=temp1.split("=");
	if (temp.size()==2) {
		int num=temp[1].toInt();
		for(int i=0;i<num;i++)  {	os>>bL1;m_OutBandList.push_back(bL1);}

	}

	///////////////////////////////////////////////

	//  	os<<"MAP_PROJECTION_BEGIN"<<endl;
	//m_MapProjection.print(os);
	//os<<"MAP_PROJECTION_END"<<endl;
	ossimTempFilename pp;
	char dp[200];
	bool flag=true;
	pp.generateRandomFile();
	fstream oss;
	oss.open(pp.c_str(),ios::out|ios_base::app);
	os>>str;
	while(flag) {
		os.getline(dp,200);str=dp;
		oss<< str.c_str();oss<<endl;
		if(str.contains("MAP_PROJECTION_END")) {flag=false;continue;}
		
	}
	oss.close();
	m_MapProjection.clear();
	m_MapPar=NULL;
	m_MapProjection.addFile(pp.c_str());
	m_MapPar=PTR_CAST(ossimMapProjection,
		ossimMapProjectionFactory::instance()->createProjection(m_MapProjection));
	//////////////读控制点//////////////////////////////////////////////
	ossimString tmp;

	int m_CtrlgcpNum,m_ChkgcpNum,m_OptCtrlNum,m_OptChkNum;
	os>>str;	 os>>str;	
	m_CtrlgcpNum=str.toInt();
	m_CtrlGptSet->clearTiePoints();
	for(int i=0;i<m_CtrlgcpNum; ++i)
	{
		//  os<<tmpoint.getGcpNumberID()<<tmpoint.getGcpNumberID()<<tmpoint.getImagePoint()<<tmpoint.getScore();
		ossimTieGpt* tmpoint;
		tmpoint=new ossimTieGpt;
		//  os>>tmpoint.GcpNumberID>>tmpoint.refGroundPoint()>>tmpoint.refImagePoint()>>tmpoint.refScore();
		os>>tmpoint->GcpNumberID>>tmpoint->refGroundPoint()>>tmpoint->refImagePoint()>>tmpoint->refScore();

		m_CtrlGptSet->addTiePoint(tmpoint);

	}
	//////////////读检查点/////////////////

	os>>str;	 os>>str;	
	m_ChkgcpNum=str.toInt();
	m_ChkGptSet->clearTiePoints();

	for(int i=0;i<m_ChkgcpNum; ++i)
	{
		//  os<<tmpoint.getGcpNumberID()<<tmpoint.getGcpNumberID()<<tmpoint.getImagePoint()<<tmpoint.getScore();
		ossimTieGpt* tmpoint;
		tmpoint=new ossimTieGpt;

		os>>tmpoint->GcpNumberID>>tmpoint->refGroundPoint()>>tmpoint->refImagePoint()>>tmpoint->refScore();
		m_ChkGptSet->addTiePoint(tmpoint);

	}


/////////////用于控制点优化界面中 控制点 和  剔除点的保存///////////////////////
	os>>str;  os>>str;
	m_OptCtrlNum=str.toInt();
	m_OptCtrlGptSet->clearTiePoints();
	for(int i=0;i<m_OptCtrlNum; ++i)
	{
		ossimTieGpt* tmpoint;
		tmpoint=new ossimTieGpt;
		os>>tmpoint->GcpNumberID>>tmpoint->refGroundPoint()>>tmpoint->refImagePoint()>>tmpoint->refScore();
		m_OptCtrlGptSet->addTiePoint(tmpoint);

	}

	os>>str;  os>>str;
	m_OptChkNum=str.toInt();
	m_OptChkGptSet->clearTiePoints();
	for(int i=0;i<m_OptChkNum; ++i)
	{
		ossimTieGpt* tmpoint;
		tmpoint=new ossimTieGpt;
		os>>tmpoint->GcpNumberID>>tmpoint->refGroundPoint()>>tmpoint->refImagePoint()>>tmpoint->refScore();
		m_OptChkGptSet->addTiePoint(tmpoint);

	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	os>>starline;//
	temp1=starline;
	temp=temp1.split("=");
	if (temp.size()==2) starline=atoi(temp[1]);
	else
		starline=0;
	os>>starpixel;//
	temp1=starpixel;
	temp=temp1.split("=");
	if (temp.size()==2) starpixel=atoi(temp[1]);
	else
		starpixel=0;
	os>>endline;//
	temp1=endline;
	temp=temp1.split("=");
	if (temp.size()==2) endline=atoi(temp[1]);
	else
		endline=0;
	os>>endpixel;//
	temp1=endpixel;
	temp=temp1.split("=");
	if (temp.size()==2) endpixel=atoi(temp[1]);
	else
		endpixel=0;


	os>>temp1;//
	temp=temp1.split("=");
	if (temp.size()==2)  m_PolyDegree=temp[1];

	//   os>>starline>>starpixel>>endline>>endpixel;

	//////////////////////////////////////////////////////////////////////////////////////////
	os.close();


}

void  MyProject::write(ossimFilename m_filename)
{

	int m_pointNum=m_CtrlGptSet->size();
	//  ofstream os(m_filename.c_str(),ios::out|ios_base::app);
	fstream os;
	os.open(m_filename.c_str(),ios::out);
	os<<"m_ImgFileNameUnc="<<m_ImgFileNameUnc.c_str()<<endl;
	os<<"m_ImgFileNameRef="<<m_ImgFileNameRef.c_str()<<endl;
	os<<"m_ImgFileNameout="<<m_ImgFileNameout.c_str()<<endl;
	os<<"m_DemPath="<<m_DemPath.c_str()<<endl;
	os<<"SENSORNAME="<<m_SensorName<<endl;
	os<<"ModelType="<<m_ModelType<<endl;

	os<<"SampleType="<<m_SampleType<<endl;//1125
	os<<"FileOutType="<<m_FileOutType<<endl;//1125


	os<<"m_UncBandList="<<m_UncBandList.size()<<endl;
	for(int i=0;i<(int)m_UncBandList.size();i++) os <<m_UncBandList[i]<<endl;
	os<<"m_RefBandList="<<m_RefBandList.size()<<endl;
	for(int i=0;i<(int)m_RefBandList.size();i++) os <<m_RefBandList[i]<<endl;

	os<<"m_OutBandList="<<m_OutBandList.size()<<endl;
	for(int i=0;i<(int)m_OutBandList.size();i++) os <<m_OutBandList[i]<<endl;
	

	os<<"MAP_PROJECTION_BEGIN"<<endl;
	m_MapProjection.print(os);
	os<<"MAP_PROJECTION_END"<<endl;
/////////////////////开始写控制点选取界面中的 控制点//////////////////////////////////////////////
	os<<"GcpTiePoints: "<<m_CtrlGptSet->size()<<endl;
	ossimTieGpt tmpoint;
	for(vector<ossimRefPtr<ossimTieGpt> >::const_iterator it = m_CtrlGptSet->refTiePoints().begin(); it != m_CtrlGptSet->refTiePoints().end(); ++it)
	{
		tmpoint.GcpNumberID = (*it)->GcpNumberID;
		tmpoint.setGroundPoint((*it)->getGroundPoint());
		tmpoint.setImagePoint((*it)->getImagePoint());
		tmpoint.setScore((*it)->getScore());
		os<<tmpoint.getGcpNumberID()<<" "<<tmpoint.getGroundPoint()<<" "<<tmpoint.getImagePoint()<<" "<<tmpoint.getScore()<<" ";
		os<<endl;
	}

////////////////////开始写控制点选取界面中的 检查点///////////////////////////////
	os<<"ChkTiePoints: "<<m_ChkGptSet->size()<<endl;
	for(vector<ossimRefPtr<ossimTieGpt> >::const_iterator it = m_ChkGptSet->refTiePoints().begin(); it != m_ChkGptSet->refTiePoints().end(); ++it)
	{
		tmpoint.GcpNumberID = (*it)->GcpNumberID;
		tmpoint.setGroundPoint((*it)->getGroundPoint());
		tmpoint.setImagePoint((*it)->getImagePoint());
		tmpoint.setScore((*it)->getScore());

		// os<<tmpoint;
		os<<tmpoint.getGcpNumberID()<<" "<<tmpoint.getGroundPoint()<<" "<<tmpoint.getImagePoint()<<" "<<tmpoint.getScore()<<" ";
		os<<endl;
	}

///////////用于控制点优化界面中 控制点的保存
	os<<"OptGcpTiePoints: "<<m_OptCtrlGptSet->size()<<endl;
	for(vector<ossimRefPtr<ossimTieGpt> >::const_iterator it = m_OptCtrlGptSet->refTiePoints().begin(); it != m_OptCtrlGptSet->refTiePoints().end(); ++it)
	{
		tmpoint.GcpNumberID = (*it)->GcpNumberID;
		tmpoint.setGroundPoint((*it)->getGroundPoint());
		tmpoint.setImagePoint((*it)->getImagePoint());
		tmpoint.setScore((*it)->getScore());

		// os<<tmpoint;
		os<<tmpoint.getGcpNumberID()<<" "<<tmpoint.getGroundPoint()<<" "<<tmpoint.getImagePoint()<<" "<<tmpoint.getScore()<<" ";
		os<<endl;
	}


/////////用于控制点优化界面中 剔除点的保存

	os<<"OptRejectTiePoints: "<<m_OptRejectGptSet->size()<<endl;
	for(vector<ossimRefPtr<ossimTieGpt> >::const_iterator it = m_OptRejectGptSet->refTiePoints().begin(); it != m_OptRejectGptSet->refTiePoints().end(); ++it)
	{
		tmpoint.GcpNumberID = (*it)->GcpNumberID;
		tmpoint.setGroundPoint((*it)->getGroundPoint());
		tmpoint.setImagePoint((*it)->getImagePoint());
		tmpoint.setScore((*it)->getScore());

		// os<<tmpoint;
		os<<tmpoint.getGcpNumberID()<<" "<<tmpoint.getGroundPoint()<<" "<<tmpoint.getImagePoint()<<" "<<tmpoint.getScore()<<" ";
		os<<endl;
	}

//////////////////开始写输出图像的像素坐标//////////////////////////////////
	os<<"starline="<<starline<<endl;//ww1029   
	os<<"starpixel="<<starpixel<<endl;
	os<<"endline="<<endline<<endl;
	os<<"endpixel="<<endpixel<<endl;
	if(m_ModelType==PolynomialType) os<<"m_PolyDegree="<<m_PolyDegree<<endl;
	//  os<<endl;
	os.close();


}
//1120
void MyProject::SavePointToFile(ossimFilename filenametosave,ossimTieGptSet* m_gptset, ossimTieGptSet* mcheck_gptset, bool printProjection/*=true*/)
{
	saveGcpFile(filenametosave, m_gptset, mcheck_gptset, &m_MapProjection);
	return;
//////////////////////////////////////////////////输出txt
	if (filenametosave.ext().contains("txt")) {

	std::fstream     theTieFileStream;
	theTieFileStream.open(filenametosave.c_str(), ios_base::out);

	if (printProjection)
	{
		theTieFileStream<<"MAP_PROJECTION_BEGIN"<<endl;
		m_MapProjection.print(theTieFileStream);
		theTieFileStream<<"MAP_PROJECTION_END"<<endl;
	}

	theTieFileStream.setf(ios::fixed, ios::floatfield);
	theTieFileStream.precision(8);
	if (m_gptset)
	{
		for (int i = 0; i<(int)m_gptset->refTiePoints().size(); i++)
		{
			theTieFileStream<<setiosflags(ios::left)<<
				setw(10)<<m_gptset->refTiePoints()[i]->GcpNumberID.c_str()<<
				setw(20)<<m_gptset->refTiePoints()[i]->refImagePoint().x<<
				setw(20)<<m_gptset->refTiePoints()[i]->refImagePoint().y<<
				setw(20)<<m_gptset->refTiePoints()[i]->refGroundPoint().lon<<
				setw(20)<<m_gptset->refTiePoints()[i]->refGroundPoint().lat<<
				setw(15)<<m_gptset->refTiePoints()[i]->refGroundPoint().hgt<<endl;

		}
	}
	if (mcheck_gptset)
	{
		for (int i = 0; i<(int)mcheck_gptset->refTiePoints().size(); i++)
		{
			theTieFileStream<<setiosflags(ios::left)<<
				setw(10)<<"-"+mcheck_gptset->refTiePoints()[i]->GcpNumberID<<
				setw(20)<<mcheck_gptset->refTiePoints()[i]->refImagePoint().x<<
				setw(20)<<mcheck_gptset->refTiePoints()[i]->refImagePoint().y<<
				setw(20)<<mcheck_gptset->refTiePoints()[i]->refGroundPoint().lon<<
				setw(20)<<mcheck_gptset->refTiePoints()[i]->refGroundPoint().lat<<
				setw(15)<<mcheck_gptset->refTiePoints()[i]->refGroundPoint().hgt<<endl;

		}
	}
	theTieFileStream.close();

	}
	//////////////////////////////////////////////////输出shpfile
	if (filenametosave.ext().contains("shp")) {
		bool flag=false;
			flag=WriteShpPoint(filenametosave,m_gptset,mcheck_gptset);

		}

	}

void MyProject::SaveOptPointToFile(ossimFilename filenametosave,ossimTieGptSet* m_gptset)
{
	std::fstream     theTieFileStream;
	theTieFileStream.open(filenametosave.c_str(), ios_base::out);

	theTieFileStream<<"MAP_PROJECTION_BEGIN"<<endl;
	m_MapProjection.print(theTieFileStream);
	theTieFileStream<<"MAP_PROJECTION_END"<<endl;


	for (int i = 0; i<(int)m_gptset->refTiePoints().size(); i++)
	{
		theTieFileStream<<m_gptset->refTiePoints()[i]->GcpNumberID.c_str()<<"	"<< setiosflags(ios::fixed)<<
			setprecision(9)<<m_gptset->refTiePoints()[i]->refImagePoint().x<<"	"<<
			setprecision(9)<<m_gptset->refTiePoints()[i]->refImagePoint().y<<"	"<<
			setprecision(9)<<m_gptset->refTiePoints()[i]->refGroundPoint().lat<<"	"<<
			setprecision(9)<<m_gptset->refTiePoints()[i]->refGroundPoint().lon<<"	"<<
			setprecision(9)<<m_gptset->refTiePoints()[i]->refGroundPoint().hgt<<endl;

	}
	theTieFileStream.close();
}

	bool MyProject::WriteShpPoint(ossimFilename filenametosave,ossimTieGptSet* m_gptset,ossimTieGptSet* mcheck_gptset)
	{
	
			const char *pszDriverName = "ESRI Shapefile";
			OGRSFDriver *poDriver;
			poDriver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(
				pszDriverName );
			if( poDriver == NULL )
			{
				return false;//创建文件驱动失败
			}

			if(filenametosave.isFile()) {
				poDriver->DeleteDataSource(filenametosave.c_str());
			}
			OGRDataSource *poDS;
			poDS = poDriver->CreateDataSource( filenametosave.c_str(), NULL );
			if( poDS == NULL )
			{
				return false;//创建源失败
			}
			OGRLayer *poLayer;
			poLayer = poDS->CreateLayer( "gcp", NULL, wkbPoint, NULL );
			if( poLayer == NULL )
			{
				return false;//创建gcp层对象失败
			} 
			OGRFieldDefn oField( "gcpid", OFTString );
			oField.SetWidth(32);
			if( poLayer->CreateField( &oField ) != OGRERR_NONE )
			{
				return false;//创建gcpid对象失败
			}
			ossimGpt gptmp;

			for (vector<ossimRefPtr<ossimTieGpt> >::const_iterator it = m_gptset->refTiePoints().begin(); it != m_gptset->refTiePoints().end(); ++it)

			{
				OGRFeature *poFeature;

				poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
				poFeature->SetField( "gcpid", (*it)->GcpNumberID.c_str() );

				OGRPoint pt;
				gptmp= (*it)->getGroundPoint() ;
				pt.setX( gptmp.lat );
				pt.setY( gptmp.lon  );
				pt.setZ( gptmp.hgt );

				poFeature->SetGeometry( &pt ); 

				if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE )
				{
					return false;
				}

				OGRFeature::DestroyFeature( poFeature );
			}

		for (vector<ossimRefPtr<ossimTieGpt> >::const_iterator it = mcheck_gptset->refTiePoints().begin(); it != mcheck_gptset->refTiePoints().end(); ++it)

			{
				OGRFeature *poFeature;

				poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
				poFeature->SetField( "gcpid", (*it)->GcpNumberID.c_str() );

				OGRPoint pt;

				gptmp= (*it)->getGroundPoint() ;
				pt.setX( gptmp.lon );
				pt.setY( gptmp.lat  );
				pt.setZ( gptmp.hgt );

				poFeature->SetGeometry( &pt ); 

				if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE )
				{
					return false;
				}

				OGRFeature::DestroyFeature( poFeature );

			}

			OGRDataSource::DestroyDataSource( poDS );

		return true;
	}


bool MyProject::SavePoint2Shape(ossimFilename filenametosave,ossimTieGptSet* m_gptset)
{

	GDALAllRegister();
	OGRRegisterAll();//注册所有的文件格式驱动
	const char *pszDriverName = "ESRI Shapefile";
	OGRSFDriver *poDriver;
	poDriver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(
		pszDriverName );
	if( poDriver == NULL )
	{
		return false;//创建文件驱动失败
	}

	if(filenametosave.isFile()) {
		poDriver->DeleteDataSource(filenametosave.c_str());
	}
	OGRDataSource *poDS;
	poDS = poDriver->CreateDataSource( filenametosave.c_str(), NULL );
	if( poDS == NULL )
	{
		return false;//创建源失败
	}

	//如果是点
	OGRLayer *poLayer;
	poLayer = poDS->CreateLayer( filenametosave.fileNoExtension(), NULL, wkbPoint, NULL );
	if( poLayer == NULL )
	{
		return false;//创建图层对象失败
	} 

	OGRFieldDefn oField( "id", OFTString );
	oField.SetWidth(32);
	if( poLayer->CreateField( &oField ) != OGRERR_NONE )
	{
		return false;
	}
	for(unsigned int i = 0;i < m_gptset->getTiePoints().size();i++)
	{
		OGRFeature *poFeature;
		poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
		poFeature->SetField( "id", m_gptset->getTiePoints()[i]->getGcpNumberID() );

		OGRPoint pt;
		pt.setX(m_gptset->getTiePoints()[i]->getGroundPoint().lat);
		pt.setY(m_gptset->getTiePoints()[i]->getGroundPoint().lon);
		pt.setZ(m_gptset->getTiePoints()[i]->getGroundPoint().hgt);

		poFeature->SetGeometry( &pt ); 

		if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE )
		{
			return false;
		}
		OGRFeature::DestroyFeature( poFeature );
	}

	OGRDataSource::DestroyDataSource( poDS );
	return true;
}
//1120end


bool MyProject::UpdateFeatures(ossimSensorModel* sensorModel, vector<ossimTieFeature>& featureList)
{
	for(unsigned int i = 0;i < featureList.size();i++)
	{
		//// for debug
		//for(unsigned int j = 0;j < featureList[i].getGroundFeature().m_Points.size();j++)
		//{
		//	ossimGpt gpt = featureList[i].getGroundFeature().m_Points[j];
		//	ossimGpt ll = sensorModel->m_proj->inverse(ossimDpt(gpt.lat, gpt.lon));
		//	ossimDpt dpt;
		//	sensorModel->worldToLineSample(ll, dpt);
		//	ossimGpt ll1;
		//	sensorModel->lineSampleToWorld(dpt, ll1);
		//	ossimDpt dpt1 = sensorModel->m_proj->forward(ll1);
		//	cout<<dpt1<<endl;
		//}
		featureList[i].refGroundFeature().m_Points.clear();
		for(unsigned int j = 0;j < featureList[i].getImageFeature().m_Points.size();j++)
		{
			ossimDpt imagepoint = featureList[i].getImageFeature().m_Points[j];
			ossimGpt tGpt;
			sensorModel->lineSampleToWorld(imagepoint, tGpt);
			tGpt.hgt = theMgr->getHeightAboveEllipsoid(tGpt);
			ossimDpt gpt = sensorModel->m_proj->forward(tGpt);
			featureList[i].refGroundFeature().m_Points.push_back(ossimGpt(gpt.lon, gpt.lat, tGpt.hgt));
		}
	}
	return true;
}

bool MyProject::SaveFeaturetoShape(ossimFilename filenametosave,vector<ossimTieFeature> tiefeatureList, ossimFeatureType featureType/* = ossimFeatureType::ossimPolygonType*/)
{
	GDALAllRegister();
	OGRRegisterAll();//注册所有的文件格式驱动
	const char *pszDriverName = "ESRI Shapefile";
	OGRSFDriver *poDriver;
	poDriver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(
		pszDriverName );
	if( poDriver == NULL )
	{
		return false;//创建文件驱动失败
	}

	if(filenametosave.isFile()) {
		poDriver->DeleteDataSource(filenametosave.c_str());
	}
	OGRDataSource *poDS;
	poDS = poDriver->CreateDataSource( filenametosave.c_str(), NULL );
	if( poDS == NULL )
	{
		return false;//创建源失败
	}


	if(ossimFeatureType::ossimStraightLineType == featureType
		|| ossimFeatureType::ossimFreeLineType == featureType)
	{
		// 如果是线
		OGRLayer *poLayer;
		poLayer = poDS->CreateLayer( filenametosave.fileNoExtension(), NULL, wkbLineString, NULL );
		if( poLayer == NULL )
		{
			return false;//创建图层对象失败
		} 

		OGRFieldDefn oField( "id", OFTString );
		oField.SetWidth(32);
		if( poLayer->CreateField( &oField ) != OGRERR_NONE )
		{
			return false;
		}
		for(unsigned int i = 0;i < tiefeatureList.size();i++)
		{
			if(ossimFeatureType::ossimStraightLineType == tiefeatureList[i].getGroundFeature().m_featureType
				|| ossimFeatureType::ossimFreeLineType == tiefeatureList[i].getGroundFeature().m_featureType)
			{
				OGRFeature *poFeature;
				poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
				poFeature->SetField( "id", tiefeatureList[i].strId );

				OGRLineString lineString;
				int nPt = (int)tiefeatureList[i].getGroundFeature().m_Points.size();
				lineString.setNumPoints(nPt);
				for(int j = 0;j < nPt;j++)
				{
					lineString.setPoint(j, tiefeatureList[i].getGroundFeature().m_Points[j].lat, tiefeatureList[i].getGroundFeature().m_Points[j].lon, tiefeatureList[i].getGroundFeature().m_Points[j].hgt);
				}

				poFeature->SetGeometry( &lineString ); 

				if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE )
				{
					return false;
				}

				OGRFeature::DestroyFeature( poFeature );
			}
		}
	}
	else if(ossimFeatureType::ossimPointType == featureType)
	{
		//如果是点
		OGRLayer *poLayer;
		poLayer = poDS->CreateLayer( filenametosave.fileNoExtension(), NULL, wkbPoint, NULL );
		if( poLayer == NULL )
		{
			return false;//创建图层对象失败
		} 

		OGRFieldDefn oField( "id", OFTString );
		oField.SetWidth(32);
		if( poLayer->CreateField( &oField ) != OGRERR_NONE )
		{
			return false;
		}
		for(unsigned int i = 0;i < tiefeatureList.size();i++)
		{
			if(ossimFeatureType::ossimPointType == tiefeatureList[i].getGroundFeature().m_featureType)
			{
				OGRFeature *poFeature;
				poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
				poFeature->SetField( "id", tiefeatureList[i].strId );

				OGRPoint pt;
				pt.setX(tiefeatureList[i].getGroundFeature().m_Points[0].lat);
				pt.setY(tiefeatureList[i].getGroundFeature().m_Points[0].lon);
				pt.setZ(tiefeatureList[i].getGroundFeature().m_Points[0].hgt);

				poFeature->SetGeometry( &pt ); 

				if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE )
				{
					return false;
				}
				OGRFeature::DestroyFeature( poFeature );
			}
		}
	}
	else if(ossimFeatureType::ossimPolygonType == featureType)
	{
		// 如果是面
		OGRLayer *poLayer;
		poLayer = poDS->CreateLayer( filenametosave.fileNoExtension(), NULL, wkbPolygon, NULL );
		if( poLayer == NULL )
		{
			return false;//创建图层对象失败
		} 

		OGRFieldDefn oField( "id", OFTString );
		oField.SetWidth(32);
		if( poLayer->CreateField( &oField ) != OGRERR_NONE )
		{
			return false;
		}
		for(unsigned int i = 0;i < tiefeatureList.size();i++)
		{
			if(ossimFeatureType::ossimPolygonType == tiefeatureList[i].getGroundFeature().m_featureType)
			{
				OGRFeature *poFeature;
				poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
				poFeature->SetField( "id", tiefeatureList[i].strId );

				OGRPolygon polygon;
				OGRLinearRing *pOGRLinearRing = new OGRLinearRing();
				int nPoint = (int)tiefeatureList[i].getGroundFeature().m_Points.size();
				OGRRawPoint* pointsList = new OGRRawPoint[nPoint];
				for(int j = 0;j < nPoint;j++)
				{
					pointsList[j].x = tiefeatureList[i].getGroundFeature().m_Points[j].lat;
					pointsList[j].y = tiefeatureList[i].getGroundFeature().m_Points[j].lon;
				}
				pOGRLinearRing->setNumPoints(nPoint);
				pOGRLinearRing->setPoints(nPoint, pointsList);
				pOGRLinearRing->closeRings();
				polygon.addRing(pOGRLinearRing);

				poFeature->SetGeometry( &polygon ); 

				if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE )
				{
					return false;
				}
				delete []pointsList;
				OGRFeature::DestroyFeature( poFeature );
			}
		}
	}

	OGRDataSource::DestroyDataSource( poDS );
	return true;
}

bool MyProject::CheckSenserModel(ossimSensorModel* &sensorModel,
					  ossimTieGptSet* &allGptSet,
					  ossimTieGptSet* &ctrlSet,
					  ossimTieGptSet* &chkSet,
					  ossimTieGptSet* &errSet,
					  double meter_threshold,
					  NEWMAT::ColumnVector &ctrlResidue,
					  NEWMAT::ColumnVector &chkResidue,
					  NEWMAT::ColumnVector &errResidue)
{
	ctrlResidue = CalcResidue(sensorModel, *ctrlSet);

	int num = static_cast<int>(ctrlSet->size());

	double maxerror=0.0;
	ossimString gcpID;
	int nIndex;
	for(int i = 0;i < num;++i)
	{
		double length = ( ctrlResidue.element(i * 3 + 0) * ctrlResidue.element(i * 3 + 0)
			+ ctrlResidue.element(i * 3 + 1) * ctrlResidue.element(i * 3 + 1)
			+ ctrlResidue.element(i * 3 + 2) * ctrlResidue.element(i * 3 + 2));
		length = sqrt(length);
		if(length > maxerror ) 
		{
			maxerror = length;
			gcpID = ctrlSet->getTiePoints()[i]->GcpNumberID;
			nIndex = i;
		}
	}

	if(maxerror > meter_threshold)
	{//如果大于2 个象素，则删除此点，重新结构优化和参数优化
		for(int i = 0;i < static_cast<int>(m_CtrlGptSet->size());i++)
		{
			if(gcpID == allGptSet->getTiePoints()[i]->GcpNumberID )
			{
				ossimTieGpt* newTieGpt = new ossimTieGpt;
				*newTieGpt = *(m_CtrlGptSet->getTiePoints()[i]);
				errSet->refTiePoints().push_back(newTieGpt);

				allGptSet->refTiePoints().erase(allGptSet->refTiePoints().begin() + i);
				ctrlSet->refTiePoints().erase(ctrlSet->refTiePoints().begin() + nIndex);

				ctrlResidue = CalcResidue(sensorModel, *ctrlSet);
				chkResidue = CalcResidue(sensorModel, *chkSet);
				errResidue = CalcResidue(sensorModel, *errSet);
				return false;
			}
		}		
		return false;
	}
	ctrlResidue = CalcResidue(sensorModel, *ctrlSet);
	chkResidue = CalcResidue(sensorModel, *chkSet);
	errResidue = CalcResidue(sensorModel, *errSet);
	return true;
}

bool MyProject::DistributeOptimize(ossimTieGptSet* srcGptSet, ossimTieGptSet* &ctrlGptSet, ossimTieGptSet* &chkGtpSet,int nControl, int nCheck)
{
	int line_count;
	line_count = srcGptSet->size();

	if (line_count == 0)
		return false;
	if ((nControl + nCheck) > line_count)
		return false;

	ctrlGptSet = new ossimTieGptSet;
	chkGtpSet = new ossimTieGptSet;



	/////////////////////////////////////////////

	vector<ossimRefPtr<ossimTieGpt> >& theTPV = srcGptSet->refTiePoints();
	vector<ossimRefPtr<ossimTieGpt> >::const_iterator tit;
	vector<ossimString> m_gcpid;
	for (tit = theTPV.begin() ; tit != theTPV.end() ; ++tit)
	{
		ctrlGptSet->addTiePoint(*tit);

	}
	int i=0;

	pointinfo* p = new pointinfo[line_count];
	pointinfo* p_cluster = new pointinfo[line_count];
	double* d = new double[line_count];

	//	theTPV= ctrlGptSet->refTiePoints();
	//m_chkGtpSet->clearTiePoints();
	//m_CtrlGptSet->clearTiePoints();

	for (tit = theTPV.begin() ; tit != theTPV.end() ; ++tit)
	{
		//	  p[i].indext=(*tit)->GcpNumberID.toInt();
		m_gcpid.push_back((*tit)->GcpNumberID);
		p[i].indext=i;
		p[i].x=(*tit)->getImagePoint().x;
		p[i].y=(*tit)->getImagePoint().y;
		p[i].x2=(*tit)->getGroundPoint().lat;
		p[i].y2=(*tit)->getGroundPoint().lon;
		p[i].z=(*tit)->hgt;
		i++;
	}

	ctrlGptSet->clearTiePoints();

	///////////////////////////////////////////
	//求出最长距离的两点做为初始聚类中心
	double r=0.0;

	srand((unsigned)time(NULL));
	int random = rand() % line_count+1;
	cout<<random<<endl;
	p_cluster[0]=p[random-1];
	//r=std::distance(p[0].x,p_cluster[0].x,p[0].y,p_cluster[0].y);
	r=sqrt((p[0].x-p_cluster[0].x)*(p[0].x-p_cluster[0].x)+(p[0].y-p_cluster[0].y)*(p[0].y-p_cluster[0].y));

	p_cluster[1]=p[0];
	for (int i=0;i<line_count;i++)
	{
		d[i]=sqrt((p[i].x-p_cluster[0].x)*(p[i].x-p_cluster[0].x)+(p[i].y-p_cluster[0].y)*(p[i].y-p_cluster[0].y));

		//d[i]=distance(p[i].x,p_cluster[0].x,p[i].y,p_cluster[0].y);
		if (d[i]>r)
		{
			r=d[i];
			p_cluster[1]=p[i];
		}

	}
	int count=2;//类中的个数
	double r_temp;
	int flag=1;
	int j=0;
	int p_res;//保留的控制点数

	//	int remainnum;//保留点个数
	//int remainChecknum;//保留点个数

	p_res = nControl;

	while (count<p_res)
	{  j=0;
	r_cluster* r_clus=new r_cluster[line_count-count];

	for(int i=0;i<line_count;i++)
	{   flag=1;
	for(int l=0;l<count;l++){
		if(p_cluster[l].indext==p[i].indext)
		{flag=0;break;}}
	if(flag==1)
	{
		r=sqrt((p[i].x-p_cluster[0].x)*(p[i].x-p_cluster[0].x)+(p[i].y-p_cluster[0].y)*(p[i].y-p_cluster[0].y));
		//r=distance(p[i].x,p_cluster[0].x,p[i].y,p_cluster[0].y);
		for(int k=1;k<count;k++)
		{

			// r_temp=distance(p[i].x,p_cluster[k].x,p[i].y,p_cluster[k].y);
			r_temp=sqrt((p[i].x-p_cluster[k].x)*(p[i].x-p_cluster[k].x)+(p[i].y-p_cluster[k].y)*(p[i].y-p_cluster[k].y));

			if(r_temp<r){
				r_clus[j].r=r_temp;
				r_clus[j].indext=i;
				r=r_temp;}
			else{
				r_clus[j].r=r;
				r_clus[j].indext=i;}
		}
		j++;
	}
	}

	for(int n=0;n<line_count-count-1;n++)
	{if(r_clus[n].r>r_clus[n+1].r)
	r_clus[n+1]=r_clus[n];}
	p_cluster[count]=p[r_clus[line_count-count-1].indext];
	count++;
	}

	//输出优化结果/////////////////////////////////////////////////////
	//const char *filename=outputname.c_str();

	ossimTieGpt *aTiePt;
	for (int i=0;i<p_res;i++)
	{ 

		//o_file<<p_cluster[i].indext<<setprecision(10)<<" "<<p_cluster[i].x<<" "<<p_cluster[i].y<<" "<<p_cluster[i].x2<<" "<<p_cluster[i].y2<<" "<<p_cluster[i].z<<endl;

		ossimDpt p1(p_cluster[i].x,p_cluster[i].y);
		ossimGpt p2;


		p2.lat=p_cluster[i].x2;
		p2.lon=p_cluster[i].y2;
		p2.hgt=p_cluster[i].z;
		aTiePt=new ossimTieGpt(p2,p1,p2.hgt);
		//aTiePt->GcpNumberID=ossimString::toString(p_cluster[i].indext);
		aTiePt->GcpNumberID=m_gcpid[p_cluster[i].indext];

		ctrlGptSet->addTiePoint(aTiePt);

	}

	//输出剩余点 ///////////////////////////////////////////////
	for(j=0;j<p_res;j++)
		for(int i=0;i<line_count;i++)
		{
			if (p_cluster[j].indext==p[i].indext)
			{
				for(int k=i;k<line_count-1;k++)
					p[k]=p[k+1];
			}
		}
		//	const char filename2[]="rest.txt";

		//	ofstream o_file2;
		//	o_file2.open(filename2);
		for (int i=0;i<line_count-p_res;i++)
		{
			ossimDpt p1(p[i].x,p[i].y);
			ossimGpt p2;


			p2.lat=p[i].x2;
			p2.lon=p[i].y2;
			p2.hgt=p[i].z;

			aTiePt=new ossimTieGpt(p2,p1,p2.hgt);
			//aTiePt->GcpNumberID=ossimString::toString(p[i].indext);
			//m_chkGtpSet->addTiePoint(aTiePt);

			aTiePt->GcpNumberID=m_gcpid[p[i].indext];
			chkGtpSet->addTiePoint(aTiePt);
			//o_file2<<p[i].indext<<setprecision(10)<<" "<<p[i].x<<" "<<p[i].y<<" "<<p[i].x2<<" "<<p[i].y2<<" "<<p[i].z<<endl;
		}
		//	o_file.close();
		///////////////////////////////////////////////////////////////

		delete d;
		delete p_cluster;
		delete p;
		m_gcpid.clear();
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


		line_count = chkGtpSet->size();
		const vector<ossimRefPtr<ossimTieGpt> >& theRTPV = chkGtpSet->getTiePoints();
		p=new pointinfo[line_count];
		i=0;

		for (tit = theRTPV.begin() ; tit != theRTPV.end() ; ++tit)
		{
			m_gcpid.push_back((*tit)->GcpNumberID);
			p[i].indext=i;
			p[i].x=(*tit)->getImagePoint().x;
			p[i].y=(*tit)->getImagePoint().y;
			p[i].x2=(*tit)->getGroundPoint().lat;
			p[i].y2=(*tit)->getGroundPoint().lon;
			p[i].z=(*tit)->hgt;
			i++;
		}

		chkGtpSet->clearTiePoints();
		///////////////////////////////////////////
		//求出最长距离的两点做为初始聚类中心
		r=0.0;
		d = new double[line_count];

		p_cluster=new pointinfo[line_count];
		srand((unsigned)time(NULL));
		random = rand() % line_count+1;

		p_cluster[0]=p[random-1];
		// r=distance(p[0].x,p_cluster[0].x,p[0].y,p_cluster[0].y);
		r=sqrt((p[0].x-p_cluster[0].x)*(p[0].x-p_cluster[0].x)+(p[0].y-p_cluster[0].y)*(p[0].y-p_cluster[0].y));
		p_cluster[1]=p[0];
		for (int i=0;i<line_count;i++)
		{
			d[i]=sqrt((p[i].x-p_cluster[0].x)*(p[i].x-p_cluster[0].x)+(p[i].y-p_cluster[0].y)*(p[i].y-p_cluster[0].y));
			//d[i]=distance(p[i].x,p_cluster[0].x,p[i].y,p_cluster[0].y);
			if (d[i]>r)
			{
				r=d[i];
				p_cluster[1]=p[i];
			}

		}
		count=2;//类中的个数
		r_temp;
		flag=1;
		j=0;
		p_res;//保留的控制点数

		//	int remainnum;//保留点个数
		//int remainChecknum;//保留点个数

		p_res = nCheck;

		while (count<p_res)
		{  j=0;
		r_cluster* r_clus=new r_cluster[line_count-count];

		for(int i=0;i<line_count;i++)
		{   flag=1;
		for(int l=0;l<count;l++){
			if(p_cluster[l].indext==p[i].indext)
			{flag=0;break;}}
		if(flag==1)
		{
			r=sqrt((p[i].x-p_cluster[0].x)*(p[i].x-p_cluster[0].x)+(p[i].y-p_cluster[0].y)*(p[i].y-p_cluster[0].y));
			// r=distance(p[i].x,p_cluster[0].x,p[i].y,p_cluster[0].y);
			for(int k=1;k<count;k++)
			{
				r_temp=sqrt((p[i].x-p_cluster[k].x)*(p[i].x-p_cluster[k].x)+(p[i].y-p_cluster[k].y)*(p[i].y-p_cluster[k].y));

				//  r_temp=distance(p[i].x,p_cluster[k].x,p[i].y,p_cluster[k].y);
				if(r_temp<r){
					r_clus[j].r=r_temp;
					r_clus[j].indext=i;
					r=r_temp;}
				else{
					r_clus[j].r=r;
					r_clus[j].indext=i;}
			}
			j++;
		}
		}

		for(int n=0;n<line_count-count-1;n++)
		{if(r_clus[n].r>r_clus[n+1].r)
		r_clus[n+1]=r_clus[n];}
		p_cluster[count]=p[r_clus[line_count-count-1].indext];               
		count++;
		}

		//输出优化结果/////////////////////////////////////////////////////
		//const char *filename=outputname.c_str();


		for (int i=0;i<p_res;i++)
		{
			ossimTieGpt tpp;
			//o_file<<p_cluster[i].indext<<setprecision(10)<<" "<<p_cluster[i].x<<" "<<p_cluster[i].y<<" "<<p_cluster[i].x2<<" "<<p_cluster[i].y2<<" "<<p_cluster[i].z<<endl;

			ossimDpt p1(p_cluster[i].x,p_cluster[i].y);
			ossimGpt p2;


			p2.lat=p_cluster[i].x2;
			p2.lon=p_cluster[i].y2;
			p2.hgt=p_cluster[i].z;

			aTiePt=new ossimTieGpt(p2,p1,p2.hgt);
			//aTiePt->GcpNumberID=ossimString::toString(p_cluster[i].indext);
			//		m_chkGtpSet->addTiePoint(aTiePt);
			aTiePt->GcpNumberID=m_gcpid[p_cluster[i].indext];
			chkGtpSet->addTiePoint(aTiePt);
		}
		m_gcpid.clear();
		////////////////////////////////////////////////////////////////////
		delete d;
		delete p_cluster;
		delete p;

		return true;
}


void MyProject::UpdateSensorModel(ossimTieGptSet tieGptSet,
					   ossimSensorModel* &sensorModel,
					   ossimKeywordlist& geom)
{
	ossimDpt imagepoint,cimagepoint;
	ossimGpt goundpoint,tGpt;
	int i;

	int num = static_cast<int>(tieGptSet.size());

	vector<ossimRefPtr<ossimTieGpt> >& theTPV = tieGptSet.refTiePoints();

	vector<ossimRefPtr<ossimTieGpt> >::iterator tit;

	for (tit = theTPV.begin() ; tit != theTPV.end() ; ++tit)
	{
		imagepoint=(*tit)->getImagePoint();
		goundpoint=(*tit)->getGroundPoint();
		tGpt = m_MapPar->inverse(ossimDpt(goundpoint.lat, goundpoint.lon));
		tGpt.hgt = (*tit)->hgt;
		(*tit)->setGroundPoint(tGpt);
	}

	double tmp = 1.0;
	sensorModel->optimizeFit(tieGptSet);
	sensorModel->updateModel();
	sensorModel->saveState(geom);

	m_MapPar = PTR_CAST(ossimMapProjection,
		ossimMapProjectionFactory::instance()->createProjection(m_MapProjection));

	for(i = 0;i < static_cast<int>(tieGptSet.getTiePoints().size());i++)
	{
		ossimGpt ll = *tieGptSet.getTiePoints()[i];
		ossimDpt dpt = m_MapPar->forward(ll);
		ossimGpt gpt(dpt.x,dpt.y);
		tieGptSet.refTiePoints()[i]->setGroundPoint(ossimGpt(dpt.x,dpt.y,tieGptSet.getTiePoints()[i]->hgt));
	}
}

/*
void MyProject::UpdateSensorModel(vector < ossimTieLine > tieLineList,
								  ossimSensorModel* &sensorModel,
								  ossimKeywordlist& geom)
{
	ossimDpt imagepoint,cimagepoint;
	ossimGpt goundpoint,tGpt;
	int i;

	int num = static_cast<int>(tieLineList.size());

	for (i = 0;i < num;i++)
	{
		imagepoint = tieLineList[i].first.getImagePoint();
		goundpoint = tieLineList[i].first.getGroundPoint();
		tGpt = m_MapPar->inverse(ossimDpt(goundpoint.lat, goundpoint.lon),goundpoint);
		tGpt.hgt = tieLineList[i].first.hgt;
		tieLineList[i].first.setGroundPoint(tGpt);

		imagepoint = tieLineList[i].second.getImagePoint();
		goundpoint = tieLineList[i].second.getGroundPoint();
		tGpt = m_MapPar->inverse(ossimDpt(goundpoint.lat, goundpoint.lon),goundpoint);
		tGpt.hgt = tieLineList[i].second.hgt;
		tieLineList[i].second.setGroundPoint(tGpt);
	}

	sensorModel->optimizeFit(tieLineList);
	sensorModel->updateModel();
	sensorModel->saveState(geom);

	for(i = 0;i < num;i++)
	{
		imagepoint = sensorModel->m_proj->forward(tieLineList[i].first.getGroundPoint());
		tieLineList[i].first.setGroundPoint(ossimGpt(imagepoint.x,imagepoint.y,tieLineList[i].first.hgt));

		imagepoint = sensorModel->m_proj->forward(tieLineList[i].second.getGroundPoint());
		tieLineList[i].second.setGroundPoint(ossimGpt(imagepoint.x,imagepoint.y,tieLineList[i].second.hgt));
	}
}
*/

void MyProject::UpdateSensorModel(vector < ossimTieFeature > tieFeatureList,
								  ossimSensorModel* &sensorModel,
								  ossimKeywordlist& geom)
{
	ossimDpt imagepoint,cimagepoint;
	ossimGpt goundpoint,tGpt;
	int i,j;

	int num = static_cast<int>(tieFeatureList.size());

	for (i = 0;i < num;i++)
	{
		for(j = 0;j < (int)tieFeatureList[i].getGroundFeature().m_Points.size();j++)
		{
			goundpoint = tieFeatureList[i].getGroundFeature().m_Points[j];
			tGpt = m_MapPar->inverse(ossimDpt(goundpoint.lat, goundpoint.lon));
			tGpt.hgt = tieFeatureList[i].getGroundFeature().m_Points[j].hgt;
			tieFeatureList[i].refGroundFeature().m_Points[j] = tGpt;
		}
	}

	//用clock()来计时  毫秒
	clock_t  clockBegin, clockEnd;
	clockBegin = clock();
	sensorModel->optimizeFit(tieFeatureList);
	clockEnd = clock();
	printf("模型优化耗时：%.3lf秒\n", (clockEnd - clockBegin)*1e-3);
	// 计时结束
	sensorModel->updateModel();
	sensorModel->saveState(geom);

	for (i = 0;i < num;i++)
	{
		for(j = 0;j < (int)tieFeatureList[i].getGroundFeature().m_Points.size();j++)
		{
			imagepoint = sensorModel->m_proj->forward(tieFeatureList[i].getGroundFeature().m_Points[j]);
			tieFeatureList[i].refGroundFeature().m_Points[j] = ossimGpt(imagepoint.x,imagepoint.y,tieFeatureList[i].getGroundFeature().m_Points[j].hgt);
		}
	}
}

NEWMAT::ColumnVector MyProject::CalcResidue(ossimSensorModel* sensorModel,ossimTieGptSet gptSet, bool bPixel/*=false*/)
{
	ossimDpt imagepoint,cimagepoint;
	ossimGpt goundpoint,tGpt;
	ossimDpt residue1,residue2;
	int i;

	int num = static_cast<int>(gptSet.size());
	NEWMAT::ColumnVector residue;
	//residue.ReSize(num*3);
	residue.ReSize(num*2);

	vector<ossimRefPtr<ossimTieGpt> >& theTPV = gptSet.refTiePoints();

	vector<ossimRefPtr<ossimTieGpt> >::iterator tit;


	if(bPixel)
	{
		for(i = 0;i < num;++i)
		{
			ossimGpt gpt = gptSet.getTiePoints()[i]->getGroundPoint();
			if (sensorModel->m_proj)
			{
				gpt = sensorModel->m_proj->inverse(ossimDpt(gpt.lat, gpt.lon));
				gpt.hgt = gptSet.getTiePoints()[i]->getGroundPoint().hgt;
			}
			ossimDpt dpt = sensorModel->forward(gpt);
			ossimDpt imagepoint = gptSet.getTiePoints()[i]->getImagePoint() - sensorModel->forward(gpt);

			residue.element(i * 2 + 0) = imagepoint.x;
			residue.element(i * 2 + 1) = imagepoint.y;
		}
	}
	else
	{
		for(i = 0;i < num;++i)
		{
			imagepoint = gptSet.getTiePoints()[i]->getImagePoint();
			//tGpt.hgt = gptSet.getTiePoints()[i]->hgt;
			sensorModel->lineSampleToWorld(imagepoint,tGpt);
			//ossimDpt checkDpt;
			//sensorModel->worldToLineSample(tGpt, checkDpt);
			//checkDpt = checkDpt - imagepoint;
			if(sensorModel->m_proj) tGpt.datum(sensorModel->m_proj->getDatum());	//loong

			if (sensorModel->m_proj)
			{
				residue1 = sensorModel->m_proj->forward(tGpt);
			}
			else
			{
				residue1 = ossimDpt(tGpt.lat, tGpt.lon);
			}
			ossimGpt gpt = gptSet.getTiePoints()[i]->getGroundPoint();
			residue2 = ossimDpt(gpt.lat,gpt.lon);

			residue.element(i * 2 + 0) = residue2.x - residue1.x;
			residue.element(i * 2 + 1) = residue2.y - residue1.y;
			//residue.element(i * 3 + 2) = gpt.hgt - tGpt.hgt;
		}
	}

	return residue;
}

bool MyProject::GetElevations(ossimTieGptSet* &ctrlSet, double defaultElev/* = 0.0*/, bool forceDefault/* = false*/)
{
	return get_elevation(ctrlSet, m_MapProjection, defaultElev);
	vector<ossimRefPtr<ossimTieGpt> >&    theGcp =  ctrlSet->refTiePoints();
	vector<ossimRefPtr<ossimTieGpt> >::iterator iter,tit;

	ossimMapProjection* theMapProjection = m_MapPar;

	if(forceDefault || theMgr == NULL || theMapProjection==NULL)
	{
		if(theMgr == NULL)
			cout<<"高程源还未选取！"<<endl;
		if(theMapProjection==NULL)
			cout<<"您投影设置错误或没有设置投影信息！"<<endl;
		ossimGpt tGpt;
		for (iter = theGcp.begin() ; iter != theGcp.end() ; ++iter)
		{
			(*iter)->hgt = defaultElev;
		}
		cout<<"高程源还未选取！"<<endl;
		return false;
	}

	ossimGpt tGpt;
	for (iter = theGcp.begin() ; iter != theGcp.end() && iter->valid() ;)
	{

		tGpt = theMapProjection->inverse(ossimDpt((*iter)->lat,(*iter)->lon));

		(*iter)->hgt=theMgr->getHeightAboveEllipsoid(tGpt);
		if(ossim::isnan((*iter)->hgt) || (*iter)->hgt == 0.0)
			//if(ossim::isnan((*iter)->hgt) || (*iter)->hgt == 0.0)
		{
			// 如果为空，则去掉该点
			theGcp.erase(iter);
			// 如果为空，则赋为平均值
			//(*iter)->hgt = default_hgt;
			++iter;
		}
		else{
			++iter;
		}
	}
	return true;
}

bool MyProject::GetElevations(vector < ossimBlockTieGpt > &tiePointList, double defaultElev/* = 0.0*/, bool forceDefault/* = false*/)
{
	ossimMapProjection* theMapProjection = m_MapPar;


	if(forceDefault || theMgr == NULL || theMapProjection==NULL)
	{
		if(theMgr == NULL)
			cout<<"高程源还未选取！"<<endl;
		if(theMapProjection==NULL)
			cout<<"您投影设置错误或没有设置投影信息！"<<endl;

		ossimGpt tGpt;
		vector<ossimBlockTieGpt>::iterator iter;
		for (iter = tiePointList.begin() ; iter != tiePointList.end() ; ++iter)
		{
			iter->hgt = defaultElev;
		}
		cout<<"高程源还未选取！"<<endl;
		return false;
	}


	ossimGpt tGpt;
	vector<ossimBlockTieGpt>::iterator iter;
	for (iter = tiePointList.begin() ; iter != tiePointList.end() ; ++iter)
	{
		if(0 == iter->m_nType) continue;

		tGpt = theMapProjection->inverse(ossimDpt(iter->lat,iter->lon));

		iter->hgt=theMgr->getHeightAboveEllipsoid(tGpt);

	}

	return true;
}

//bool MyProject::GetElevations(vector < ossimTieLine > &tieLineList, double defaultElev/* = 0.0*/, bool forceDefault/* = false*/)
//{
//	int nsize = (int)tieLineList.size();
//	int i;
//
//	ossimMapProjection* theMapProjection = m_MapPar;
//
//	if(forceDefault || theMgr == NULL || theMapProjection==NULL)
//	{
//		if(theMgr == NULL)
//			cout<<"高程源还未选取！"<<endl;
//		if(theMapProjection==NULL)
//			cout<<"您投影设置错误或没有设置投影信息！"<<endl;
//
//		ossimGpt tGpt;
//		for(i = 0;i < nsize;i++)
//		{
//			tGpt = theMapProjection->inverse(ossimDpt(tieLineList[i].first.lat,tieLineList[i].first.lon),tieLineList[i].first.getGroundPoint());
//			tieLineList[i].first.hgt=defaultElev;
//
//			tGpt = theMapProjection->inverse(ossimDpt(tieLineList[i].second.lat,tieLineList[i].second.lon),tieLineList[i].second.getGroundPoint());
//			tieLineList[i].second.hgt=defaultElev;
//		}
//		return false;
//	}
//
//	ossimGpt tGpt;
//	for(i = 0;i < nsize;i++)
//	{
//		tGpt = theMapProjection->inverse(ossimDpt(tieLineList[i].first.lat,tieLineList[i].first.lon),tieLineList[i].first.getGroundPoint());
//		tieLineList[i].first.hgt=theMgr->getHeightAboveEllipsoid(tGpt);
//
//		tGpt = theMapProjection->inverse(ossimDpt(tieLineList[i].second.lat,tieLineList[i].second.lon),tieLineList[i].second.getGroundPoint());
//		tieLineList[i].second.hgt=theMgr->getHeightAboveEllipsoid(tGpt);
//	}
//
//	return true;
//}

bool MyProject::InitiateSensorModel(ossimFilename sourcefile/* = ""*/)
{
	if(sourcefile == "")
		sourcefile = m_ImgFileNameUnc;
	SensorType nSensorType = getSensorType(sourcefile);

	//添加高程
	ossimElevManager* theMgr;
	theMgr = ossimElevManager::instance();
	//theMgr->loadElevationPath(m_DemPath);

	switch(m_ModelType)
	{
	case OrbitType:
		{
			switch(nSensorType)
			{
			case modelLandsat5:
				{
					ossimImageHandler *handler   = ossimImageHandlerRegistry::instance()->open(sourcefile);
					if(!handler) return false;

					ossimKeywordlist kwlnew,kk;
					ossimDpt imagesize;

					m_sensorModel = new ossimLandSatModel(sourcefile);
					m_MapPar =NULL;
					m_MapPar = PTR_CAST(ossimMapProjection,
						ossimMapProjectionFactory::instance()->createProjection(m_MapProjection));
					if (!m_MapPar) return false;
					m_sensorModel->m_proj = m_MapPar;
					m_sensorModel->saveState(geom);
					break;
				}
			case modelSpot5:
				{
					ossimImageHandler *handler   = ossimImageHandlerRegistry::instance()->open(sourcefile);

					ossimFilename strPath = sourcefile.path();
					ossimString spot5Header = strPath + "\\metadata.dim";
					m_sensorModel = new ossimSpot5Model();
					m_sensorModel->setupOptimizer(spot5Header);
					m_MapPar = PTR_CAST(ossimMapProjection,
						ossimMapProjectionFactory::instance()->createProjection(m_MapProjection));
					if (!m_MapPar) return false;
					m_sensorModel->m_proj = m_MapPar;

					m_sensorModel->saveState(geom);
					break;
				}
			//case modelTheos:
			//	{
			//		ossimImageHandler *handler   = ossimImageHandlerRegistry::instance()->open(sourcefile);

			//		QString strPath(sourcefile.c_str());
			//		strPath = QBeforeLast(strPath, '\\') + "\\";
			//		ossimString theosHeader = (strPath + "metadata.dim").toLatin1();

			//		m_sensorModel = new ossimTheosModel();
			//		m_sensorModel->setupOptimizer(theosHeader);
			//		m_MapPar = PTR_CAST(ossimMapProjection,
			//			ossimMapProjectionFactory::instance()->createProjection(m_MapProjection));
			//		if (!m_MapPar) return false;
			//		m_sensorModel->m_proj = m_MapPar;

			//		m_sensorModel->saveState(geom);
			//		break;
			//	}
			case modelLandsat7:
				{
					ossimImageHandler *handler   = ossimImageHandlerRegistry::instance()->open(sourcefile);
					if(!handler) return false;

					ossimKeywordlist kwlnew,kk;
					ossimDpt imagesize;

					m_sensorModel = new ossimLandSatModel(sourcefile);

					m_MapPar = PTR_CAST(ossimMapProjection,
						ossimMapProjectionFactory::instance()->createProjection(m_MapProjection));

					m_sensorModel->m_proj =  m_MapPar;
					m_sensorModel->saveState(geom);
					break;
				}
			case modelHJ1:
				{
					ossimImageHandler *handler   = ossimImageHandlerRegistry::instance()->open(sourcefile);

					ossimFilename strPath = sourcefile.path();
					ossimString descfile = strPath + "\\desc.xml";

					m_sensorModel = new ossimplugins::ossimHj1Model();
					m_sensorModel->setupOptimizer(descfile);
					m_MapPar = PTR_CAST(ossimMapProjection,
						ossimMapProjectionFactory::instance()->createProjection(m_MapProjection));
					if (!m_MapPar) return false;
					m_sensorModel->m_proj = m_MapPar;

					m_sensorModel->saveState(geom);
					break;
				}
			default:
				break;
			}
			break;
		}
	case RPCType:
		{
			//if(modelAlosAVNIR2_1B2 == nSensorType)
			//{
			//	ossimFilename imgPath;
			//	if(!ossimFilename(m_ImgFileNameUnc).isDir())
			//		imgPath = m_ImgFileNameUnc.path();
			//	else
			//		imgPath = m_ImgFileNameUnc;
			//	AlosAVNIR2 alosUti(imgPath);
			//	if(!alosUti.getInitState())
			//	{
			//		cout<<"warning: Alos AVNIR2数据\""<<m_ImgFileNameUnc<<"\"初始化失败！"<<endl;
			//		return false;
			//	}
			//	m_ImgFileNameUnc = alosUti.m_FileTIF;

			//	ossimRpcProjection rpcProjection;
			//	ossimRpcModel::rpcModelStruct rpcStruct;
			//	readRPCFile(alosUti.m_FileRPC, rpcStruct);
			//	ossimRpcModel *rpcModel = new ossimRpcModel;
			//	rpcModel->setAttributes(rpcStruct);
			//	m_sensorModel = rpcModel;
			//}
			//else if(modelAlosPRISM_1B2 == nSensorType)
			//{
			//	ossimFilename imgPath;
			//	if(!ossimFilename(m_ImgFileNameUnc).isDir())
			//		imgPath = m_ImgFileNameUnc.path();
			//	else
			//		imgPath = m_ImgFileNameUnc;
			//	AlosPRISM alosUti(imgPath);
			//	if(!alosUti.getInitState())
			//	{
			//		cout<<"warning: Alos PRISM数据\""<<imgPath<<"\"初始化失败！"<<endl;
			//		return false;
			//	}
			//	m_ImgFileNameUnc = alosUti.m_FileTIF;

			//	ossimRpcProjection rpcProjection;
			//	ossimRpcModel::rpcModelStruct rpcStruct;
			//	readRPCFile(alosUti.m_FileRPC, rpcStruct);
			//	ossimRpcModel *rpcModel = new ossimRpcModel;
			//	rpcModel->setAttributes(rpcStruct);
			//	m_sensorModel = rpcModel;
			//}
			//else
			//{
				ossimRpcSolver *solver = new ossimRpcSolver(true, false);
				vector < ossimDpt > imagePoints;
				vector < ossimGpt > groundControlPoints;
				m_MapPar =NULL;
				m_MapPar = PTR_CAST(ossimMapProjection,
					ossimMapProjectionFactory::instance()->createProjection(m_MapProjection));
				if (!m_MapPar) return false;

			//}
			//int num = static_cast<int>(m_OptCtrlGptSet->getTiePoints().size());
			//ossimGpt tGpt;
			//for(int i = 0;i < num;i++)
			//{

			//	tGpt = m_MapPar->inverse(ossimDpt(m_OptCtrlGptSet->getTiePoints()[i]->getGroundPoint().lat,
			//		m_OptCtrlGptSet->getTiePoints()[i]->getGroundPoint().lon),m_OptCtrlGptSet->getTiePoints()[i]->getGroundPoint());
			//	tGpt.hgt = m_OptCtrlGptSet->getTiePoints()[i]->getGroundPoint().hgt;
			//	groundControlPoints.push_back(tGpt);
			//	imagePoints.push_back(m_OptCtrlGptSet->getTiePoints()[i]->getImagePoint());
			//}
			//solver.solveCoefficients(imagePoints,groundControlPoints);

			//m_sensorModel = solver.createRpcModel();
			//m_sensorModel->m_proj = m_MapPar;
			//m_sensorModel->saveState(geom);

			break;
		}
	case PolynomialType:
		{
			ossimDpt imagepoint,cimagepoint;
			ossimGpt goundpoint,tGpt;
			//int i;

			int num = static_cast<int>(m_CtrlGptSet->size());
			m_MapPar =NULL;
			m_MapPar = PTR_CAST(ossimMapProjection,
				ossimMapProjectionFactory::instance()->createProjection(m_MapProjection));
			if (!m_MapPar) return false;
			m_sensorModel = (ossimSensorModel*)new ossimPolynomProjection;
			m_sensorModel->setupOptimizer(m_PolyDegree);
			m_sensorModel->m_proj = m_MapPar;

			//vector<ossimRefPtr<ossimTieGpt> >& theTPV = m_CtrlGptSet->refTiePoints();
			//vector<ossimRefPtr<ossimTieGpt> >::iterator tit;
			//for (tit = theTPV.begin() ; tit != theTPV.end() ; ++tit)
			//{
			//	imagepoint=(*tit)->getImagePoint();
			//	goundpoint=(*tit)->getGroundPoint();
			//	tGpt = m_MapPar->inverse(ossimDpt(goundpoint.lat,goundpoint.lon));
			//	tGpt.hgt = (*tit)->hgt;
			//	(*tit)->setGroundPoint(tGpt);
			//}
			//m_sensorModel = (ossimSensorModel*)new ossimPolynomProjection;
			////m_sensorModel->setupOptimizer("1 x y x2 xy y2 x3 y3 xy2 x2y z xz yz");
			//m_sensorModel->setupOptimizer(m_PolyDegree);
			//m_sensorModel->optimizeFit(*m_CtrlGptSet);
			//m_sensorModel->saveState(geom);
			////ossimPolynomProjection* tempProj = new ossimPolynomProjection;
			////tempProj->setupOptimizer("1 x y x2 xy y2 x3 y3 xy2 x2y z xz yz");
			////tempProj->optimizeFit(*m_ProjectS.m_OptCtrlGptSet);
			////tempProj->saveState(m_ProjectS.geom);

			//for(int i = 0;i < static_cast<int>(m_CtrlGptSet->getTiePoints().size());i++)
			//{
			//	ossimDpt dpt = m_MapPar->forward(*m_CtrlGptSet->getTiePoints()[i]);
			//	ossimGpt gpt(dpt.x,dpt.y);
			//	m_CtrlGptSet->refTiePoints()[i]->setGroundPoint(ossimGpt(dpt.x,dpt.y,m_CtrlGptSet->getTiePoints()[i]->hgt));
			//}
			//m_sensorModel->m_proj = m_MapPar;
			//m_sensorModel->saveState(geom);
			break;
		}

	default:
		return false;
	}

	return true;
}

bool MyProject::OutputReport(ossimFilename reportfile, ossimSensorModel* sensorModel, ossimTieGptSet* ctrlSet, ossimTieGptSet* chkSet, bool bPixel/*=false*/)
{
	int i;
	fstream fs;
	fs.open(reportfile.c_str(), ios_base::out);
	fs.setf(ios::fixed, ios::floatfield);
	fs.precision(2);

	if(bPixel)
	{
		fs<<"残差报告\n\n"<<"残差单位：像素\n";
	}
	else
	{
		fs<<"残差报告\n\n"<<"残差单位：米\n";
	}

	NEWMAT::ColumnVector residue1 = CalcResidue(sensorModel, *ctrlSet, bPixel);
	NEWMAT::ColumnVector residue2 = CalcResidue(sensorModel, *chkSet, bPixel);

	fs<<"控制点点数："<<ctrlSet->size();
	if(static_cast<int>(ctrlSet->size()) > 0)
	{
		double ResidueX = 0.0;
		double ResidueY = 0.0;
		double ResidueZ = 0.0;

		for(i = 0;i < static_cast<int>(ctrlSet->size());i++)
		{
			ResidueX += residue1.element(i * 2 + 0) * residue1.element(i * 2 + 0);
			ResidueY += residue1.element(i * 2 + 1) * residue1.element(i * 2 + 1);
			//ResidueZ += residue1.element(i * 3 + 2) * residue1.element(i * 3 + 2);
		}
		ResidueX = sqrt(ResidueX / static_cast<int>(ctrlSet->size()));
		ResidueY = sqrt(ResidueY / static_cast<int>(ctrlSet->size()));
		ResidueZ = sqrt(ResidueZ / static_cast<int>(ctrlSet->size()));
		fs<<"\t\t"<<"均方差：X "<<ResidueX<<"\tY "<<ResidueY<<"\tZ "<<ResidueZ<<endl;
	}
	else
		fs<<endl;

	fs<<"检查点点数："<<chkSet->size();
	if(static_cast<int>(chkSet->size()) > 0)
	{
		double ResidueX = 0.0;
		double ResidueY = 0.0;
		double ResidueZ = 0.0;

		for(i = 0;i < static_cast<int>(chkSet->size());i++)
		{
			ResidueX += residue2.element(i * 2 + 0) * residue2.element(i * 2 + 0);
			ResidueY += residue2.element(i * 2 + 1) * residue2.element(i * 2 + 1);
			//ResidueZ += residue2.element(i * 3 + 2) * residue2.element(i * 3 + 2);
		}
		ResidueX = sqrt(ResidueX / static_cast<int>(chkSet->size()));
		ResidueY = sqrt(ResidueY / static_cast<int>(chkSet->size()));
		ResidueZ = sqrt(ResidueZ / static_cast<int>(chkSet->size()));
		fs<<"\t\t"<<"均方差：X "<<ResidueX<<"\tY "<<ResidueY<<"\tZ "<<ResidueZ<<endl;
	}
	else
		fs<<endl;

	fs<<"清单："<<endl;
	fs<<"用于计算模型的控制点点数："<<ctrlSet->size()<<endl;
	fs<<setw(10)<<"标示"<<"\t"
		<<setw(10)<<"误差"<<"\t"
		<<setw(10)<<"残差X"<<"\t"
		<<setw(10)<<"残差Y"<<"\t"
		<<setw(8)<<"类型"<<"\t"
		<<setw(10)<<"高程"<<"\t"
		<<setw(15)<<"实际X"<<"\t"
		<<setw(15)<<"实际Y"<<"\t"
		<<setw(15)<<"对照X"<<"\t"
		<<setw(15)<<"对照Y"<<"\t"<<endl;


	//residue = CalcResidue(*m_CtrTieGptSet);
	for(i = 0;i < static_cast<int>(ctrlSet->size());i++)
	{
		fs<<setw(10)<<ctrlSet->getTiePoints()[i]->GcpNumberID<<"\t"
			<<setw(10)<<sqrt(residue1.element(i * 2 + 0)*residue1.element(i * 2 + 0) + residue1.element(i * 2 + 1)*residue1.element(i * 2 + 1))<<"\t"
			<<setw(10)<<residue1.element(i * 2 + 0)<<"\t"
			<<setw(10)<<residue1.element(i * 2 + 1)<<"\t"
			<<setw(8)<<"GCP"<<"\t"
			<<setw(10)<<ctrlSet->getTiePoints()[i]->getGroundPoint().hgt<<"\t"
			<<setw(15)<<ctrlSet->getTiePoints()[i]->getGroundPoint().lat<<"\t"
			<<setw(15)<<ctrlSet->getTiePoints()[i]->getGroundPoint().lon<<"\t"
			<<setw(15)<<ctrlSet->getTiePoints()[i]->getGroundPoint().lat + residue1.element(i * 2 + 0)<<"\t"
			<<setw(15)<<ctrlSet->getTiePoints()[i]->getGroundPoint().lon + residue1.element(i * 2 + 1)<<"\n";
	}

	//fs<<endl;

	//residue = CalcResidue(*m_ChkTieGptSet);
	for(i = 0;i < static_cast<int>(chkSet->size());i++)
	{
		fs<<setw(10)<<chkSet->getTiePoints()[i]->GcpNumberID<<"\t"
			<<setw(10)<<sqrt(residue2.element(i * 2 + 0)*residue2.element(i * 2 + 0) + residue2.element(i * 2 + 1)*residue2.element(i * 2 + 1))<<"\t"
			<<setw(10)<<residue2.element(i * 2 + 0)<<"\t"
			<<setw(10)<<residue2.element(i * 2 + 1)<<"\t"
			//<<residue2.element(i * 3 + 2)<<"\t"
			<<setw(8)<<"CHECK"<<"\t"
			<<setw(10)<<chkSet->getTiePoints()[i]->getGroundPoint().hgt<<"\t"
			<<setw(15)<<chkSet->getTiePoints()[i]->getGroundPoint().lat<<"\t"
			<<setw(15)<<chkSet->getTiePoints()[i]->getGroundPoint().lon<<"\t"
			<<setw(15)<<chkSet->getTiePoints()[i]->getGroundPoint().lat + residue2.element(i * 2 + 0)<<"\t"
			<<setw(15)<<chkSet->getTiePoints()[i]->getGroundPoint().lon + residue2.element(i * 2 + 1)<<"\n";
	}

	fs.close();

	return true;
}

bool MyProject::OutputReport(fstream& fs, ossimSensorModel* sensorModel, ossimTieGptSet* ctrlSet, ossimTieGptSet* chkSet)
{
	int i;

	fs<<"残差报告\n\n"<<"残差单位：米\n";

	NEWMAT::ColumnVector residue1 = CalcResidue(sensorModel, *ctrlSet);
	NEWMAT::ColumnVector residue2 = CalcResidue(sensorModel, *chkSet);

	fs<<"控制点点数："<<ctrlSet->size();
	if(static_cast<int>(ctrlSet->size()) > 0)
	{
		double ResidueX = 0.0;
		double ResidueY = 0.0;
		double ResidueZ = 0.0;

		for(i = 0;i < static_cast<int>(ctrlSet->size());i++)
		{
			ResidueX += residue1.element(i * 2 + 0) * residue1.element(i * 2 + 0);
			ResidueY += residue1.element(i * 2 + 1) * residue1.element(i * 2 + 1);
			//ResidueZ += residue1.element(i * 3 + 2) * residue1.element(i * 3 + 2);
		}
		ResidueX = sqrt(ResidueX / static_cast<int>(ctrlSet->size()));
		ResidueY = sqrt(ResidueY / static_cast<int>(ctrlSet->size()));
		ResidueZ = sqrt(ResidueZ / static_cast<int>(ctrlSet->size()));
		fs<<"\t\t"<<"均方差：X "<<ResidueX<<"\tY "<<ResidueY<<"\tZ "<<ResidueZ<<endl;
	}
	else
		fs<<endl;

	fs<<"检查点点数："<<chkSet->size();
	if(static_cast<int>(chkSet->size()) > 0)
	{
		double ResidueX = 0.0;
		double ResidueY = 0.0;
		double ResidueZ = 0.0;

		for(i = 0;i < static_cast<int>(chkSet->size());i++)
		{
			ResidueX += residue2.element(i * 2 + 0) * residue2.element(i * 2 + 0);
			ResidueY += residue2.element(i * 2 + 1) * residue2.element(i * 2 + 1);
			//ResidueZ += residue2.element(i * 3 + 2) * residue2.element(i * 3 + 2);
		}
		ResidueX = sqrt(ResidueX / static_cast<int>(chkSet->size()));
		ResidueY = sqrt(ResidueY / static_cast<int>(chkSet->size()));
		ResidueZ = sqrt(ResidueZ / static_cast<int>(chkSet->size()));
		fs<<"\t\t"<<"均方差：X "<<ResidueX<<"\tY "<<ResidueY<<"\tZ "<<ResidueZ<<endl;
	}
	else
		fs<<endl;

	fs<<"清单："<<endl;

	fs<<"标示\t"<<"误差\t"<<"残差X\t"<<"残差Y\t"<<"类型\t"<<"高程\t"<<"实际X\t"<<"实际Y\t"<<"对照X\t"<<"对照Y\t"<<endl;

	fs<<"用于计算模型的控制点点数："<<ctrlSet->size()<<endl;

	//residue = CalcResidue(*m_CtrTieGptSet);
	for(i = 0;i < static_cast<int>(ctrlSet->size());i++)
	{
		fs<<ctrlSet->getTiePoints()[i]->GcpNumberID<<"\t"
			<<sqrt(residue1.element(i * 2 + 0)*residue1.element(i * 2 + 0) + residue1.element(i * 2 + 1)*residue1.element(i * 2 + 1))<<"\t"
			<<residue1.element(i * 2 + 0)<<"\t"
			<<residue1.element(i * 2 + 1)<<"\t"
			//<<residue1.element(i * 3 + 2)<<"\t"
			<<"GCP"<<"\t"
			<<ctrlSet->getTiePoints()[i]->getGroundPoint().hgt<<"\t"
			<<ctrlSet->getTiePoints()[i]->getGroundPoint().lat<<"\t"
			<<ctrlSet->getTiePoints()[i]->getGroundPoint().lon<<"\t"
			<<ctrlSet->getTiePoints()[i]->getGroundPoint().lat + residue1.element(i * 2 + 0)<<"\t"
			<<ctrlSet->getTiePoints()[i]->getGroundPoint().lon + residue1.element(i * 2 + 1)<<"\n";
	}

	//fs<<endl;

	//residue = CalcResidue(*m_ChkTieGptSet);
	for(i = 0;i < static_cast<int>(chkSet->size());i++)
	{
		fs<<chkSet->getTiePoints()[i]->GcpNumberID<<"\t"
			<<sqrt(residue2.element(i * 2 + 0)*residue2.element(i * 2 + 0) + residue2.element(i * 2 + 1)*residue2.element(i * 2 + 1))<<"\t"
			<<residue2.element(i * 2 + 0)<<"\t"
			<<residue2.element(i * 2 + 1)<<"\t"
			//<<residue2.element(i * 3 + 2)<<"\t"
			<<"CHECK"<<"\t"
			<<chkSet->getTiePoints()[i]->getGroundPoint().hgt<<"\t"
			<<chkSet->getTiePoints()[i]->getGroundPoint().lat<<"\t"
			<<chkSet->getTiePoints()[i]->getGroundPoint().lon<<"\t"
			<<chkSet->getTiePoints()[i]->getGroundPoint().lat + residue2.element(i * 2 + 0)<<"\t"
			<<chkSet->getTiePoints()[i]->getGroundPoint().lon + residue2.element(i * 2 + 1)<<"\n";
	}

	return true;
}


bool MyProject::Orthograph(ossimFilename outfile)
{
	ossimImageHandler *handler   = ossimImageHandlerRegistry::instance()->open(m_ImgFileNameUnc);
	if(!handler) return false;   //应该弹出警告对话框

	ossimRefPtr<ossimImageGeometry> imageGeom = new ossimImageGeometry;
	imageGeom->setProjection(m_sensorModel);
	//imageGeom->loadState(geom);

	handler->setImageGeometry(imageGeom.get());//1128
	//cout<<geom<<endl;
	ossimKeywordlist tt_geom;

	//SensorType sensorType = getSensorType(m_ImgFileNameUnc);

	m_ImgFileNameout = outfile;
	

	ossimDpt imagesize(handler->getImageRectangle().width(), handler->getImageRectangle().height());
	ossimImageRenderer* renderer = new ossimImageRenderer;
	ossimPolyCutter* theCutter;
	ossimBandSelector* theBandSelector;
	vector<ossimDpt> polygon;
	ossimIrect bound,boundw;
	theCutter = new ossimPolyCutter;
	///////////////////以下四行应该从界面取得//
	if (endpixel == 0)
	{
		endpixel = imagesize.x - 1;
	}
	if (endline == 0)
	{
		endline = imagesize.y - 1;
	}
	//starline	=	40000;
	//starpixel	=	5000;
	//endline		=	45000;
	//endpixel	=	8000;
	//starline	=	34000;
	//starpixel	=	0;
	//endline		=	46000;
	//endpixel	=	11000;
	//starline	=	40000;
	//starpixel	=	6000;
	//endline		=	46000;
	//endpixel	=	11000;

	//starline	=	40000;
	//starpixel	=	1000;
	//endline		=	46000;
	//endpixel	=	11000;

	//starline	=	5000;//
	//starpixel	=	5000;//
	//endline		=	9000;//
	//endpixel	=	9000;//
	//starline	=	0000;//
	//starpixel	=	0000;//
	//endline		=	4000;//
	//endpixel	=	4000;//
	//starline	=	0;
	//starpixel	=	0;
	//endline		=	0;
	//endpixel	=	0;
	//starline	=	30000;
	//starpixel	=	0000;
	//endline		=	33000;
	//endpixel	=	12000;
	//starline	=	35000;
	//starpixel	=	500;
	//endline		=	35500;
	//endpixel	=	3500;
	theBandSelector = new ossimBandSelector;
	theBandSelector->connectMyInputTo(0, handler);
	if (!m_OutBandList.empty())
	{
		theBandSelector->setOutputBandList(m_OutBandList);
	}
	else
	{
		int nBands = handler->getNumberOfInputBands();
		std::vector<ossim_uint32> bandList;
		for (int i=0;i<nBands;++i)
		{
			bandList.push_back(i);
		}
		theBandSelector->setOutputBandList(bandList);
	}


	ossimDpt ps(starpixel,starline),p2(endpixel,starline),p3(endpixel,endline),p4(starpixel,endline),p5(starpixel,starline);

	polygon.push_back(ps);
	polygon.push_back(p2);
	polygon.push_back(p3);
	polygon.push_back(p4);
	polygon.push_back(p5);
	theCutter->connectMyInputTo(theBandSelector);
	theCutter->setPolygon(polygon);
	theCutter->setCutType(ossimPolyCutter::ossimPolyCutterCutType::OSSIM_POLY_NULL_OUTSIDE);
	theCutter->setNumberOfPolygons(1);



	//renderer->getResampler()->setFilterType(filter_type.c_str());////////////////应该从界面取得//

	if(m_SampleType.contains("NEAREST_NEIGHBOR"))  renderer->getResampler()->setFilterType(ossimFilterResampler::ossimFilterResamplerType::ossimFilterResampler_NEAREST_NEIGHBOR);
	if(m_SampleType.contains("BILINEAR"))  renderer->getResampler()->setFilterType(ossimFilterResampler::ossimFilterResamplerType::ossimFilterResampler_BILINEAR);
	if(m_SampleType.contains("BICUBIC"))  renderer->getResampler()->setFilterType(ossimFilterResampler::ossimFilterResamplerType::ossimFilterResampler_CUBIC);

	ossimRefPtr<ossimImageFileWriter> writer = ossimImageWriterFactoryRegistry::instance()->
		createWriterFromExtension( m_ImgFileNameout.ext() );
	if(writer==NULL) return false;
	tt_geom.clear();
	writer->saveState(tt_geom);
	tt_geom.add("",ossimKeywordNames::PIXEL_TYPE_KW,"PIXEL_IS_AREA",true);
	writer->loadState(tt_geom);

	renderer->connectMyInputTo(theCutter);
	renderer->setView(m_MapPar);

	m_progress = new myOutProgress(0,true);
	writer->addListener(m_progress);

	writer->setFilename(m_ImgFileNameout);

	int nThreads = OpenThreads::GetNumberOfProcessors() * 2;
	ossimRefPtr<ossimMultiThreadSequencer> sequencer = new ossimMultiThreadSequencer(0, nThreads);
	writer->changeSequencer(sequencer.get());
	writer->connectMyInputTo(0,renderer);
	//ossimIrect bounding = handler->getBoundingRect();
	//writer->setAreaOfInterest(bounding);

	//bool bResult = writer->execute();

	if(!writer->execute())
	{
		writer->removeListener(m_progress);
		writer->disconnectAllInputs();
		renderer->disconnectAllInputs();
		theCutter->disconnectAllInputs();
		//delete handler;
		//handler->close();
		//delete handler;
		return false;
	}

	writer->disableListener();
	writer->removeListener(m_progress);
	writer->disconnectAllInputs();
	renderer->disconnectAllInputs();
	theCutter->disconnectAllInputs();
	//delete writer;
	//theBandSelector->disconnectAllInputs();
	//delete theBandSelector;
	//delete m_progress;

	//handler->close();
	return true;
}

//bool MyProject::Orthograph(ossimFilename outfile)
//{
//	ossimRefPtr<ossimSingleImageChain> theProductChain = new ossimSingleImageChain;
//	theProductChain->addImageHandler(m_ImgFileNameUnc, true);
//
//	//ossimImageHandler *handler   = ossimImageHandlerRegistry::instance()->open(m_ImgFileNameUnc);
//	//if(!handler) return false;   //应该弹出警告对话框
//
//	ossimRefPtr<ossimImageGeometry> imageGeom = new ossimImageGeometry;
//	imageGeom->loadState(geom);
//	//handler->setImageGeometry(imageGeom.get());//1128
//	//theProductChain->addLast(handler);
//	theProductChain->getImageHandler()->setImageGeometry(imageGeom.get());
//
//	
//	if (!m_MapPar)
//	{
//		ossimRefPtr<ossimImageGeometry> geom = theProductChain->getImageGeometry();
//		if ( geom.valid() ) 
//			m_MapPar = (ossimMapProjection*)geom->getProjection();
//	}
//
//	// Create the chain.
//	theProductChain->createRenderedChain();
//	m_SampleType = "BICUBIC";
//	if(m_SampleType.contains("NEAREST_NEIGHBOR"))  theProductChain->getImageRenderer()->getResampler()->setFilterType(ossimFilterResampler::ossimFilterResamplerType::ossimFilterResampler_NEAREST_NEIGHBOR);
//	if(m_SampleType.contains("BILINEAR"))  theProductChain->getImageRenderer()->getResampler()->setFilterType(ossimFilterResampler::ossimFilterResamplerType::ossimFilterResampler_BILINEAR);
//	if(m_SampleType.contains("BICUBIC"))  theProductChain->getImageRenderer()->getResampler()->setFilterType(ossimFilterResampler::ossimFilterResamplerType::ossimFilterResampler_CUBIC);
//
//	int nBands = theProductChain->getNumberOfInputBands();
//	m_OutBandList.clear();
//	for (int i=0;i<nBands;++i)
//	{
//		m_OutBandList.push_back(i);
//	}
//	theProductChain->setBandSelection(m_OutBandList);
//
//	ossimDpt imagesize(theProductChain->getBoundingRect().width(), 
//		theProductChain->getBoundingRect().height());//->getImageRectangle().width(), handler->getImageRectangle().height());
//	vector<ossimDpt> polygon;
//	///////////////////以下四行应该从界面取得//
//	int starline,starpixel,endpixel,endline;//
//	//starline	=	40000;
//	//starpixel	=	5000;
//	//endline		=	45000;
//	//endpixel	=	8000;
//	//starline	=	34000;
//	//starpixel	=	0;
//	//endline		=	46000;
//	//endpixel	=	11000;
//	//starline	=	40000;
//	//starpixel	=	6000;
//	//endline		=	46000;
//	//endpixel	=	11000;
//
//	//starline	=	40000;
//	//starpixel	=	1000;
//	//endline		=	46000;
//	//endpixel	=	11000;
//
//	//starline	=	5000;//
//	//starpixel	=	5000;//
//	//endline		=	9000;//
//	//endpixel	=	9000;//
//	//starline	=	0000;//
//	//starpixel	=	0000;//
//	//endline		=	4000;//
//	//endpixel	=	4000;//
//	//starline	=	0;
//	//starpixel	=	0;
//	//endline		=	0;
//	//endpixel	=	0;
//	starline	=	34000;
//	starpixel	=	0000;
//	endline		=	40000;
//	endpixel	=	10000;
//	if (0 == endpixel)
//	{
//		endpixel = imagesize.x - 1;
//	}
//	if (0 == endline)
//	{
//		endline = imagesize.y - 1;
//	}
//
//
//	ossimDpt ps(starpixel,starline),p2(endpixel,starline),p3(endpixel,endline),p4(starpixel,endline),p5(starpixel,starline);
//
//	polygon.push_back(ps);
//	polygon.push_back(p2);
//	polygon.push_back(p3);
//	polygon.push_back(p4);
//	polygon.push_back(p5);
//	ossimGeoPolyCutter* theCutter = new ossimGeoPolyCutter;
//	theCutter->setNumberOfPolygons(1);
//	theCutter->setPolygon(polygon);
//	//theProductChain->add(theCutter);
//
//
//	//cout<<geom<<endl;
//	ossimKeywordlist tt_geom;
//
//	//SensorType sensorType = getSensorType(m_ImgFileNameUnc);
//
//	m_ImgFileNameout = outfile;
//
//	ossimRefPtr<ossimImageFileWriter> writer = ossimImageWriterFactoryRegistry::instance()->
//		createWriterFromExtension( m_ImgFileNameout.ext() );
//
//	if(writer==NULL) return false;
//	
//	tt_geom.clear();
//	writer->saveState(tt_geom);
//	tt_geom.add("",ossimKeywordNames::PIXEL_TYPE_KW,"PIXEL_IS_AREA",true);
//	writer->loadState(tt_geom);
//	m_progress = new myOutProgress(0,true);
//	writer->addListener(m_progress);
//	writer->setFilename(m_ImgFileNameout);
//
//	theProductChain->getImageRenderer()->setView(m_MapPar);
//	writer->connectMyInputTo(theProductChain.get());
//	//ossimIrect bounding = handler->getBoundingRect();
//	//writer->setAreaOfInterest(bounding);
//
//
//	writer->initialize();
//	writer->setAreaOfInterest( theCutter->getBoundingRect() );
//
//	if(!writer->execute())
//	{
//		return false;
//	}
//
//	writer->removeListener(m_progress);
//	return true;
//}

NEWMAT::ColumnVector MyProject::getResidue(ossimSensorModel *sensorModel, vector< ossimTieFeature > tieFeatureList, bool useImageObs/* = true  2010.1.18 loong*/)
{
	/*ossimDpt imagepoint,cimagepoint;
	ossimGpt goundpoint,tGpt;
	int i,j;

	int num = static_cast<int>(tieFeatureList.size());

	for (i = 0;i < num;i++)
	{
		for(j = 0;j < (int)tieFeatureList[i].m_TiePoints.size();j++)
		{
			imagepoint = tieFeatureList[i].m_TiePoints[j].getImagePoint();
			goundpoint = tieFeatureList[i].m_TiePoints[j].getGroundPoint();
			tGpt = m_MapPar->inverse(ossimDpt(goundpoint.lat, goundpoint.lon),goundpoint);
			tGpt.hgt = tieFeatureList[i].m_TiePoints[j].hgt;
			tieFeatureList[i].m_TiePoints[j].setGroundPoint(tGpt);
		}
	}

	NEWMAT::ColumnVector residue = sensorModel->getResidue(tieFeatureList);

	for (i = 0;i < num;i++)
	{
		for(j = 0;j < (int)tieFeatureList[i].m_TiePoints.size();j++)
		{
			imagepoint = sensorModel->m_proj->forward(tieFeatureList[i].m_TiePoints[j].getGroundPoint());
			tieFeatureList[i].m_TiePoints[j].setGroundPoint(ossimGpt(imagepoint.x,imagepoint.y,tieFeatureList[i].m_TiePoints[j].hgt));
		}
	}*/

	NEWMAT::ColumnVector residue;
	return residue;
}
bool MyProject::readRPCFile(const char* rpcFile, ossimRpcModel::rpcModelStruct& rpcStruct)
{
	//fstream fin;
	//fin.open(rpcFile, ios_base::in);
	//string strtmp0;
	//std::getline(fin, strtmp0);
	//QString strtmp(strtmp0.c_str());

	///*
	//―――――――――――――――――――――――――――――――――――――
	//FIELD                         SIZE         VALUE RANGE         UNITS
	//―――――――――――――――――――――――――――――――――――――
	//LINE_OFF                       6      000000 to 999999        pixels
	//SAMP_OFF                       5       00000 to 99999         pixels
	//LAT_OFF                        8         ±90.0000            degrees
	//LONG_OFF                       9        ±180.0000            degrees
	//HEIGHT_OFF                     5          ±9999              meters
	//LINE_SCALE                     6      000001 to 999999        pixels
	//SAMP_SCALE                     5       00001 to 99999         pixels
	//LAT_SCALE                      8         ±90.0000            degrees
	//LONG_SCALE                     9        ±180.0000            degrees
	//HEIGHT_SCALE                   5          ±9999              meters
	//LINE_NUM_COEFF1               12       ±9.999999E±9
	//･･                         ･･            ･･
	//LINE_NUM_COEFF20              12       ±9.999999E±9
	//LINE_DEN_COEFF1               12       ±9.999999E±9
	//･･                         ･･            ･･
	//LINE_DEN_COEFF20              12       ±9.999999E±9
	//SAMP_NUM_COEFF1               12       ±9.999999E±9
	//･･                         ･･            ･･
	//SAMP_NUM_COEFF20              12       ±9.999999E±9
	//SAMP_DEN_COEFF1               12       ±9.999999E±9
	//･･                         ･･            ･･
	//SAMP_DEN_COEFF20              12       ±9.999999E±9
	//―――――――――――――――――――――――――――――――――――――
	//*/

	////ossim_float64 LINE_OFF;
	////ossim_float64 SAMP_OFF;
	////ossim_float64 LAT_OFF;
	////ossim_float64 LONG_OFF;
	////ossim_float64 HEIGHT_OFF;
	////ossim_float64 LINE_SCALE;
	////ossim_float64 SAMP_SCALE;
	////ossim_float64 LAT_SCALE;
	////ossim_float64 LONG_SCALE;
	////ossim_float64 HEIGHT_SCALE;
	////std::vector<double> LINE_NUM_COEFF;
	////std::vector<double> LINE_DEN_COEFF;
	////std::vector<double> SAMP_NUM_COEFF;
	////std::vector<double> SAMP_DEN_COEFF;
	//int nCoeff = 20;

	//int pos = 0;
	//
	//int word_width = 6;
	//rpcStruct.lineOffset = strtmp.mid(pos, word_width).toDouble();
	//pos+=word_width;

	//word_width = 5;
	//rpcStruct.sampOffset = strtmp.mid(pos, word_width).toDouble();
	//pos+=word_width;

	//word_width = 8;
	//rpcStruct.latOffset = strtmp.mid(pos, word_width).toDouble();
	//pos+=word_width;

	//word_width = 9;
	//rpcStruct.lonOffset = strtmp.mid(pos, word_width).toDouble();
	//pos+=word_width;

	//word_width = 5;
	//rpcStruct.hgtOffset = strtmp.mid(pos, word_width).toDouble();
	//pos+=word_width;

	//word_width = 6;
	//rpcStruct.lineScale = strtmp.mid(pos, word_width).toDouble();
	//pos+=word_width;

	//word_width = 5;
	//rpcStruct.sampScale = strtmp.mid(pos, word_width).toDouble();
	//pos+=word_width;

	//word_width = 8;
	//rpcStruct.latScale = strtmp.mid(pos, word_width).toDouble();
	//pos+=word_width;

	//word_width = 9;
	//rpcStruct.lonScale = strtmp.mid(pos, word_width).toDouble();
	//pos+=word_width;

	//word_width = 5;
	//rpcStruct.hgtScale = strtmp.mid(pos, word_width).toDouble();
	//pos+=word_width;



	//int i;
	//for(i = 0;i < nCoeff;i++)
	//{
	//	word_width = 12;
	//	rpcStruct.lineNumCoef[i] = strtmp.mid(pos, word_width).toDouble();
	//	pos+=word_width;
	//}
	//for(i = 0;i < nCoeff;i++)
	//{
	//	word_width = 12;
	//	rpcStruct.lineDenCoef[i] = strtmp.mid(pos, word_width).toDouble();
	//	pos+=word_width;
	//}
	//for(i = 0;i < nCoeff;i++)
	//{
	//	word_width = 12;
	//	rpcStruct.sampNumCoef[i] = strtmp.mid(pos, word_width).toDouble();
	//	pos+=word_width;
	//}
	//for(i = 0;i < nCoeff;i++)
	//{
	//	word_width = 12;
	//	rpcStruct.sampDenCoef[i] = strtmp.mid(pos, word_width).toDouble();
	//	pos+=word_width;
	//}
	//rpcStruct.type = ossimRpcModel::PolynomialType::B;

	return true;
}

/*
void xl()
{
	//获得灰度映射表
	//拉伸至-255
	double k2=(255-k3*(255-n)-m*k1)/(n-m);
	//int m0 = (int)(a + (b-a)/255.0 + k1*m/(k1-k2));
	//int n0 = (int)(b - (b-a)/255.0 + k3*(255-n)/(k3-k2));

	int m0 = (int)(a + (b-a)/255.0*m);
	int n0 = (int)(b - (b-a)/255.0*(255-n));
	BYTE bMap[256];


	for( i=0;i<256;i++)
	{
		if(i <= m0)
		{
			gray=(int)(k1*(i - 0) + 0);
		}
		else if (i>m0 && i<=n0)
		{
			gray=(int)(k2*(i-m0)*255.0/(b-a)+k1*m);
		}
		else if (i>n0)
		{			
			gray=(int)(k3*(i-n0)*255.0/(b-a)+k1*m+k2*(m-n));
		}
		bMap[i]=(BYTE)gray;
	}


	//对各象素进行灰度变换
	for(i=0;i<height;i++)
	{
		for(j=0;j<wide;j++)
		{
			new_p[wide*i+j]=bMap[(int)p[wide*i+j]];
		}
	}

}*/
bool MyProject::GetElevations(vector<ossimTieFeature> &tieFeatureList, double defaultElev/* = 0.0*/, bool forceDefault/* = false*/)
{

	ossimMapProjection* theMapProjection = m_MapPar;

	if(forceDefault || theMgr == NULL || theMapProjection==NULL)
	{
		if(theMgr == NULL)
			cout<<"高程源还未选取！"<<endl;
		if(theMapProjection==NULL)
			cout<<"您投影设置错误或没有设置投影信息！"<<endl;
		
		for(unsigned int i = 0;i < tieFeatureList.size();i++)
		{
			for(unsigned int j = 0;j < tieFeatureList[i].getImageFeature().m_Points.size();j++)
			{
				tieFeatureList[i].refGroundFeature().m_Points[j].hgt = defaultElev;
			}

		}
		cout<<"高程源还未选取！"<<endl;
		return false;
	}

	ossimGpt tGpt;
	for(unsigned int i = 0;i < tieFeatureList.size();i++)
	{
		for(unsigned int j = 0;j < tieFeatureList[i].getGroundFeature().m_Points.size();j++)
		{
			ossimGpt gpt = tieFeatureList[i].getGroundFeature().m_Points[j];
			tGpt = theMapProjection->inverse(ossimDpt(gpt.lat, gpt.lon));
			tieFeatureList[i].refGroundFeature().m_Points[j].hgt = theMgr->getHeightAboveEllipsoid(tGpt);
		}
		
	}
	return true;
}

bool MyProject::OutputReport1(ossimFilename reportfile, ossimSensorModel* sensorModel, ossimTieGptSet* ctrlSet, ossimTieGptSet* chkSet, bool bPixel/*=false*/)
{
	int i;
	fstream fs;
	fs.open(reportfile.c_str(), ios_base::out);
	fs.setf(ios::fixed, ios::floatfield);
	fs.precision(2);

	if(bPixel)
	{
		fs<<"残差报告\n\n"<<"残差单位：像素\n";
	}
	else
	{
		fs<<"残差报告\n\n"<<"残差单位：米\n";
	}

	NEWMAT::ColumnVector residue1 = CalcResidue1(sensorModel, *ctrlSet, bPixel);
	NEWMAT::ColumnVector residue2 = CalcResidue1(sensorModel, *chkSet, bPixel);

	fs<<"控制点点数："<<ctrlSet->size();
	if(static_cast<int>(ctrlSet->size()) > 0)
	{
		double ResidueX = 0.0;
		double ResidueY = 0.0;
		double ResidueZ = 0.0;

		for(i = 0;i < static_cast<int>(ctrlSet->size());i++)
		{
			ResidueX += residue1.element(i * 2 + 0) * residue1.element(i * 2 + 0);
			ResidueY += residue1.element(i * 2 + 1) * residue1.element(i * 2 + 1);
			//ResidueZ += residue1.element(i * 3 + 2) * residue1.element(i * 3 + 2);
		}
		ResidueX = sqrt(ResidueX / static_cast<int>(ctrlSet->size()));
		ResidueY = sqrt(ResidueY / static_cast<int>(ctrlSet->size()));
		ResidueZ = sqrt(ResidueZ / static_cast<int>(ctrlSet->size()));
		fs<<"\t\t"<<"均方差：X "<<ResidueX<<"\tY "<<ResidueY<<"\tZ "<<ResidueZ<<endl;
	}
	else
		fs<<endl;

	fs<<"检查点点数："<<chkSet->size();
	if(static_cast<int>(chkSet->size()) > 0)
	{
		double ResidueX = 0.0;
		double ResidueY = 0.0;
		double ResidueZ = 0.0;

		for(i = 0;i < static_cast<int>(chkSet->size());i++)
		{
			ResidueX += residue2.element(i * 2 + 0) * residue2.element(i * 2 + 0);
			ResidueY += residue2.element(i * 2 + 1) * residue2.element(i * 2 + 1);
			//ResidueZ += residue2.element(i * 3 + 2) * residue2.element(i * 3 + 2);
		}
		ResidueX = sqrt(ResidueX / static_cast<int>(chkSet->size()));
		ResidueY = sqrt(ResidueY / static_cast<int>(chkSet->size()));
		ResidueZ = sqrt(ResidueZ / static_cast<int>(chkSet->size()));
		fs<<"\t\t"<<"均方差：X "<<ResidueX<<"\tY "<<ResidueY<<"\tZ "<<ResidueZ<<endl;
	}
	else
		fs<<endl;

	fs<<"清单："<<endl;

	fs<<"标示\t"<<"误差\t"<<"残差X\t"<<"残差Y\t"<<"类型\t"<<"幅号\t"<<"实际X\t"<<"实际Y\t"<<"对照X\t"<<"对照Y\t"<<endl;

	fs<<"用于计算模型的控制点点数："<<ctrlSet->size()<<endl;

	//residue = CalcResidue(*m_CtrTieGptSet);
	for(i = 0;i < static_cast<int>(ctrlSet->size());i++)
	{
		fs<<ctrlSet->getTiePoints()[i]->GcpNumberID<<"\t"
			<<sqrt(residue1.element(i * 2 + 0)*residue1.element(i * 2 + 0) + residue1.element(i * 2 + 1)*residue1.element(i * 2 + 1))<<"\t"
			<<residue1.element(i * 2 + 0)<<"\t"
			<<residue1.element(i * 2 + 1)<<"\t"
			//<<residue1.element(i * 3 + 2)<<"\t"
			<<"GCP"<<"\t"
			<<""<<"\t"
			<<ctrlSet->getTiePoints()[i]->getGroundPoint().lat<<"\t"
			<<ctrlSet->getTiePoints()[i]->getGroundPoint().lon<<"\t"
			<<ctrlSet->getTiePoints()[i]->getGroundPoint().lat + residue1.element(i * 2 + 0)<<"\t"
			<<ctrlSet->getTiePoints()[i]->getGroundPoint().lon + residue1.element(i * 2 + 1)<<"\n";
	}

	//fs<<endl;

	//residue = CalcResidue(*m_ChkTieGptSet);
	for(i = 0;i < static_cast<int>(chkSet->size());i++)
	{
		fs<<chkSet->getTiePoints()[i]->GcpNumberID<<"\t"
			<<sqrt(residue2.element(i * 2 + 0)*residue2.element(i * 2 + 0) + residue2.element(i * 2 + 1)*residue2.element(i * 2 + 1))<<"\t"
			<<residue2.element(i * 2 + 0)<<"\t"
			<<residue2.element(i * 2 + 1)<<"\t"
			//<<residue2.element(i * 3 + 2)<<"\t"
			<<"CHECK"<<"\t"
			<<""<<"\t"
			<<chkSet->getTiePoints()[i]->getGroundPoint().lat<<"\t"
			<<chkSet->getTiePoints()[i]->getGroundPoint().lon<<"\t"
			<<chkSet->getTiePoints()[i]->getGroundPoint().lat + residue2.element(i * 2 + 0)<<"\t"
			<<chkSet->getTiePoints()[i]->getGroundPoint().lon + residue2.element(i * 2 + 1)<<"\n";
	}

	fs.close();

	return true;
}

NEWMAT::ColumnVector MyProject::CalcResidue1(ossimSensorModel* sensorModel,ossimTieGptSet gptSet, bool bPixel/*=false*/)
{
	ossimDpt imagepoint,cimagepoint;
	ossimGpt goundpoint,tGpt;
	ossimDpt residue1,residue2;
	int i;

	int num = static_cast<int>(gptSet.size());
	NEWMAT::ColumnVector residue;
	//residue.ReSize(num*3);
	residue.ReSize(num*2);

	vector<ossimRefPtr<ossimTieGpt> >& theTPV = gptSet.refTiePoints();

	vector<ossimRefPtr<ossimTieGpt> >::iterator tit;


	if(bPixel)
	{
		for(i = 0;i < num;++i)
		{
			ossimGpt gpt = gptSet.getTiePoints()[i]->getGroundPoint();
			gpt = sensorModel->m_proj->inverse(ossimDpt(gpt.lat, gpt.lon));
			gpt.hgt = gptSet.getTiePoints()[i]->getGroundPoint().hgt;
			ossimDpt imagepoint = gptSet.getTiePoints()[i]->getImagePoint() - sensorModel->forward(gpt);

			residue.element(i * 2 + 0) = imagepoint.x;
			residue.element(i * 2 + 1) = imagepoint.y;
		}
	}
	else
	{
		for(i = 0;i < num;++i)
		{
			imagepoint = gptSet.getTiePoints()[i]->getImagePoint();
			//tGpt.hgt = gptSet.getTiePoints()[i]->hgt;
			sensorModel->lineSampleToWorld(imagepoint,tGpt);
			ossimDpt dpt = sensorModel->forward(tGpt);

			residue.element(i * 2 + 0) = dpt.x - imagepoint.x;
			residue.element(i * 2 + 1) = dpt.y - imagepoint.y;
			//residue.element(i * 3 + 2) = gpt.hgt - tGpt.hgt;
		}
	}

	return residue;
}



bool MyProject::CreateL5Projection(const ossimFilename &headerFile)
{
	char strtmp[2094];
	std::vector<ossimString> strList;

	fstream os;
	os.open(headerFile.c_str(), ios_base::in);
	os.getline(strtmp, 2096);
	ossimString ossimStrTmp(strtmp);
	ossimStrTmp = ossimStrTmp.after("USGS PROJECTION PARAMETERS = ");
	ossimStrTmp = ossimStrTmp.before(" EARTH ELLIPSOID");
	strList = ossimStrTmp.split(" ");
	if(strList.size() < 5) return false;
	double centerLon = (double)floor(strList[4].toDouble() + 0.5);


	m_MapProjection.clear();
	ossimString tmp;
	char* prefix="";
	//中央纬度
	double centerLat = 0.0;	
	//投影带
	//中央经线

	m_MapProjection.add(prefix,ossimKeywordNames::ORIGIN_LATITUDE_KW, centerLat,true);
	m_MapProjection.add(prefix,ossimKeywordNames::CENTRAL_MERIDIAN_KW,centerLon,true);

	//使用椭球模型
	ossimString m_EllipsoidModel = "WE";
	const 	ossimEllipsoid * theelli=ossimEllipsoidFactory::instance()->create(m_EllipsoidModel.c_str());
	theelli->saveState(m_MapProjection, prefix);

	ossimDpt	theMetersPerPixel;
	//X方向分辨率
	theMetersPerPixel.lon = 30.0;
	//Y方向分辨率
	theMetersPerPixel.lat = 30.0;

	//设置分辨率 单位为米
	m_MapProjection.add(prefix,	ossimKeywordNames::PIXEL_SCALE_XY_KW,theMetersPerPixel.toString().c_str(),true);
	m_MapProjection.add(prefix,	ossimKeywordNames::PIXEL_SCALE_UNITS_KW,	ossimUnitTypeLut::instance()->getEntryString(OSSIM_METERS),true); 

	ossimDpt	theFalseEastingNorthing;
	//东向偏移值
	theFalseEastingNorthing.lon = 500000.0;
	//北向偏移值
	theFalseEastingNorthing.lat = 0.0;

	m_MapProjection.add(prefix,ossimKeywordNames::FALSE_EASTING_NORTHING_KW,theFalseEastingNorthing.toString().c_str(), true);
	m_MapProjection.add(prefix,	ossimKeywordNames::FALSE_EASTING_NORTHING_UNITS_KW,	ossimUnitTypeLut::instance()->getEntryString(OSSIM_METERS), true);
	bool theElevationLookupFlag=true;
	m_MapProjection.add(prefix,	ossimKeywordNames::ELEVATION_LOOKUP_FLAG_KW,	ossimString::toString(theElevationLookupFlag), true);

	//设置TM投影
	m_MapProjection.add(prefix,	ossimKeywordNames::SCALE_FACTOR_KW,tmp.toDouble(),true);
	m_MapProjection.add(prefix,	"type","ossimTransMercatorProjection",true);

	m_MapPar=PTR_CAST(ossimMapProjection,
		ossimMapProjectionFactory::instance()->createProjection(m_MapProjection));
	if(m_MapPar==NULL)
	{
		cout<<"您的投影信息设置有误！"<<endl;
		return false;
	}
	return true;
}