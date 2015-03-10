#include "ZY3QVProcReader.h"
#include "gdal_priv.h"
#include "cpl_string.h"
#include "strUtil.h"
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <QDir>
#include <QXmlStreamWriter>
#include <QMapIterator>
using namespace std;


const int BUFFER_SIZE = 16384;

ZY3QVProcReader::ZY3QVProcReader()
	:time_ready(false)
{

}

bool ZY3QVProcReader::writeRPC(const char* filename, const RPC_STRUCT& rpc_info)
{
	fstream ofs;
	ofs.open(filename, ios_base::out);
	ofs<<"satId = \"XXX\";"<<endl;
	ofs<<"bandId = \"XXX\";"<<endl;
	ofs<<"SpecId = \"XXX\";"<<endl;
	ofs<<"BEGIN_GROUP = IMAGE"<<endl;
	{
		ofs<<"  errBias =   "<<1.0<<endl;
		ofs<<"  errRand =   "<<0.0<<endl;
		ofs<<"  lineOffset = "<<rpc_info.LINE_OFF<<endl;
		ofs<<"  sampOffset = "<<rpc_info.SAMP_OFF<<endl;
		ofs<<"  latOffset = "<<rpc_info.LAT_OFF<<endl;
		ofs<<"  longOffset = "<<rpc_info.LONG_OFF<<endl;
		ofs<<"  heightOffset = "<<rpc_info.HEIGHT_OFF<<endl;
		ofs<<"  lineScale = "<<rpc_info.LINE_SCALE<<endl;
		ofs<<"  sampScale = "<<rpc_info.SAMP_SCALE<<endl;
		ofs<<"  latScale = "<<rpc_info.LAT_SCALE<<endl;
		ofs<<"  longScale = "<<rpc_info.LONG_SCALE<<endl;
		ofs<<"  heightScale = "<<rpc_info.HEIGHT_SCALE<<endl;
		ofs<<"  lineNumCoef = ("<<endl;
		{
			for (int i=0;i<20;i++)
			{
				ofs<<"    "<<rpc_info.LINE_NUM_COEFF[i];
				if (i < 19)
				{
					ofs<<","<<endl;
				}
			}
		}
		ofs<<");"<<endl;

		ofs<<"  lineDenCoef = ("<<endl;
		{
			for (int i=0;i<20;i++)
			{
				ofs<<"    "<<rpc_info.LINE_DEN_COEFF[i];
				if (i < 19)
				{
					ofs<<","<<endl;
				}
			}
		}
		ofs<<");"<<endl;

		ofs<<"  sampNumCoef = ("<<endl;
		{
			for (int i=0;i<20;i++)
			{
				ofs<<"    "<<rpc_info.SAMP_NUM_COEFF[i];
				if (i < 19)
				{
					ofs<<","<<endl;
				}
			}
		}
		ofs<<");"<<endl;

		ofs<<"  sampDenCoef = ("<<endl;
		{
			for (int i=0;i<20;i++)
			{
				ofs<<"    "<<rpc_info.SAMP_DEN_COEFF[i];
				if (i < 19)
				{
					ofs<<","<<endl;
				}
			}
		}
		ofs<<");"<<endl;
	}
	ofs<<"END_GROUP = IMAGE"<<endl;
	ofs<<"END;"<<endl;
	ofs.close();
	return true;
}

bool ZY3QVProcReader::readHeader(const char* header)
{
	int last_pos = 0;
	string strPath = SBeforeLast(string(header), '\\');
	GDALAllRegister();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO"); // gdal ����·��֧��
	CPLSetConfigOption("SHAPE_ENCODING", "");	// shapefile �����ֶ�֧��
	FILE *pfIn = fopen(header, "rb+");
	if (!pfIn)
	{
		cerr << "open error!" << endl;
		return false;
	}
	fseek(pfIn, 0L, SEEK_END);
	LONGLONG file_size = ftell(pfIn);
	// seek back to the beginning:
	fseek(pfIn, 0L, SEEK_SET);


	// ��QUIVIMAGE_HEAD_INFO
	readHeadInfo(pfIn, theHeaderInfo);
	writeDescXML(theHeaderInfo, (strPath + "\\desc.xml").c_str());
	int cpos = ftell(pfIn);
	
	fstream ephFile;
	ephFile.open((strPath + "\\eph.txt").c_str(), std::ios_base::out);
	ephFile.close();

	fstream nadirsFile;
	nadirsFile.open((strPath + "\\nadirs.txt").c_str(), std::ios_base::out);
	nadirsFile.close();

	int percent = 0;
	printf("\r%d%%", percent);

	int SceneCount = 0;
	while (!feof(pfIn)) {
		int pos_start = ftell(pfIn);
		// ��ȡ��ȫ�ָ�����Ϣ
		SCENE_GLOBAL_INFO scene_global_info;
		readSceneGlobalInfo(pfIn, scene_global_info);
		cpos = ftell(pfIn);
		//char buf[1024];
		//sprintf_s(buf, "%s\\Scene%02d", strPath.c_str(), SceneCount + 1);
		//if (!QDir(buf).exists())
		//{
		//	_mkdir(buf);
		//}
		//string sceneDir(buf);
		//string sceneImageFile = sceneDir + "\\IMAGE.TIF";
		//string sceneJp2ImageFile = sceneDir + "\\IMAGE.jp2";
		//string sceneRpbFile = sceneDir + "\\IMAGE.rpb";
		//string sceneEphFile = sceneDir + "\\IMAGE.eph";

		GDALDataType eDT;
		if (16 == scene_global_info.sample_bit_count)
		{
			eDT = GDALDataType::GDT_Int16;
		}
		else
		{
			eDT = GDALDataType::GDT_Byte;
		}

		theBands = theHeaderInfo.image_mode ? 1 : 4;
		int lineDataByte = theBands * scene_global_info.scene_samples * scene_global_info.sample_bit_count / 8;
		int nLines = scene_global_info.scene_lines;
		int nSamples = scene_global_info.scene_samples;
		
		int iBit = scene_global_info.sample_bit_count / 8;
		for (int iLine = 0; iLine < nLines;)
		{
			// ��ȡͼ���и�����Ϣ
			QUIVIMAGE_AUX_INFO aux;
			readQuivAuxInfo(pfIn, aux);
			iLine++;
			cpos = ftell(pfIn);
		}
		//writeRPC(sceneRpbFile.c_str(), scene_global_info.rpc_info);

		streampos pos_ = ftell(pfIn); //��ȡ�ļ�ָ���λ��
		int tmpPercent = (int)(pos_ / (double)(file_size)* 100 + 0.5);
		printf("\r%d%%", tmpPercent);
		int pos_end = ftell(pfIn);
		int scene_size = pos_end - pos_start;
		SceneCount++;
	}
	fclose(pfIn);

	//SetConsoleCursorPosition(hOut, pos);
	//cout<<100<<"%"<<endl;
	printf("\r%d%%\n", 100);
	return true;
}

bool ZY3QVProcReader::extractHeader(const char* filename)
{
	int last_pos = 0;
	string strPath = SBeforeLast(string(filename), '\\');
	string strHeaderFile = strPath + "\\qvheader.dat";
	GDALAllRegister();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO"); // gdal ����·��֧��
	CPLSetConfigOption("SHAPE_ENCODING", "");	// shapefile �����ֶ�֧��
	FILE *pfIn = fopen(filename, "rb+");
	FILE *pfOut = fopen(strHeaderFile.c_str(), "wb+");
	if (!pfIn)
	{
		cerr << "open error!" << endl;
		return false;
	}
	fseek(pfIn, 0L, SEEK_END);
	LONGLONG file_size = ftell(pfIn);
	// seek back to the beginning:
	fseek(pfIn, 0L, SEEK_SET);
	

	// ��QUIVIMAGE_HEAD_INFO
	readHeadInfo(pfIn, theHeaderInfo, pfOut);
	writeDescXML(theHeaderInfo, (strPath + "\\desc.xml").c_str());
	int cpos = ftell(pfIn);

	//theBands = theHeaderInfo.image_mode?1:3;
	//int lineDataByte = theBands * theHeaderInfo.data_width * theHeaderInfo.sample_bit_count/8;
	//theLines = (file_size-180)/(92+lineDataByte);
	//theSamples = theHeaderInfo.data_width;

	fstream ephFile;
	ephFile.open((strPath + "\\eph.txt").c_str(), std::ios_base::out);
	ephFile.close();

	fstream nadirsFile;
	nadirsFile.open((strPath + "\\nadirs.txt").c_str(), std::ios_base::out);
	nadirsFile.close();

	int percent = 0;
	printf("\r%d%%", percent);

	int SceneCount = 0;
	while (!feof(pfIn)) {
		int pos_start = ftell(pfIn);
		// ��ȡ��ȫ�ָ�����Ϣ
		SCENE_GLOBAL_INFO scene_global_info;
		readSceneGlobalInfo(pfIn, scene_global_info, pfOut);
		cpos = ftell(pfIn);
		char buf[1024];
		sprintf_s(buf, "%s\\Scene%02d", strPath.c_str(), SceneCount + 1);
		if (!QDir(buf).exists())
		{
			_mkdir(buf);
		}
		string sceneDir(buf);
		string sceneImageFile = sceneDir + "\\IMAGE.TIF";
		string sceneJp2ImageFile = sceneDir + "\\IMAGE.jp2";
		string sceneRpbFile = sceneDir + "\\IMAGE.rpb";
		string sceneEphFile = sceneDir + "\\IMAGE.eph";

		GDALDataType eDT;
		if (16 == scene_global_info.sample_bit_count)
		{
			eDT = GDALDataType::GDT_Int16;
		}
		else
		{
			eDT = GDALDataType::GDT_Byte;
		}

		theBands = theHeaderInfo.image_mode ? 1 : 4;
		int lineDataByte = theBands * scene_global_info.scene_samples * scene_global_info.sample_bit_count / 8;
		int nLines = scene_global_info.scene_lines;
		int nSamples = scene_global_info.scene_samples;

		//GDALDatasetH hDstDS = new GDALDatasetH;
		//const char         *pszFormat = "GTiff";
		//char               *pszTargetSRS = NULL;
		//double adfThisGeoTransform[6];
		//GDALDriverH hDriver = GDALGetDriverByName( pszFormat );
		//hDstDS = GDALCreate( hDriver, sceneImageFile.c_str(), nSamples, nLines, theBands, eDT, NULL );

		vector<int> panBandMap;
		for (int i = 0; i < theBands; ++i)
		{
			panBandMap.push_back(i + 1);
		}
		//GByte *pBuf = NULL;
		int iBit = scene_global_info.sample_bit_count / 8;
		//vector<GByte> pBuf(nLines*lineDataByte);// = new GByte[nLines*lineDataByte];
		for (int iLine = 0; iLine < nLines;)
		{
			// ��ȡͼ���и�����Ϣ
			QUIVIMAGE_AUX_INFO aux;
			readQuivAuxInfo(pfIn, aux, pfOut);
			iLine++;
			cpos = ftell(pfIn);
		}
		writeRPC(sceneRpbFile.c_str(), scene_global_info.rpc_info);
		////fs.ignore(scene_global_info.qv_line_info_length+1);
		//for (int iLine = 0;iLine < nLines;)
		//{
		//	////	// ��ȡͼ���и�����Ϣ
		//	//QUIVIMAGE_AUX_INFO aux;
		//	//readQuivAuxInfo(fs, aux);
		//	if (file_size - fs.tellg() < lineDataByte)
		//	{
		//		((GDALDataset*)hDstDS)->RasterIO(GF_Write,0,0,nSamples, nLines, &pBuf[0], nSamples, nLines, eDT, theBands, &panBandMap[0],theBands, lineDataByte, 1);
		//		GDALClose( hDstDS );
		//		//CPLFree( pBuf );
		//		writeRPC(sceneRpbFile.c_str(), scene_global_info.rpc_info);
		//		break;
		//	}

		//	fs.read((char*)(&pBuf[0]+iLine*lineDataByte), lineDataByte);

		//	if (iLine >= nLines-1)
		//	{
		//		((GDALDataset*)hDstDS)->RasterIO(GF_Write,0,0,nSamples, nLines, &pBuf[0], nSamples, nLines, eDT, theBands, &panBandMap[0],theBands, lineDataByte, 1);
		//		GDALClose( hDstDS );
		//		//CPLFree( pBuf );
		//		writeRPC(sceneRpbFile.c_str(), scene_global_info.rpc_info);
		//	}
		//	iLine++;

		//	// ���㵱ǰ����
		//	streampos pos_ = fs.tellg(); //��ȡ�ļ�ָ���λ��
		//	int tmpPercent = (int)(pos_ / (double)(file_size) * 100 + 0.5);
		//	if(tmpPercent > percent)
		//	{
		//		percent = tmpPercent;
		//		// �ڱ������λ�����������ԭ�������
		//		SetConsoleCursorPosition(hOut, pos);
		//		cout<<percent<<"%";
		//	}
		//}
		FILE* pJp2Handle;
		fopen_s(&pJp2Handle, sceneJp2ImageFile.c_str(), "wb+");
		char* sceneBuf = new char[scene_global_info.scene_size];
		fread(sceneBuf, scene_global_info.scene_size, 1, pfIn);
		fwrite(sceneBuf, scene_global_info.scene_size, 1, pJp2Handle);
		delete[]sceneBuf;
		sceneBuf = NULL;
		//int scenePos = 0;
		//while (scenePos < scene_global_info.scene_size)
		//{
		//	int bufSize = min(BUFFER_SIZE, scene_global_info.scene_size - scenePos);
		//	char* sceneBuf = new char[bufSize];
		//	fs.read(sceneBuf, bufSize);
		//	fwrite(sceneBuf, bufSize, 1, pJp2Handle);
		//	delete []sceneBuf;
		//	sceneBuf = NULL;
		//	scenePos += bufSize;
		//}
		fclose(pJp2Handle);
		//fs.ignore(scene_global_info.scene_size);
		streampos pos_ = ftell(pfIn); //��ȡ�ļ�ָ���λ��
		int tmpPercent = (int)(pos_ / (double)(file_size)* 100 + 0.5);
		printf("\r%d%%", tmpPercent);
		int pos_end = ftell(pfIn);
		int scene_size = pos_end - pos_start;
		SceneCount++;
	}
	fclose(pfIn);
	fclose(pfOut);

	//SetConsoleCursorPosition(hOut, pos);
	//cout<<100<<"%"<<endl;
	printf("\r%d%%\n", 100);
	return true;
}

bool ZY3QVProcReader::read(const char* filename, const char* outPath)
{
	int last_pos = 0;
	//string strPath = SBeforeLast(string(filename), '\\');
	string strPath = string(outPath);
	GDALAllRegister();
	//OGRRegisterAll();//ע�����е��ļ���ʽ����
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO"); // gdal ����·��֧��
	CPLSetConfigOption("SHAPE_ENCODING", "");	// shapefile �����ֶ�֧��
	ifstream fs(filename, std::ios_base::binary);
	//fs.open(filename, std::ios_base::binary);
	if(!fs)
	{
		cerr<<"open error!"<<endl;
		return false;
	}
	fs.seekg(0, ios::end);
	LONGLONG file_size = fs.tellg();
	fs.seekg(0, ios::beg);
	
	// ��QUIVIMAGE_HEAD_INFO
	readHeadInfo(fs, theHeaderInfo);


	//theBands = theHeaderInfo.image_mode?1:3;
	//int lineDataByte = theBands * theHeaderInfo.data_width * theHeaderInfo.sample_bit_count/8;
	//theLines = (file_size-180)/(92+lineDataByte);
	//theSamples = theHeaderInfo.data_width;

	fstream ephFile;
	ephFile.open((strPath+"\\eph.txt").c_str(), std::ios_base::out);
	ephFile.close();

	fstream nadirsFile;
	nadirsFile.open((strPath+"\\nadirs.txt").c_str(), std::ios_base::out);
	nadirsFile.close();

	writeDescXML(theHeaderInfo, (strPath+"\\desc.xml").c_str());

	int percent = 0;
	printf("\r%d%%", percent);
	fflush(stdout);

	int SceneCount = 0;
	while (!fs.eof()) {
		// ��ȡ��ȫ�ָ�����Ϣ
		SCENE_GLOBAL_INFO scene_global_info;
		readSceneGlobalInfo(fs, scene_global_info);
		char strBuf[1024];
		sprintf_s(strBuf, "SCENE_%d", SceneCount+1);
		string sceneName(strBuf);
		string sceneImageFile = strPath + "\\" + sceneName + ".tif";
		string sceneRpbFile = strPath + "\\" + sceneName + ".rpb";
		string sceneEphFile = strPath + "\\" + sceneName + ".eph";
		writeRPC(sceneRpbFile.c_str(), scene_global_info.rpc_info);

		GDALDataType eDT;
		if (16 == scene_global_info.sample_bit_count)
		{
			eDT = GDALDataType::GDT_Int16;
		}else
		{
			eDT = GDALDataType::GDT_Byte;
		}

		theBands = theHeaderInfo.image_mode?1:4;
		int lineDataByte = theBands * scene_global_info.scene_samples * scene_global_info.sample_bit_count/8;
		int nLines = scene_global_info.scene_lines;
		int nSamples = scene_global_info.scene_samples;

		GDALDatasetH hDstDS = new GDALDatasetH;
		const char         *pszFormat = "GTiff";
		char               *pszTargetSRS = NULL;
		double adfThisGeoTransform[6];
		GDALDriverH hDriver = GDALGetDriverByName( pszFormat );
		hDstDS = GDALCreate( hDriver, sceneImageFile.c_str(), nSamples, nLines, theBands, eDT, NULL );

		vector<int> panBandMap;
		for (int i=0;i < theBands;++i)
		{
			panBandMap.push_back(i+1);
		}
		GByte *pBuf = NULL;
		int iBit = scene_global_info.sample_bit_count/8;
		pBuf = new GByte[nLines*lineDataByte];
		for (int iLine = 0;iLine < nLines;)
		{
			// ��ȡͼ���и�����Ϣ
			QUIVIMAGE_AUX_INFO aux;
			readQuivAuxInfo(fs, aux);

			if (file_size - fs.tellg() < lineDataByte)
			{
				((GDALDataset*)hDstDS)->RasterIO(GF_Write,0,0,nSamples, nLines, pBuf, nSamples, nLines, eDT, theBands, &panBandMap[0],theBands, lineDataByte, 1);
				GDALClose( hDstDS );
				CPLFree( pBuf );
				break;
			}

			fs.read((char*)(pBuf+iLine*lineDataByte), lineDataByte);

			if (iLine >= nLines-1)
			{
				((GDALDataset*)hDstDS)->RasterIO(GF_Write,0,0,nSamples, nLines, pBuf, nSamples, nLines, eDT, theBands, &panBandMap[0],theBands, lineDataByte, 1);
				CPLFree( pBuf );
			}
			iLine++;

			// ���㵱ǰ����
			streampos pos_ = fs.tellg(); //��ȡ�ļ�ָ���λ��
			int tmpPercent = (int)(pos_ / (double)(file_size) * 100 + 0.5);
			if(tmpPercent > percent)
			{
				percent = tmpPercent;
				printf("\r%d%%", percent);
				fflush(stdout);
			}
		}
		SceneCount++;
	}
	fs.close();

	printf("\r%d%%\n", 100);
	fflush(stdout);
	return true;
}

bool ZY3QVProcReader::read_by_scene(const char* filename, const char* outPath)
{
	int last_pos = 0;
	//string strPath = SBeforeLast(string(filename), '\\');
	string strPath = string(outPath);
	GDALAllRegister();
	//OGRRegisterAll();//ע�����е��ļ���ʽ����
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO"); // gdal ����·��֧��
	CPLSetConfigOption("SHAPE_ENCODING", "");	// shapefile �����ֶ�֧��
	ifstream fs(filename, std::ios_base::binary);
	//fs.open(filename, std::ios_base::binary);
	if(!fs)
	{
		cerr<<"open error!"<<endl;
		return false;
	}
	fs.seekg(0, ios::end);
	LONGLONG file_size = fs.tellg();
	fs.seekg(0, ios::beg);

	// ��QUIVIMAGE_HEAD_INFO
	readHeadInfo(fs, theHeaderInfo);
	writeDescXML(theHeaderInfo, (strPath+"\\desc.xml").c_str());
	int cpos = fs.tellg();

	//theBands = theHeaderInfo.image_mode?1:3;
	//int lineDataByte = theBands * theHeaderInfo.data_width * theHeaderInfo.sample_bit_count/8;
	//theLines = (file_size-180)/(92+lineDataByte);
	//theSamples = theHeaderInfo.data_width;

	fstream ephFile;
	ephFile.open((strPath+"\\eph.txt").c_str(), std::ios_base::out);
	ephFile.close();

	fstream nadirsFile;
	nadirsFile.open((strPath+"\\nadirs.txt").c_str(), std::ios_base::out);
	nadirsFile.close();

	int percent = 0;
	printf("\r%d%%", percent);

	int SceneCount = 0;
	int iLineTotal = 0;

	double fLastTime = 0.0;
	SAT_POS lastPos;
	lastPos.x = 0.0;
	lastPos.y = 0.0;
	lastPos.z = 0.0;
	SAT_ATT lastAtt;
	lastAtt.pitch = 0.0;
	lastAtt.yaw = 0.0;
	lastAtt.roll = 0.0;

	while (!fs.eof()) {
		int pos_start = fs.tellg();
		// ��ȡ��ȫ�ָ�����Ϣ
		SCENE_GLOBAL_INFO scene_global_info;
		readSceneGlobalInfo(fs, scene_global_info);
		cpos = fs.tellg();
		char buf[1024];
		sprintf_s(buf, "%s\\Scene%02d", strPath.c_str(), SceneCount+1);
		if (!QDir(buf).exists())
		{
			_mkdir(buf);
		}
		string sceneDir(buf);
		string sceneImageFile = sceneDir + "\\IMAGE.TIF";
		string sceneJp2ImageFile = sceneDir + "\\IMAGE.jp2";
		string sceneRpbFile = sceneDir + "\\IMAGE.rpb";
		string sceneEphFile = sceneDir + "\\IMAGE.eph";

		GDALDataType eDT;
		if (16 == scene_global_info.sample_bit_count)
		{
			eDT = GDALDataType::GDT_Int16;
		}else
		{
			eDT = GDALDataType::GDT_Byte;
		}

		theBands = theHeaderInfo.image_mode?1:4;
		int lineDataByte = theBands * scene_global_info.scene_samples * scene_global_info.sample_bit_count/8;
		int nLines = scene_global_info.scene_lines;
		int nSamples = scene_global_info.scene_samples;

		//GDALDatasetH hDstDS = new GDALDatasetH;
		//const char         *pszFormat = "GTiff";
		//char               *pszTargetSRS = NULL;
		//double adfThisGeoTransform[6];
		//GDALDriverH hDriver = GDALGetDriverByName( pszFormat );
		//hDstDS = GDALCreate( hDriver, sceneImageFile.c_str(), nSamples, nLines, theBands, eDT, NULL );

		vector<int> panBandMap;
		for (int i=0;i < theBands;++i)
		{
			panBandMap.push_back(i+1);
		}
		//GByte *pBuf = NULL;
		int iBit = scene_global_info.sample_bit_count/8;
		//vector<GByte> pBuf(nLines*lineDataByte);// = new GByte[nLines*lineDataByte];

		vector<QUIVIMAGE_AUX_INFO> auxList;
		for (int iLine = 0;iLine < nLines;)
		{
			// ��ȡͼ���и�����Ϣ
			QUIVIMAGE_AUX_INFO aux;
			readQuivAuxInfo(fs, aux);

			double fNewTime = aux.line_time;
			if (0 == aux.line_index)
			{
				iLine++;
				iLineTotal++;
				continue;
			}

			if (fLastTime != fNewTime)
			{
				if (lastPos.x == aux.satpos.x
					&& lastPos.y == aux.satpos.y
					&& lastPos.z == aux.satpos.z)
				{
					iLine++;
					iLineTotal++;
					//lastPos = aux.satpos;
					//lastAtt = aux.satatt;
					//fLastTime = fNewTime;
					continue;
				}
				else if (lastAtt.pitch == aux.satatt.pitch
					&& lastAtt.yaw == aux.satatt.yaw
					&& lastAtt.roll == aux.satatt.roll)
				{
					iLine++;
					iLineTotal++;
					//lastPos = aux.satpos;
					//lastAtt = aux.satatt;
					//fLastTime = fNewTime;
					continue;
				}
				auxList.push_back(aux);

				ephFile.open((strPath + "\\eph.txt").c_str(), std::ios_base::app);
				ephFile <<
					iLineTotal << " "
					<< aux.line_index << " "
					<< setprecision(25)
					<< aux.satpos.x << " "
					<< aux.satpos.y << " "
					<< aux.satpos.z << " "
					<< aux.satpos.vx << " "
					<< aux.satpos.vy << " "
					<< aux.satpos.vz << " "
					<< aux.satatt.roll << " "
					<< aux.satatt.pitch << " "
					<< aux.satatt.yaw << " "
					<< aux.satatt.vroll << " "
					<< aux.satatt.vpitch << " "
					<< aux.satatt.vyaw << " "
					<< aux.line_time << endl;
				ephFile.close();

				double lat, lon, alt;
				ecef2lla(aux.satpos.x, aux.satpos.y, aux.satpos.z, lat, lon, alt);
				nadirsFile.open((strPath + "\\nadirs.txt").c_str(), std::ios_base::app);
				nadirsFile <<
					iLineTotal << " "
					<< aux.line_index << " "
					<< lat << " "
					<< lon << endl;
				nadirsFile.close();
				fLastTime = fNewTime;
				lastPos = aux.satpos;
				lastAtt = aux.satatt;
			}
			iLine++;
			iLineTotal++;
			cpos = fs.tellg();
		}
		writeRPC(sceneRpbFile.c_str(), scene_global_info.rpc_info);
		writeDescXML(theHeaderInfo, scene_global_info, auxList, (sceneDir + "\\desc.xml").c_str());
		////fs.ignore(scene_global_info.qv_line_info_length+1);
		//for (int iLine = 0;iLine < nLines;)
		//{
		//	////	// ��ȡͼ���и�����Ϣ
		//	//QUIVIMAGE_AUX_INFO aux;
		//	//readQuivAuxInfo(fs, aux);
		//	if (file_size - fs.tellg() < lineDataByte)
		//	{
		//		((GDALDataset*)hDstDS)->RasterIO(GF_Write,0,0,nSamples, nLines, &pBuf[0], nSamples, nLines, eDT, theBands, &panBandMap[0],theBands, lineDataByte, 1);
		//		GDALClose( hDstDS );
		//		//CPLFree( pBuf );
		//		writeRPC(sceneRpbFile.c_str(), scene_global_info.rpc_info);
		//		break;
		//	}

		//	fs.read((char*)(&pBuf[0]+iLine*lineDataByte), lineDataByte);

		//	if (iLine >= nLines-1)
		//	{
		//		((GDALDataset*)hDstDS)->RasterIO(GF_Write,0,0,nSamples, nLines, &pBuf[0], nSamples, nLines, eDT, theBands, &panBandMap[0],theBands, lineDataByte, 1);
		//		GDALClose( hDstDS );
		//		//CPLFree( pBuf );
		//		writeRPC(sceneRpbFile.c_str(), scene_global_info.rpc_info);
		//	}
		//	iLine++;

		//	// ���㵱ǰ����
		//	streampos pos_ = fs.tellg(); //��ȡ�ļ�ָ���λ��
		//	int tmpPercent = (int)(pos_ / (double)(file_size) * 100 + 0.5);
		//	if(tmpPercent > percent)
		//	{
		//		percent = tmpPercent;
		//		// �ڱ������λ�����������ԭ�������
		//		SetConsoleCursorPosition(hOut, pos);
		//		cout<<percent<<"%";
		//	}
		//}
		FILE* pJp2Handle;
		fopen_s(&pJp2Handle, sceneJp2ImageFile.c_str(), "wb+");
		char* sceneBuf = new char[scene_global_info.scene_size];
		fs.read(sceneBuf, scene_global_info.scene_size);
		fwrite(sceneBuf, scene_global_info.scene_size, 1, pJp2Handle);
		delete []sceneBuf;
		sceneBuf = NULL;
		//int scenePos = 0;
		//while (scenePos < scene_global_info.scene_size)
		//{
		//	int bufSize = min(BUFFER_SIZE, scene_global_info.scene_size - scenePos);
		//	char* sceneBuf = new char[bufSize];
		//	fs.read(sceneBuf, bufSize);
		//	fwrite(sceneBuf, bufSize, 1, pJp2Handle);
		//	delete []sceneBuf;
		//	sceneBuf = NULL;
		//	scenePos += bufSize;
		//}
		fclose(pJp2Handle);
		//fs.ignore(scene_global_info.scene_size);
		streampos pos_ = fs.tellg(); //��ȡ�ļ�ָ���λ��
		int tmpPercent = (int)(pos_ / (double)(file_size)* 100 + 0.5);
		printf("\r%d%%", tmpPercent);
		int pos_end = fs.tellg();
		int scene_size = pos_end - pos_start;
		SceneCount++;
	}
	fs.close();

	//SetConsoleCursorPosition(hOut, pos);
	//cout<<100<<"%"<<endl;
	printf("\r%d%%\n", 100);
	return true;
}

bool ZY3QVProcReader::readHeadInfo(FILE* pfIn, QUIVIMAGE_HEAD_INFO& header, FILE* pfOut/* = NULL */)
{
	int pos = ftell(pfIn);

	// ��ʽId 64 �磺��CHINA RADI IECAS QuivImage Ver 01����ʣ��ռ���ո�0x20����ͬ��
	fread((char*)header.format_id, sizeof(header.format_id), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)header.format_id, sizeof(header.format_id), 1, pfOut);
	}
	// �汾�� 16
	fread((char*)header.version_id, sizeof(header.version_id), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)header.version_id, sizeof(header.version_id), 1, pfOut);
	}
	//	���ո����ݵĵ���վ�ı�ʶ 16
	fread((char*)theHeaderInfo.station_id, sizeof(theHeaderInfo.station_id), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)theHeaderInfo.station_id, sizeof(theHeaderInfo.station_id), 1, pfOut);
	}
	//	��Ӧ�����Ǳ�ʶȫ�� 64
	fread((char*)theHeaderInfo.satellite_id, sizeof(theHeaderInfo.satellite_id), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)theHeaderInfo.satellite_id, sizeof(theHeaderInfo.satellite_id), 1, pfOut);
	}
	//	��Ӧ�Ĵ�������ʶȫ�� 512
	fread((char*)theHeaderInfo.sensor_id, sizeof(theHeaderInfo.sensor_id), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)theHeaderInfo.sensor_id, sizeof(theHeaderInfo.sensor_id), 1, pfOut);
	}
	//	�����ʶ 32
	fread((char*)theHeaderInfo.jobTaskID, sizeof(theHeaderInfo.jobTaskID), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)theHeaderInfo.jobTaskID, sizeof(theHeaderInfo.jobTaskID), 1, pfOut);
	}
	//	����� 4
	fread((char*)&theHeaderInfo.orbit_num, sizeof(theHeaderInfo.orbit_num), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&theHeaderInfo.orbit_num, sizeof(theHeaderInfo.orbit_num), 1, pfOut);
	}
	//	ͨ���� 4
	fread((char*)&theHeaderInfo.channel_num, sizeof(theHeaderInfo.channel_num), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&theHeaderInfo.channel_num, sizeof(theHeaderInfo.channel_num), 1, pfOut);
	}
	//	�����񵥴�������ļ����� 4
	fread((char*)&theHeaderInfo.totalfiles, sizeof(theHeaderInfo.totalfiles), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&theHeaderInfo.totalfiles, sizeof(theHeaderInfo.totalfiles), 1, pfOut);
	}
	//	���δ�����ļ������������е��ļ���� 4
	fread((char*)&theHeaderInfo.files_num, sizeof(theHeaderInfo.files_num), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&theHeaderInfo.files_num, sizeof(theHeaderInfo.files_num), 1, pfOut);
	}
	//	���п�������ʱ�Ľ�������,��һ���Ľ������� 4
	fread((char*)&theHeaderInfo.desample_num, sizeof(theHeaderInfo.desample_num), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&theHeaderInfo.desample_num, sizeof(theHeaderInfo.desample_num), 1, pfOut);
	}
	//  ͼ��ģʽ 4 ����ֵ=1ʱ����ͼ��Ϊ�Ҷ�ͼ�񣬷���Ϊ��ɫ�ϳ�ͼ���磺0
	fread((char*)&theHeaderInfo.image_mode, sizeof(theHeaderInfo.image_mode), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&theHeaderInfo.image_mode, sizeof(theHeaderInfo.image_mode), 1, pfOut);
	}
	//	���ݽ��յĿ�ʼʱ�� 32
	readTimeInfo(pfIn, theHeaderInfo.start_time, pfOut);
	//  ���ݽ��յĽ���ʱ�� 32
	readTimeInfo(pfIn, theHeaderInfo.end_time, pfOut);
	//	��Ӧ�ĵ���վ�ڵ����ϵ�λ����Ϣ 8
	fread((char*)&theHeaderInfo.station_pos.longitude, sizeof(theHeaderInfo.station_pos.longitude), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&theHeaderInfo.station_pos.longitude, sizeof(theHeaderInfo.station_pos.longitude), 1, pfOut);
	}
	fread((char*)&theHeaderInfo.station_pos.latitude, sizeof(theHeaderInfo.station_pos.latitude), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&theHeaderInfo.station_pos.latitude, sizeof(theHeaderInfo.station_pos.latitude), 1, pfOut);
	}
	// Ԥ���ռ� 4193505
	long nullLength = 4193504L;
	fseek(pfIn, nullLength, SEEK_CUR);
	if (pfOut)
	{
		const int bufSize = 1024;
		char buffer[bufSize] = { 0 };
		int bufPos = 0;
		while (bufPos < nullLength)
		{
			int nsize = min(bufSize, nullLength - bufPos);
			fwrite(buffer, sizeof(char), nsize, pfOut);
			bufPos += nsize;
		}
	}

	return true;
}

bool ZY3QVProcReader::readHeadInfo(ifstream& fs, QUIVIMAGE_HEAD_INFO& header)
{
	int pos = fs.tellg();

	// ��ʽId 64 �磺��CHINA RADI IECAS QuivImage Ver 01����ʣ��ռ���ո�0x20����ͬ��
	fs.read((char*)header.format_id, sizeof(header.format_id));
	// �汾�� 16
	fs.read((char*)header.version_id, sizeof(header.version_id));
	//	���ո����ݵĵ���վ�ı�ʶ 16
	fs.read((char*)theHeaderInfo.station_id, sizeof(theHeaderInfo.station_id));
	//	��Ӧ�����Ǳ�ʶȫ�� 64
	fs.read((char*)theHeaderInfo.satellite_id, sizeof(theHeaderInfo.satellite_id));
	//	��Ӧ�Ĵ�������ʶȫ�� 512
	fs.read((char*)theHeaderInfo.sensor_id, sizeof(theHeaderInfo.sensor_id));
	//	�����ʶ 32
	fs.read((char*)theHeaderInfo.jobTaskID, sizeof(theHeaderInfo.jobTaskID));
	//	����� 4
	fs.read((char*)&theHeaderInfo.orbit_num, sizeof(theHeaderInfo.orbit_num));
	//	ͨ���� 4
	fs.read((char*)&theHeaderInfo.channel_num, sizeof(theHeaderInfo.channel_num));
	//	�����񵥴�������ļ����� 4
	fs.read((char*)&theHeaderInfo.totalfiles, sizeof(theHeaderInfo.totalfiles));
	//	���δ�����ļ������������е��ļ���� 4
	fs.read((char*)&theHeaderInfo.files_num, sizeof(theHeaderInfo.files_num));
	//	���п�������ʱ�Ľ�������,��һ���Ľ������� 4
	fs.read((char*)&theHeaderInfo.desample_num, sizeof(theHeaderInfo.desample_num));
	//  ͼ��ģʽ 4 ����ֵ=1ʱ����ͼ��Ϊ�Ҷ�ͼ�񣬷���Ϊ��ɫ�ϳ�ͼ���磺0
	fs.read((char*)&theHeaderInfo.image_mode, sizeof(theHeaderInfo.image_mode));
	//	���ݽ��յĿ�ʼʱ�� 32
	readTimeInfo(fs, theHeaderInfo.start_time);
	//  ���ݽ��յĽ���ʱ�� 32
	readTimeInfo(fs, theHeaderInfo.end_time);
	//	��Ӧ�ĵ���վ�ڵ����ϵ�λ����Ϣ 8
	fs.read((char*)&theHeaderInfo.station_pos.longitude, sizeof(theHeaderInfo.station_pos.longitude));
	fs.read((char*)&theHeaderInfo.station_pos.latitude, sizeof(theHeaderInfo.station_pos.latitude));
	// Ԥ���ռ� 4193505
	fs.ignore(4193504);

	int pos1 = fs.tellg();
	int db = pos1 - pos;
	return true;
}

bool ZY3QVProcReader::readTimeInfo(FILE* pfIn, _TIMESTAMP& t, FILE* pfOut/* = NULL */)
{
	//	year 4
	fread((char*)&t.year, sizeof(int), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&t.year, sizeof(int), 1, pfOut);
	}
	//	mon 4
	fread((char*)&t.mon, sizeof(int), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&t.mon, sizeof(int), 1, pfOut);
	}
	//	day 4
	fread((char*)&t.day, sizeof(int), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&t.day, sizeof(int), 1, pfOut);
	}
	//	hour 4
	fread((char*)&t.hour, sizeof(int), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&t.hour, sizeof(int), 1, pfOut);
	}
	//	min 4
	fread((char*)&t.min, sizeof(int), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&t.min, sizeof(int), 1, pfOut);
	}
	//	sec 4
	fread((char*)&t.sec, sizeof(int), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&t.sec, sizeof(int), 1, pfOut);
	}
	//	millsec 4
	fread((char*)&t.millsec, sizeof(int), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&t.millsec, sizeof(int), 1, pfOut);
	}
	//	microsec 4
	fread((char*)&t.microsec, sizeof(int), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&t.microsec, sizeof(int), 1, pfOut);
	}
	return true;
}

bool ZY3QVProcReader::readTimeInfo(ifstream& fs, _TIMESTAMP& t)
{
	//	year 4
	fs.read((char*)&t.year, sizeof(int));
	//	mon 4
	fs.read((char*)&t.mon, sizeof(int));
	//	day 4
	fs.read((char*)&t.day, sizeof(int));
	//	hour 4
	fs.read((char*)&t.hour, sizeof(int));
	//	min 4
	fs.read((char*)&t.min, sizeof(int));
	//	sec 4
	fs.read((char*)&t.sec, sizeof(int));
	//	millsec 4
	fs.read((char*)&t.millsec, sizeof(int));
	//	microsec 4
	fs.read((char*)&t.microsec, sizeof(int));
	return true;
}

bool ZY3QVProcReader::readSAT_POS(FILE* pfIn, SAT_POS& pos, FILE* pfOut/* = NULL */)
{
	//	x 4
	fread((char*)&pos.x, sizeof(float), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&pos.x, sizeof(float), 1, pfOut);
	}
	//	y 4
	fread((char*)&pos.y, sizeof(float), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&pos.y, sizeof(float), 1, pfOut);
	}
	//	z 4
	fread((char*)&pos.z, sizeof(float), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&pos.z, sizeof(float), 1, pfOut);
	}
	//	vx 4
	fread((char*)&pos.vx, sizeof(float), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&pos.vx, sizeof(float), 1, pfOut);
	}
	//	vy 4
	fread((char*)&pos.vy, sizeof(float), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&pos.vy, sizeof(float), 1, pfOut);
	}
	//	vz 4
	fread((char*)&pos.vz, sizeof(float), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&pos.vz, sizeof(float), 1, pfOut);
	}
	return true;
}

bool ZY3QVProcReader::readSAT_POS(ifstream& fs, SAT_POS& pos)
{
	//	x 4
	fs.read((char*)&pos.x, sizeof(float));
	//	y 4
	fs.read((char*)&pos.y, sizeof(float));
	//	z 4
	fs.read((char*)&pos.z, sizeof(float));
	//	vx 4
	fs.read((char*)&pos.vx, sizeof(float));
	//	vy 4
	fs.read((char*)&pos.vy, sizeof(float));
	//	vz 4
	fs.read((char*)&pos.vz, sizeof(float));
	return true;
}

bool ZY3QVProcReader::readSAT_ATT(FILE* pfIn, SAT_ATT& att, FILE* pfOut/* = NULL */)
{
	//	roll 4
	fread((char*)&att.roll, sizeof(float), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&att.roll, sizeof(float), 1, pfOut);
	}
	//	pitch 4
	fread((char*)&att.pitch, sizeof(float), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&att.pitch, sizeof(float), 1, pfOut);
	}
	//	yaw 4
	fread((char*)&att.yaw, sizeof(float), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&att.yaw, sizeof(float), 1, pfOut);
	}
	const float pi = 3.14159265358979f;
	att.roll *= pi / 180.0f;
	att.pitch *= pi / 180.0f;
	att.yaw *= pi / 180.0f;
	return true;
}

bool ZY3QVProcReader::readSAT_ATT(ifstream& fs, SAT_ATT& att)
{
	//	roll 4
	fs.read((char*)&att.roll, sizeof(float));
	//	pitch 4
	fs.read((char*)&att.pitch, sizeof(float));
	//	yaw 4
	fs.read((char*)&att.yaw, sizeof(float));
	const float pi = 3.14159265358979f;
	att.roll *= pi/180.0f;
	att.pitch *= pi/180.0f;
	att.yaw *= pi/180.0f;
	return true;
}

bool ZY3QVProcReader::readRPC_INFO(FILE* pfIn, RPC_STRUCT& rpc_info, FILE* pfOut/* = NULL */)
{
	//	LINE_OFF 8
	fread((char*)&rpc_info.LINE_OFF, 8, 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&rpc_info.LINE_OFF, 8, 1, pfOut);
	}
	fread((char*)&rpc_info.SAMP_OFF, 8, 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&rpc_info.SAMP_OFF, 8, 1, pfOut);
	}
	fread((char*)&rpc_info.LAT_OFF, 8, 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&rpc_info.LAT_OFF, 8, 1, pfOut);
	}
	fread((char*)&rpc_info.LONG_OFF, 8, 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&rpc_info.LONG_OFF, 8, 1, pfOut);
	}
	fread((char*)&rpc_info.HEIGHT_OFF, 8, 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&rpc_info.HEIGHT_OFF, 8, 1, pfOut);
	}
	fread((char*)&rpc_info.LINE_SCALE, 8, 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&rpc_info.LINE_SCALE, 8, 1, pfOut);
	}
	fread((char*)&rpc_info.SAMP_SCALE, 8, 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&rpc_info.SAMP_SCALE, 8, 1, pfOut);
	}
	fread((char*)&rpc_info.LAT_SCALE, 8, 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&rpc_info.LAT_SCALE, 8, 1, pfOut);
	}
	fread((char*)&rpc_info.LONG_SCALE, 8, 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&rpc_info.LONG_SCALE, 8, 1, pfOut);
	}
	fread((char*)&rpc_info.HEIGHT_SCALE, 8, 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&rpc_info.HEIGHT_SCALE, 8, 1, pfOut);
	}
	for (int i = 0; i < 20; i++)
	{
		fread((char*)&rpc_info.LINE_NUM_COEFF[i], 8, 1, pfIn);
		if (pfOut)
		{
			fwrite((char*)&rpc_info.LINE_NUM_COEFF[i], 8, 1, pfOut);
		}
	}
	for (int i = 0; i < 20; i++)
	{
		fread((char*)&rpc_info.LINE_DEN_COEFF[i], 8, 1, pfIn);
		if (pfOut)
		{
			fwrite((char*)&rpc_info.LINE_DEN_COEFF[i], 8, 1, pfOut);
		}
	}
	for (int i = 0; i < 20; i++)
	{
		fread((char*)&rpc_info.SAMP_NUM_COEFF[i], 8, 1, pfIn);
		if (pfOut)
		{
			fwrite((char*)&rpc_info.SAMP_NUM_COEFF[i], 8, 1, pfOut);
		}
	}
	for (int i = 0; i < 20; i++)
	{
		fread((char*)&rpc_info.SAMP_DEN_COEFF[i], 8, 1, pfIn);
		if (pfOut)
		{
			fwrite((char*)&rpc_info.SAMP_DEN_COEFF[i], 8, 1, pfOut);
		}
	}
	return true;
}

bool ZY3QVProcReader::readRPC_INFO(ifstream& fs, RPC_STRUCT& rpc_info)
{
	//	LINE_OFF 8
	fs.read((char*)&rpc_info.LINE_OFF, 8);
	fs.read((char*)&rpc_info.SAMP_OFF, 8);
	fs.read((char*)&rpc_info.LAT_OFF, 8);
	fs.read((char*)&rpc_info.LONG_OFF, 8);
	fs.read((char*)&rpc_info.HEIGHT_OFF, 8);
	fs.read((char*)&rpc_info.LINE_SCALE, 8);
	fs.read((char*)&rpc_info.SAMP_SCALE, 8);
	fs.read((char*)&rpc_info.LAT_SCALE, 8);
	fs.read((char*)&rpc_info.LONG_SCALE, 8);
	fs.read((char*)&rpc_info.HEIGHT_SCALE, 8);
	for (int i = 0;i < 20;i++)
	{
		fs.read((char*)&rpc_info.LINE_NUM_COEFF[i], 8);
	}
	for (int i = 0;i < 20;i++)
	{
		fs.read((char*)&rpc_info.LINE_DEN_COEFF[i], 8);
	}
	for (int i = 0;i < 20;i++)
	{
		fs.read((char*)&rpc_info.SAMP_NUM_COEFF[i], 8);
	}
	for (int i = 0;i < 20;i++)
	{
		fs.read((char*)&rpc_info.SAMP_DEN_COEFF[i], 8);
	}
	return true;
}

bool ZY3QVProcReader::readSceneGlobalInfo(FILE* pfIn, SCENE_GLOBAL_INFO& scene_info, FILE* pfOut/* = NULL */)
{
	//	��ȫ�ָ�����Ϣ֡ͷ 4
	fread((char*)&scene_info.info_header, sizeof(scene_info.info_header), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.info_header, sizeof(scene_info.info_header), 1, pfOut);
	}
	//	������ 4
	fread((char*)&scene_info.scene_count, sizeof(scene_info.scene_count), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.scene_count, sizeof(scene_info.scene_count), 1, pfOut);
	}
	//	��Ӧ�����Ǳ�ʶȫ�� 64
	fread((char*)&scene_info.satellite_id, sizeof(scene_info.satellite_id), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.satellite_id, sizeof(scene_info.satellite_id), 1, pfOut);
	}
	//	��Ӧ�Ĵ�������ʶȫ�� 512
	fread((char*)&scene_info.sensor_id, 512, 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.sensor_id, 512, 1, pfOut);
	}
	//	�þ�ͼ����ģʽ��Ϣ 4
	fread((char*)&scene_info.work_mode, sizeof(scene_info.work_mode), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.work_mode, sizeof(scene_info.work_mode), 1, pfOut);
	}
	//	�þ�ͼ���Ӧ����ͼ������ 4
	fread((char*)&scene_info.scene_lines, sizeof(scene_info.scene_lines), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.scene_lines, sizeof(scene_info.scene_lines), 1, pfOut);
	}
	//	�þ�ͼ���Ӧ����ͼ������ 4
	fread((char*)&scene_info.scene_samples, sizeof(scene_info.scene_samples), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.scene_samples, sizeof(scene_info.scene_samples), 1, pfOut);
	}
	//	����ͼ���и�����Ϣ���ֽ��� 4
	fread((char*)&scene_info.qv_line_info_length, sizeof(scene_info.qv_line_info_length), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.qv_line_info_length, sizeof(scene_info.qv_line_info_length), 1, pfOut);
	}
	//	������ 4
	fread((char*)&scene_info.bands, sizeof(scene_info.bands), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.bands, sizeof(scene_info.bands), 1, pfOut);
	}
	//	����λ�� 4
	fread((char*)&scene_info.sample_bit_count, sizeof(scene_info.sample_bit_count), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.sample_bit_count, sizeof(scene_info.sample_bit_count), 1, pfOut);
	}
	//	�ನ��ͼ�����з�ʽ 4 BSQ BIL BIP
	fread((char*)&scene_info.interleave, 4, 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.interleave, 4, 1, pfOut);
	}
	//	����һ��ͼ��������ʼʱ�� 32
	readTimeInfo(pfIn, scene_info.first_time, pfOut);
	//	�����һ��ͼ�����ʱ�� 32
	readTimeInfo(pfIn, scene_info.last_time, pfOut);
	//	ͼ���ʽ 4
	fread((char*)&scene_info.image_format, sizeof(scene_info.image_format), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.image_format, sizeof(scene_info.image_format), 1, pfOut);
	}
	//	�þ�ͼ��ʹ�õ�ѹ���㷨 4
	fread((char*)&scene_info.compress_algorithm, sizeof(scene_info.compress_algorithm), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.compress_algorithm, sizeof(scene_info.compress_algorithm), 1, pfOut);
	}
	//	�þ�ͼ���ѹ��������Ϣ 4
	fread((char*)&scene_info.compress_rate, sizeof(scene_info.compress_rate), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.compress_rate, sizeof(scene_info.compress_rate), 1, pfOut);
	}
	//	��ͼ�����ݴ�С 8
	int n = sizeof(scene_info.scene_size);
	fread((char*)&scene_info.scene_size, sizeof(scene_info.scene_size), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.scene_size, sizeof(scene_info.scene_size), 1, pfOut);
	}
	//BitInverse((char*)&scene_info.scene_size, sizeof(scene_info.scene_size));
	//	���ϽǾ�γ�� 8
	fread((char*)&scene_info.left_upper.longitude, sizeof(scene_info.left_upper.longitude), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.left_upper.longitude, sizeof(scene_info.left_upper.longitude), 1, pfOut);
	}
	fread((char*)&scene_info.left_upper.latitude, sizeof(scene_info.left_upper.latitude), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.left_upper.latitude, sizeof(scene_info.left_upper.latitude), 1, pfOut);
	}
	//	���ϽǾ�γ�� 8
	fread((char*)&scene_info.right_upper.longitude, sizeof(scene_info.right_upper.longitude), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.right_upper.longitude, sizeof(scene_info.right_upper.longitude), 1, pfOut);
	}
	fread((char*)&scene_info.right_upper.latitude, sizeof(scene_info.right_upper.latitude), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.right_upper.latitude, sizeof(scene_info.right_upper.latitude), 1, pfOut);
	}
	//	���½Ǿ�γ�� 8
	fread((char*)&scene_info.left_lower.longitude, sizeof(scene_info.left_lower.longitude), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.left_lower.longitude, sizeof(scene_info.left_lower.longitude), 1, pfOut);
	}
	fread((char*)&scene_info.left_lower.latitude, sizeof(scene_info.left_lower.latitude), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.left_lower.latitude, sizeof(scene_info.left_lower.latitude), 1, pfOut);
	}
	//	���½Ǿ�γ�� 8
	fread((char*)&scene_info.right_lower.longitude, sizeof(scene_info.right_lower.longitude), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.right_lower.longitude, sizeof(scene_info.right_lower.longitude), 1, pfOut);
	}
	fread((char*)&scene_info.right_lower.latitude, sizeof(scene_info.right_lower.latitude), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.right_lower.latitude, sizeof(scene_info.right_lower.latitude), 1, pfOut);
	}
	//	�����ľ�γ�� 8
	fread((char*)&scene_info.scene_center.longitude, sizeof(scene_info.scene_center.longitude), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.scene_center.longitude, sizeof(scene_info.scene_center.longitude), 1, pfOut);
	}
	fread((char*)&scene_info.scene_center.latitude, sizeof(scene_info.scene_center.latitude), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.scene_center.latitude, sizeof(scene_info.scene_center.latitude), 1, pfOut);
	}
	// RPCϵ�������Ϣ 720
	readRPC_INFO(pfIn, scene_info.rpc_info, pfOut);
	// ԭʼ�����и�����Ϣ��ʾ 2
	fread((char*)&scene_info.valid_flag, 2, 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.valid_flag, 2, 1, pfOut);
	}
	// ԭʼ�����и�����Ϣ��С 4
	fread((char*)&scene_info.line_info_size, sizeof(scene_info.line_info_size), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.line_info_size, sizeof(scene_info.line_info_size), 1, pfOut);
	}
	// ԭʼ�����и�����Ϣ���ֽ��� 4
	fread((char*)&scene_info.line_info_length, sizeof(scene_info.line_info_length), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&scene_info.line_info_length, sizeof(scene_info.line_info_length), 1, pfOut);
	}
	// Ԥ���ռ� 2630
	long nullLength = 2630L;
	fseek(pfIn, nullLength, SEEK_CUR);
	
	if (pfOut)
	{
		const int bufSize = 1024;
		char buffer[bufSize] = { 0 };
		int bufPos = 0;
		while (bufPos < nullLength)
		{
			int nsize = min(bufSize, nullLength - bufPos);
			fwrite(buffer, sizeof(char), nsize, pfOut);
			bufPos += nsize;
		}
	}

	return true;
}

bool ZY3QVProcReader::readSceneGlobalInfo(ifstream& fs, SCENE_GLOBAL_INFO& scene_info)
{
	int pos = fs.tellg();

	//	��ȫ�ָ�����Ϣ֡ͷ 4
	fs.read((char*)&scene_info.info_header, sizeof(scene_info.info_header));
	//	������ 4
	fs.read((char*)&scene_info.scene_count, sizeof(scene_info.scene_count));
	//	��Ӧ�����Ǳ�ʶȫ�� 64
	fs.read((char*)scene_info.satellite_id, sizeof(scene_info.satellite_id));
	//	��Ӧ�Ĵ�������ʶȫ�� 512
	fs.read((char*)scene_info.sensor_id, 512);//sizeof(scene_info.sensor_id));
	//	�þ�ͼ����ģʽ��Ϣ 4
	fs.read((char*)&scene_info.work_mode, sizeof(scene_info.work_mode));
	//	�þ�ͼ���Ӧ����ͼ������ 4
	fs.read((char*)&scene_info.scene_lines, sizeof(scene_info.scene_lines));
	//	�þ�ͼ���Ӧ����ͼ������ 4
	fs.read((char*)&scene_info.scene_samples, sizeof(scene_info.scene_samples));
	//	����ͼ���и�����Ϣ���ֽ��� 4
	fs.read((char*)&scene_info.qv_line_info_length, sizeof(scene_info.qv_line_info_length));
	//	������ 4
	fs.read((char*)&scene_info.bands, sizeof(scene_info.bands));
	//	����λ�� 4
	fs.read((char*)&scene_info.sample_bit_count, sizeof(scene_info.sample_bit_count));
	//	�ನ��ͼ�����з�ʽ 4 BSQ BIL BIP
	fs.read((char*)scene_info.interleave, 4);//sizeof(scene_info.interleave));
	//	����һ��ͼ��������ʼʱ�� 32
	readTimeInfo(fs, scene_info.first_time);
	//	�����һ��ͼ�����ʱ�� 32
	readTimeInfo(fs, scene_info.last_time);
	//	ͼ���ʽ 4
	fs.read((char*)&scene_info.image_format, sizeof(scene_info.image_format));
	//	�þ�ͼ��ʹ�õ�ѹ���㷨 4
	fs.read((char*)&scene_info.compress_algorithm, sizeof(scene_info.compress_algorithm));
	//	�þ�ͼ���ѹ��������Ϣ 4
	fs.read((char*)&scene_info.compress_rate, sizeof(scene_info.compress_rate));
	//	��ͼ�����ݴ�С 8
	int n = sizeof(scene_info.scene_size);
	fs.read((char*)&scene_info.scene_size, sizeof(scene_info.scene_size));
	//BitInverse((char*)&scene_info.scene_size, sizeof(scene_info.scene_size));
	//	���ϽǾ�γ�� 8
	fs.read((char*)&scene_info.left_upper.longitude, sizeof(scene_info.left_upper.longitude));
	fs.read((char*)&scene_info.left_upper.latitude, sizeof(scene_info.left_upper.latitude));
	//	���ϽǾ�γ�� 8
	fs.read((char*)&scene_info.right_upper.longitude, sizeof(scene_info.right_upper.longitude));
	fs.read((char*)&scene_info.right_upper.latitude, sizeof(scene_info.right_upper.latitude));
	//	���½Ǿ�γ�� 8
	fs.read((char*)&scene_info.left_lower.longitude, sizeof(scene_info.left_lower.longitude));
	fs.read((char*)&scene_info.left_lower.latitude, sizeof(scene_info.left_lower.latitude));
	//	���½Ǿ�γ�� 8
	fs.read((char*)&scene_info.right_lower.longitude, sizeof(scene_info.right_lower.longitude));
	fs.read((char*)&scene_info.right_lower.latitude, sizeof(scene_info.right_lower.latitude));
	//	�����ľ�γ�� 8
	fs.read((char*)&scene_info.scene_center.longitude, sizeof(scene_info.scene_center.longitude));
	fs.read((char*)&scene_info.scene_center.latitude, sizeof(scene_info.scene_center.latitude));
	// RPCϵ�������Ϣ 720
	readRPC_INFO(fs, scene_info.rpc_info);
	// ԭʼ�����и�����Ϣ��ʾ 2
	fs.read((char*)scene_info.valid_flag, 2);
	// ԭʼ�����и�����Ϣ��С 4
	fs.read((char*)&scene_info.line_info_size, sizeof(scene_info.line_info_size));
	// ԭʼ�����и�����Ϣ���ֽ��� 4
	fs.read((char*)&scene_info.line_info_length, sizeof(scene_info.line_info_length));
	// Ԥ���ռ� 2630
	fs.ignore(2630);

	int pos1 = fs.tellg();
	int db = pos1 - pos;
	return true;
}

bool ZY3QVProcReader::readQuivAuxInfo(FILE* pfIn, QUIVIMAGE_AUX_INFO& aux, FILE* pfOut/* = NULL */)
{
	bool bState = true;

	//	����ͼ���и�����Ϣ���� 4
	fread((char*)&aux.line_index, sizeof(aux.line_index), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&aux.line_index, sizeof(aux.line_index), 1, pfOut);
	}
	//	��Ӧ�����µ��λ�� 8
	fread((char*)&aux.nadir_pos.longitude, sizeof(aux.nadir_pos.longitude), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&aux.nadir_pos.longitude, sizeof(aux.nadir_pos.longitude), 1, pfOut);
	}
	fread((char*)&aux.nadir_pos.latitude, sizeof(aux.nadir_pos.latitude), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&aux.nadir_pos.latitude, sizeof(aux.nadir_pos.latitude), 1, pfOut);
	}
	//	�þ����߶�Ӧ������λ�ã����Ⱥ�γ�ȣ� 8
	fread((char*)&aux.line_left_pos.longitude, sizeof(aux.line_left_pos.longitude), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&aux.line_left_pos.longitude, sizeof(aux.line_left_pos.longitude), 1, pfOut);
	}
	fread((char*)&aux.line_left_pos.latitude, sizeof(aux.line_left_pos.latitude), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&aux.line_left_pos.latitude, sizeof(aux.line_left_pos.latitude), 1, pfOut);
	}
	//	�þ����߶�Ӧ���Ҳ��λ�ã����Ⱥ�γ�ȣ� 8
	fread((char*)&aux.line_right_pos.longitude, sizeof(aux.line_right_pos.longitude), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&aux.line_right_pos.longitude, sizeof(aux.line_right_pos.longitude), 1, pfOut);
	}
	fread((char*)&aux.line_right_pos.latitude, sizeof(aux.line_right_pos.latitude), 1, pfIn);
	if (pfOut)
	{
		fwrite((char*)&aux.line_right_pos.latitude, sizeof(aux.line_right_pos.latitude), 1, pfOut);
	}
	//	���ǵ�x,y,z�����Լ�vx,vy,vz 24
	readSAT_POS(pfIn, aux.satpos, pfOut);
	//	���ǵ���̬����,��������ƫ����r,p,y 12
	readSAT_ATT(pfIn, aux.satatt, pfOut);
	//  ����ʱ�� 32 
	//readTimeInfo(pfIn, aux.line_time, pfOut);
	_TIMESTAMP line_time;
	readTimeInfo(pfIn, line_time, pfOut);
	aux.line_time = convertTimeStamp(time2String(line_time));
	aux.att_time = aux.line_time;
	aux.star_time = aux.line_time;
	// Ԥ���ռ�
	long nullLength = 160L;
	fseek(pfIn, nullLength, SEEK_CUR);

	if (pfOut)
	{
		const int bufSize = 1024;
		char buffer[bufSize] = { 0 };
		int bufPos = 0;
		while (bufPos < nullLength)
		{
			int nsize = min(bufSize, nullLength - bufPos);
			fwrite(buffer, sizeof(char), nsize, pfOut);
			bufPos += nsize;
		}
	}

	return bState;
}

bool ZY3QVProcReader::readQuivAuxInfo(ifstream& fs, QUIVIMAGE_AUX_INFO& aux)
{
	bool bState = true;
	int pos = fs.tellg();
	
	//	����ͼ���и�����Ϣ���� 4
	fs.read((char*)&aux.line_index, sizeof(aux.line_index));
	//	��Ӧ�����µ��λ�� 8
	fs.read((char*)&aux.nadir_pos.longitude, sizeof(aux.nadir_pos.longitude));
	fs.read((char*)&aux.nadir_pos.latitude, sizeof(aux.nadir_pos.latitude));
	//	�þ����߶�Ӧ������λ�ã����Ⱥ�γ�ȣ� 8
	fs.read((char*)&aux.line_left_pos.longitude, sizeof(aux.line_left_pos.longitude));
	fs.read((char*)&aux.line_left_pos.latitude, sizeof(aux.line_left_pos.latitude));
	//	�þ����߶�Ӧ���Ҳ��λ�ã����Ⱥ�γ�ȣ� 8
	fs.read((char*)&aux.line_right_pos.longitude, sizeof(aux.line_right_pos.longitude));
	fs.read((char*)&aux.line_right_pos.latitude, sizeof(aux.line_right_pos.latitude));
	//	���ǵ�x,y,z�����Լ�vx,vy,vz 24
	readSAT_POS(fs, aux.satpos);
	//	������̬ģʽ��0Ϊr,p,y��1Ϊ��Ԫ��
	fs.read((char*)&aux.att_model, sizeof(aux.att_model));
	//	���ǵ���̬����,��������ƫ����r,p,y 12
	readSAT_ATT(fs, aux.satatt);
	//  ����ʱ�� 32 
	_TIMESTAMP line_time;
	readTimeInfo(fs, line_time);
	aux.line_time = convertTimeStamp(time2String(line_time));
	aux.att_time = aux.line_time;
	aux.star_time = aux.line_time;
	// Ԥ���ռ�
	//fs.ignore(160);
	fs.ignore(156);		// ����������̬ģʽ��

	int pos1 = fs.tellg();
	int db = pos1 - pos;
	return bState;
}

void ZY3QVProcReader::writeDescXML(const QUIVIMAGE_HEAD_INFO& headerInfo, const char* descXML)const
{
	// get a test document
	pugi::xml_document doc;
	doc.load("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");

	pugi::xml_node DATASET_NODE = doc.append_child("DATASET");
	pugi::xml_node BASE_INFO_NODE = DATASET_NODE.append_child("BASE_INFO");
	BASE_INFO_NODE.append_child("PROCESSING").append_child(pugi::node_pcdata).set_value("0");
	BASE_INFO_NODE.append_child("STATION_ID").append_child(pugi::node_pcdata).set_value(headerInfo.station_id);
	BASE_INFO_NODE.append_child("SPACECRAFT_ID").append_child(pugi::node_pcdata).set_value(headerInfo.satellite_id);
	BASE_INFO_NODE.append_child("SENSOR_ID").append_child(pugi::node_pcdata).set_value(headerInfo.sensor_id); 
	BASE_INFO_NODE.append_child("START_TIME").append_child(pugi::node_pcdata).set_value(time2String(headerInfo.start_time).c_str());
	BASE_INFO_NODE.append_child("STOP_TIME").append_child(pugi::node_pcdata).set_value(time2String(headerInfo.end_time).c_str());
	BASE_INFO_NODE.append_child("COLUMNS").append_child(pugi::node_pcdata).set_value(boost::lexical_cast<std::string>(theSamples).c_str());
	BASE_INFO_NODE.append_child("LINES").append_child(pugi::node_pcdata).set_value(boost::lexical_cast<std::string>(theLines).c_str());
	BASE_INFO_NODE.append_child("BANDS").append_child(pugi::node_pcdata).set_value(boost::lexical_cast<std::string>(theBands).c_str());
	BASE_INFO_NODE.append_child("PYRAMID_LEVELS").append_child(pugi::node_pcdata).set_value(boost::lexical_cast<std::string>(headerInfo.desample_num).c_str());
	BASE_INFO_NODE.append_child("IMAGE_TILE_SIDE").append_child(pugi::node_pcdata).set_value("512");
	BASE_INFO_NODE.append_child("IFCITY").append_child(pugi::node_pcdata).set_value("0");
	BASE_INFO_NODE.append_child("IFPROVINCE").append_child(pugi::node_pcdata).set_value("0");
	BASE_INFO_NODE.append_child("IFEPH").append_child(pugi::node_pcdata).set_value("0");
	BASE_INFO_NODE.append_child("IFNADIR").append_child(pugi::node_pcdata).set_value("0");
	BASE_INFO_NODE.append_child("GRID_TILE_SIDE").append_child(pugi::node_pcdata).set_value("32");
	//end of BASE_INFO

	doc.save_file(descXML, "\t");
}


//void ZY3QVProcReader::writeDescXML(const QUIVIMAGE_HEAD_INFO& headerInfo, const char* descXML)const
//{
//	QFile file(descXML);
//	if (!file.open(QIODevice::WriteOnly))
//	{
//		std::cout << "Read only. The file is in read only mode" << endl;
//	}
//	else
//	{
//		QXmlStreamWriter* xmlWriter = new QXmlStreamWriter();
//		xmlWriter->setDevice(&file);
//		xmlWriter->writeStartDocument();
//		xmlWriter->setAutoFormatting(true);
//		xmlWriter->setAutoFormattingIndent(true);
//		xmlWriter->writeStartElement("DATASET");
//		xmlWriter->writeStartElement("BASE_INFO");
//
//		xmlWriter->writeStartElement("PROCESSING");
//		xmlWriter->writeCharacters("0");
//		xmlWriter->writeEndElement();
//
//		xmlWriter->writeStartElement("STATION_ID");
//		xmlWriter->writeCharacters(headerInfo.station_id);
//		xmlWriter->writeEndElement();
//
//		xmlWriter->writeStartElement("SPACECRAFT_ID");
//		xmlWriter->writeCharacters(headerInfo.satellite_id);
//		xmlWriter->writeEndElement();
//
//		xmlWriter->writeStartElement("SENSOR_ID");
//		xmlWriter->writeCharacters(headerInfo.sensor_id);
//		xmlWriter->writeEndElement();
//
//		xmlWriter->writeStartElement("RESAMPLE_RATE");
//		xmlWriter->writeCharacters(QString::number(headerInfo.sample_bit_count));
//		xmlWriter->writeEndElement();
//
//		xmlWriter->writeStartElement("START_TIME");
//		xmlWriter->writeCharacters(time2String(headerInfo.start_time).c_str());
//		xmlWriter->writeEndElement();
//
//		xmlWriter->writeStartElement("STOP_TIME");
//		xmlWriter->writeCharacters(time2String(headerInfo.end_time).c_str());
//		xmlWriter->writeEndElement();
//
//		xmlWriter->writeStartElement("COLUMNS");
//		xmlWriter->writeCharacters(QString::number(headerInfo.sample_num));
//		xmlWriter->writeEndElement();
//
//		xmlWriter->writeStartElement("LINES");
//		xmlWriter->writeCharacters(QString::number(headerInfo.line_num));
//		xmlWriter->writeEndElement();
//
//		xmlWriter->writeStartElement("BANDS");
//		xmlWriter->writeCharacters(QString::number(headerInfo.band_num));
//		xmlWriter->writeEndElement();
//
//		xmlWriter->writeStartElement("PYRAMID_LEVELS");
//		xmlWriter->writeCharacters(QString::number(headerInfo.desample_num));
//		xmlWriter->writeEndElement();
//
//		xmlWriter->writeStartElement("IMAGE_TILE_SIDE");
//		xmlWriter->writeCharacters(QString::number(512));
//		xmlWriter->writeEndElement();
//
//		xmlWriter->writeStartElement("IFCITY");
//		xmlWriter->writeCharacters(QString::number(0));
//		xmlWriter->writeEndElement();
//
//		xmlWriter->writeStartElement("IFPROVINCE");
//		xmlWriter->writeCharacters(QString::number(0));
//		xmlWriter->writeEndElement();
//
//		xmlWriter->writeStartElement("IFEPH");
//		xmlWriter->writeCharacters(QString::number(1));
//		xmlWriter->writeEndElement();
//
//		xmlWriter->writeStartElement("IFNADIR");
//		xmlWriter->writeCharacters(QString::number(0));
//		xmlWriter->writeEndElement();
//
//		xmlWriter->writeStartElement("GRID_TILE_SIDE");
//		xmlWriter->writeCharacters(QString::number(32));
//		xmlWriter->writeEndElement();
//
//		xmlWriter->writeEndElement();
//		xmlWriter->writeEndElement();
//		xmlWriter->writeEndDocument();
//	}
//	//fstream fs;
//	//fs.open(descXML, ios_base::out);
//	//fs<<"<?xml version=\"1.0\"?>"<<endl;
//	//fs<<"<DATASET>"<<endl;
//	//fs<<"  <BASE_INFO>"<<endl;
//	//fs<<"    <PROCESSING>"<<0<<"</PROCESSING>"<<endl;
//	//fs<<"    <STATION_ID>"<<headerInfo.station_id<<"</STATION_ID>"<<endl;
//	//fs<<"    <SPACECRAFT_ID>"<<headerInfo.satellite_id<<"</SPACECRAFT_ID>"<<endl;
//	//fs<<"    <SENSOR_ID>"<<headerInfo.sensor_id<<"</SENSOR_ID>"<<endl;
//	//fs<<"    <RESAMPLE_RATE>"<<headerInfo.sample_bit_count/8<<"</RESAMPLE_RATE>"<<endl;
//	//fs<<"    <START_TIME>"<<time2String(headerInfo.start_time)<<"</START_TIME>"<<endl;
//	//fs<<"    <STOP_TIME>"<<time2String(headerInfo.end_time)<<"</STOP_TIME>"<<endl;
//	//fs<<"    <COLUMNS>"<<headerInfo.sample_num<<"</COLUMNS>"<<endl;
//	//fs<<"    <LINES>"<<headerInfo.line_num<<"</LINES>"<<endl;
//	//fs<<"    <BANDS>"<<headerInfo.band_num<<"</BANDS>"<<endl;
//	//fs<<"    <PYRAMID_LEVELS>"<<headerInfo.desample_num<<"</PYRAMID_LEVELS>"<<endl;
//	//fs<<"    <IMAGE_TILE_SIDE>"<<512<<"</IMAGE_TILE_SIDE>"<<endl;
//	//fs<<"    <IFCITY>"<<0<<"</IFCITY>"<<endl;
//	//fs<<"    <IFPROVINCE>"<<0<<"</IFPROVINCE>"<<endl;
//	//fs<<"    <IFAUXINFO>"<<0<<"</IFAUXINFO>"<<endl;
//	//fs<<"    <IFEPH>"<<1<<"</IFEPH>"<<endl;
//	//fs<<"    <IFNADIR>"<<0<<"</IFNADIR>"<<endl;
//	//fs<<"    <GRID_TILE_SIDE>"<<32<<"</GRID_TILE_SIDE>"<<endl;
//	//fs<<"  </BASE_INFO>"<<endl;
//	//fs<<"</DATASET>"<<endl;
//}


void ZY3QVProcReader::writeDescXML(const QUIVIMAGE_HEAD_INFO& headerInfo,
	const SCENE_GLOBAL_INFO& sceneInfo,
	const vector<QUIVIMAGE_AUX_INFO>& auxList,
	const char* descXML,
	int line_offset/* = 0*/)const
{
	// get a test document
	pugi::xml_document doc;
	doc.load("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");

	pugi::xml_node DATASET_NODE = doc.append_child("DATASET");
	pugi::xml_node BASE_INFO_NODE = DATASET_NODE.append_child("BASE_INFO");
	BASE_INFO_NODE.append_child("PROCESSING").append_child(pugi::node_pcdata).set_value("0");
	BASE_INFO_NODE.append_child("STATION_ID").append_child(pugi::node_pcdata).set_value(headerInfo.station_id);
	BASE_INFO_NODE.append_child("SPACECRAFT_ID").append_child(pugi::node_pcdata).set_value(headerInfo.satellite_id);
	BASE_INFO_NODE.append_child("SENSOR_ID").append_child(pugi::node_pcdata).set_value(headerInfo.sensor_id);
	BASE_INFO_NODE.append_child("START_TIME").append_child(pugi::node_pcdata).set_value(time2String(headerInfo.start_time).c_str());
	BASE_INFO_NODE.append_child("STOP_TIME").append_child(pugi::node_pcdata).set_value(time2String(headerInfo.end_time).c_str());
	BASE_INFO_NODE.append_child("COLUMNS").append_child(pugi::node_pcdata).set_value(boost::lexical_cast<std::string>(theSamples).c_str());
	BASE_INFO_NODE.append_child("LINES").append_child(pugi::node_pcdata).set_value(boost::lexical_cast<std::string>(theLines).c_str());
	BASE_INFO_NODE.append_child("BANDS").append_child(pugi::node_pcdata).set_value(boost::lexical_cast<std::string>(theBands).c_str());
	BASE_INFO_NODE.append_child("PYRAMID_LEVELS").append_child(pugi::node_pcdata).set_value(boost::lexical_cast<std::string>(headerInfo.desample_num).c_str());
	BASE_INFO_NODE.append_child("IMAGE_TILE_SIDE").append_child(pugi::node_pcdata).set_value("512");
	BASE_INFO_NODE.append_child("IFCITY").append_child(pugi::node_pcdata).set_value("0");
	BASE_INFO_NODE.append_child("IFPROVINCE").append_child(pugi::node_pcdata).set_value("0");
	BASE_INFO_NODE.append_child("IFEPH").append_child(pugi::node_pcdata).set_value("0");
	BASE_INFO_NODE.append_child("IFNADIR").append_child(pugi::node_pcdata).set_value("0");
	BASE_INFO_NODE.append_child("GRID_TILE_SIDE").append_child(pugi::node_pcdata).set_value("32");
	//end of BASE_INFO

	vector<QUIVIMAGE_AUX_INFO>::const_iterator aux;

	// Ephemeris
	pugi::xml_node Ephemeris_NODE = DATASET_NODE.append_child("Ephemeris");
	for (aux = auxList.begin();
		aux != auxList.end();
		aux++)
	{
		//<Point>
		// <TIME>349234357.6785600185</TIME>
		// <Location>
		//  <X>-3763220.5000000000</X>
		//  <Y>2643178.2500000000</Y>
		//  <Z>5469612.5000000000</Z>
		// </Location>
		// <Velocity>
		//  <X>-3372.7299804688</X>
		//  <Y>4885.5000000000</Y>
		//  <Z>-4670.6000976562</Z>
		// </Velocity>
		//</Point>
		pugi::xml_node Point_NODE = Ephemeris_NODE.append_child("Point");
		Point_NODE.append_child("TIME").append_child(pugi::node_pcdata).set_value(boost::str(boost::format("%.10lf") % aux->line_time).c_str());
		pugi::xml_node Location_NODE = Point_NODE.append_child("Location");
		Location_NODE.append_child("X").append_child(pugi::node_pcdata).set_value(boost::str(boost::format("%.10lf") % aux->satpos.x).c_str());
		Location_NODE.append_child("Y").append_child(pugi::node_pcdata).set_value(boost::str(boost::format("%.10lf") % aux->satpos.y).c_str());
		Location_NODE.append_child("Z").append_child(pugi::node_pcdata).set_value(boost::str(boost::format("%.10lf") % aux->satpos.z).c_str());
		pugi::xml_node Velocity_NODE = Point_NODE.append_child("Velocity");
		Velocity_NODE.append_child("X").append_child(pugi::node_pcdata).set_value(boost::str(boost::format("%.10lf") % aux->satpos.vx).c_str());
		Velocity_NODE.append_child("Y").append_child(pugi::node_pcdata).set_value(boost::str(boost::format("%.10lf") % aux->satpos.vy).c_str());
		Velocity_NODE.append_child("Z").append_child(pugi::node_pcdata).set_value(boost::str(boost::format("%.10lf") % aux->satpos.vz).c_str());
	}


	// Attitudes
	pugi::xml_node Attitudes_NODE = DATASET_NODE.append_child("Attitudes");
	for (aux = auxList.begin();
		aux != auxList.end();
		aux++)
	{
		pugi::xml_node Attitude_NODE = Attitudes_NODE.append_child("Attitude");
		Attitude_NODE.append_child("TIME").append_child(pugi::node_pcdata).set_value(boost::str(boost::format("%.10lf") % aux->att_time).c_str());
		pugi::xml_node Angle_NODE = Attitude_NODE.append_child("Angle");
		Angle_NODE.append_child("YAW").append_child(pugi::node_pcdata).set_value(boost::str(boost::format("%.10lf") % aux->satatt.yaw).c_str());
		Angle_NODE.append_child("PITCH").append_child(pugi::node_pcdata).set_value(boost::str(boost::format("%.10lf") % aux->satatt.pitch).c_str());
		Angle_NODE.append_child("ROLL").append_child(pugi::node_pcdata).set_value(boost::str(boost::format("%.10lf") % aux->satatt.roll).c_str());
		pugi::xml_node Angle_Velocity_NODE = Attitude_NODE.append_child("Angle_Velocity");
		Angle_Velocity_NODE.append_child("YAW").append_child(pugi::node_pcdata).set_value(boost::str(boost::format("%.10lf") % aux->satatt.vyaw).c_str());
		Angle_Velocity_NODE.append_child("PITCH").append_child(pugi::node_pcdata).set_value(boost::str(boost::format("%.10lf") % aux->satatt.vpitch).c_str());
		Angle_Velocity_NODE.append_child("ROLL").append_child(pugi::node_pcdata).set_value(boost::str(boost::format("%.10lf") % aux->satatt.vroll).c_str());
	}

	// Line Times
	pugi::xml_node Line_Times_NODE = DATASET_NODE.append_child("Line_Times");
	for (aux = auxList.begin();
		aux != auxList.end();
		aux++)
	{
		pugi::xml_node Line_Time_NODE = Line_Times_NODE.append_child("Line_Time");
		Line_Time_NODE.append_child("TIME").append_child(pugi::node_pcdata).set_value(boost::str(boost::format("%.10lf") % aux->star_time).c_str());
		Line_Time_NODE.append_child("LINE").append_child(pugi::node_pcdata).set_value(boost::str(boost::format("%.1lf") % (aux->line_index)).c_str());
	}

	doc.save_file(descXML, "\t");
}