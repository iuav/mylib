#include "AlosBatch.h"
/*
* AlosBatch Alos������������У�����캯��
* @Param
* wxString strInputPath: ��У������Ŀ¼
* wxString strOutputPath: ���Ŀ¼
* int nType: ��������(Ĭ��Ϊ0)
*		0: ��ʾ����PRISM��AVNIR2����
*		1: ��ʾֻ����PRISM����
*		2: ��ʾֻ����AVNIR2����
*/
AlosBatch::AlosBatch(ossimFilename strInputPath, ossimFilename strOutputPath, int nType/* = 0*/)
:m_InputPath(strInputPath),m_OutputPath(strOutputPath)
{
	QStringList strListTmp;
	m_AlosTask.clear();
	unsigned int i;
	if(0 == nType)
	{//����PRISM��AVNIR2

		//PRISM
		strListTmp.clear();
		QFindFile(strInputPath.c_str(), strListTmp, QStringList("IMG-ALPSMW*_O1B2R_UW.tif"));
		for(i = 0;i < strListTmp.size();i++)
		{
			AlosTask task;
			task.inputPath = ossimFilename(QBeforeLast(strListTmp[i], '\\').toLatin1());
			task.type = PRISM;
			m_AlosTask.push_back(task);
		}

		//AVNIR2
		strListTmp.clear();
		QFindFile(strInputPath.c_str(), strListTmp, QStringList("IMG-ALAV2*_O1B2R_U.tif"));
		for(i = 0;i < strListTmp.size();i++)
		{
			AlosTask task;
			task.inputPath = ossimFilename(QBeforeLast(strListTmp[i], '\\').toLatin1());
			task.type = AVNIR2;
			m_AlosTask.push_back(task);
		}
	}
	else if(1 == nType)
	{
		//PRISM
		strListTmp.clear();
		QFindFile(strInputPath.c_str(), strListTmp, QStringList("IMG-ALPSMW*_O1B2R_UW.tif"));
		for(i = 0;i < strListTmp.size();i++)
		{
			AlosTask task;
			task.inputPath = ossimFilename(QBeforeLast(strListTmp[i], '\\').toLatin1());
			task.type = PRISM;
			m_AlosTask.push_back(task);
		}

	}
	else if(2 == nType)
	{
		//AVNIR2
		strListTmp.clear();
		QFindFile(strInputPath.c_str(), strListTmp, QStringList("IMG-ALAV2*_O1B2R_U.tif"));
		for(i = 0;i < strListTmp.size();i++)
		{
			AlosTask task;
			task.inputPath = ossimFilename(QBeforeLast(strListTmp[i], '\\').toLatin1());
			task.type = AVNIR2;
			m_AlosTask.push_back(task);
		}

	}
	m_nFinished = 0;
	m_nFailed = 0;
	m_nCurrentTask = 0;
	m_nTask = m_AlosTask.size();
}


/*
* AlosBatch Alos������������У��ִ�к���
*/
bool AlosBatch::Run()
{
	int i;
	//��ȡϵͳʱ��
	time_t tm;
	tm = time(NULL);
	char tmp[64];
	strftime(tmp,sizeof(tmp),"%Y-%m-%d %X",localtime(&tm));
	for(i = 0;i < sizeof(tmp);i++)
	{
		if(':' == tmp[i])
			tmp[i] = '-';
	}
	ossimFilename strLogFile = m_OutputPath +  "\\log_" + ossimFilename(tmp) + ".txt";

	QString strFinished = QString::number(m_nFinished);
	addLogLine(strLogFile, ossimFilename(strFinished.toLatin1()));

	m_nFinished = 0;
	m_nFailed = 0;
	m_nCurrentTask = 0;
	m_nTask = m_AlosTask.size();
	for(m_nCurrentTask = 0;m_nCurrentTask < m_nTask;m_nCurrentTask++)
	{
		cout<<"�������"<<m_nTask<<"���еĵ�"<<m_nCurrentTask + 1<<"��..."<<endl;
		switch(m_AlosTask[m_nCurrentTask].type)
		{
		case PRISM:
			{
				if(Alos_PRISM_Rpc(m_AlosTask[m_nCurrentTask].inputPath, m_ElevationPath, m_EgmFile, m_OutputPath, strLogFile))
				{
					m_nFinished++;
				}
				else
					m_nFailed++;
				break;
			}
		case AVNIR2:
			{
				if(Alos_AVNIR2_Rpc(m_AlosTask[m_nCurrentTask].inputPath, m_ElevationPath, m_EgmFile, m_OutputPath,strLogFile))
				{
					m_nFinished++;
				}
				else
					m_nFailed++;
				break;
			}
		}
		QString strFinished = QString::number(m_nFinished);
		setLogLine(strLogFile, 0, ossimFilename(strFinished.toLatin1()));
	}
	return true;
}


bool AlosBatch::Alos_PRISM_Rpc(ossimFilename AlosDir, ossimFilename elevationpath, ossimFilename EGMfile,
							   ossimFilename outPath, ossimFilename logFile/* = wxT("")*/)
{
	MyProject prj;
	AlosPRISM alosUti(AlosDir);
	ossimFilename outfile = outPath + "\\" + AlosDir.afterPos(AlosDir.find_last_of('\\')) + "_"  + alosUti.getSceneID() + ".TIF";
	ossimFilename gcpfile = AlosDir + "\\gcp.txt";
	ossimFilename reportfile = AlosDir + "\\report.txt";
	bool bReport = true;
	ossimString strCurrentTask = ossimString::toString(m_nCurrentTask + 1);
	if(!alosUti.getInitState())
	{
		//cout<<"warning: Alos PRISM����\""<<AlosDir<<"\"��ʼ��ʧ�ܣ�"<<endl;
		//ossimString strState = "warning: Alos PRISM����\"" + AlosDir + "\"��ʼ��ʧ�ܣ�";
		ossimString strState = "ʧ��";
		time_t tm;
		tm = time(NULL);
		char tmp[64];
		strftime(tmp,sizeof(tmp),"%Y/%m/%d %X",localtime(&tm));
		ossimString strContent = strCurrentTask + "\t" + alosUti.getSceneID() + "\tPRISM\t" + tmp + "\t" + outfile + "\t" + strState + "\t����У��";
		addLogLine(logFile, strContent);
		return false;
	}
	//prj.theMgr = ossimElevManager::instance();
	//if(!prj.theMgr->loadElevationPath(ossimFilename(elevationpath)))
	//{
	//	//cout<<"warning: ����DEMʧ�ܣ�"<<endl;
	//	//ossimString strState = "warning: ����DEMʧ�ܣ�";
	//	ossimString strState = "ʧ��";
	//	time_t tm;
	//	tm = time(NULL);
	//	char tmp[64];
	//	strftime(tmp,sizeof(tmp),"%Y/%m/%d %X",localtime(&tm));
	//	ossimString strContent = strCurrentTask + "\t" + alosUti.getSceneID() + "\tPRISM\t" + tmp + "\t" + outfile + "\t" + strState + "\t����У��";
	//	addLogLine(logFile, ossimString2wxString(strContent));
	//	return false;
	//}
	/*if(!prj.theMgr->openMGH(EGMfile))
	{
	//cout<<"warning: ���ش��ˮ׼������ʧ�ܣ�"<<endl;
	//ossimString strState = "warning: ���ش��ˮ׼������ʧ�ܣ�";
	ossimString strState = "ʧ��";
	time_t tm;
	tm = time(NULL);
	char tmp[64];
	strftime(tmp,sizeof(tmp),"%Y/%m/%d %X",localtime(&tm));
	ossimString strContent = strCurrentTask + "\t" + alosUti.getSceneID() + "\tPRISM\t" + tmp + "\t" + outfile + "\t" + strState + "\t����У��";
	addLogLine(logFile, strContent);
	return false;
	}*/
	prj.m_DemPath = ossimFilename(elevationpath);
	prj.m_MapProjection = alosUti.getKeywordlist();
	prj.m_MapPar = alosUti.getMapProjection();
	if(gcpfile.exists())
	{
		prj.ReadGcpAndProjection(ossimFilename(gcpfile));
		prj.GetElevations(prj.m_CtrlGptSet);
	}
	else
	{
		prj.m_CtrlGptSet = new ossimTieGptSet;
	}

	prj.m_ImgFileNameUnc = ossimFilename(alosUti.m_FileTIF);
	prj.m_OutBandList.clear();
	prj.m_OutBandList.push_back(0);

	prj.m_ModelType = RPCType;
	if(!prj.InitiateSensorModel(prj.m_ImgFileNameUnc))
	{
		//cout<<"warning: δ�ҵ��ʺ�\""<<prj.m_ImgFileNameUnc<<"\"��ģ�ͣ�"<<endl;
		//ossimString strState = "warning: δ�ҵ��ʺ�\"" + prj.m_ImgFileNameUnc + "\"��ģ�ͣ�";
		ossimString strState = "ʧ��";
		time_t tm;
		tm = time(NULL);
		char tmp[64];
		strftime(tmp,sizeof(tmp),"%Y/%m/%d %X",localtime(&tm));
		ossimString strContent = strCurrentTask + "\t" + alosUti.getSceneID() + "\tPRISM\t" + tmp + "\t" + outfile + "\t" + strState + "\t����У��";
		addLogLine(logFile, strContent);
		return false;
	}
	ossimRpcModel *rpcModel = new ossimRpcModel;
	rpcModel->setAttributes(alosUti.getRpcModelStruct());
	prj.m_sensorModel = rpcModel;
	prj.m_sensorModel->m_proj = prj.m_MapPar;
	prj.UpdateSensorModel(*prj.m_CtrlGptSet, prj.m_sensorModel, prj.geom);
	if(bReport)
		prj.OutputReport(reportfile, prj.m_sensorModel, prj.m_CtrlGptSet, prj.m_ChkGptSet);

	//cout<<prj.geom;
	if(!prj.Orthograph(outfile))
	{
		//ossimString strState = "warning: ����ͼ��\"" + outfile + "\"ʧ�ܣ�";
		ossimString strState = "ʧ��";
		time_t tm;
		tm = time(NULL);
		char tmp[64];
		strftime(tmp,sizeof(tmp),"%Y/%m/%d %X",localtime(&tm));
		ossimString strContent = strCurrentTask + "\t" + alosUti.getSceneID() + "\tPRISM\t" + tmp + "\t" + outfile + "\t" + strState + "\t����У��";
		addLogLine(logFile, strContent);
		return false;

	}


	ossimString strState = "�ɹ�";
	time_t tm;
	tm = time(NULL);
	char tmp[64];
	strftime(tmp,sizeof(tmp),"%Y/%m/%d %X",localtime(&tm));
	ossimString strContent = strCurrentTask + "\t" + alosUti.getSceneID() + "\tPRISM\t" + tmp + "\t" + outfile + "\t" + strState + "\t����У��";
	addLogLine(logFile, strContent);

	return true;
}
bool AlosBatch::Alos_AVNIR2_Rpc(ossimFilename AlosDir, ossimFilename elevationpath, ossimFilename EGMfile,
								ossimFilename outPath, ossimFilename logFile/* = wxT("")*/)
{
	MyProject prj;
	AlosAVNIR2 alosUti(AlosDir);
	ossimFilename outfile = outPath + "\\" + AlosDir.afterPos(AlosDir.find_last_of('\\')) + "_" + alosUti.getSceneID() + ".TIF";
	ossimFilename gcpfile = AlosDir + "\\gcp.txt";
	ossimFilename reportfile = AlosDir + "\\report.txt";
	bool bReport = true;
	ossimString strCurrentTask = ossimString::toString(m_nCurrentTask + 1);
	if(!alosUti.getInitState())
	{
		//cout<<"warning: Alos AVNIR2����\""<<AlosDir<<"\"��ʼ��ʧ�ܣ�"<<endl;
		//ossimString strState = "warning: Alos PRISM����\"" + AlosDir + "\"��ʼ��ʧ�ܣ�";
		ossimString strState = "ʧ��";
		time_t tm;
		tm = time(NULL);
		char tmp[64];
		strftime(tmp,sizeof(tmp),"%Y/%m/%d %X",localtime(&tm));
		ossimString strContent = strCurrentTask + "\t" + alosUti.getSceneID() + "\tAVNIR2\t" + tmp + "\t" + outfile + "\t" + ossimString(strState) + "\t����У��";
		addLogLine(logFile, strContent);
		return false;
	}
	prj.theMgr = ossimElevManager::instance();
	if(!prj.theMgr->loadElevationPath(ossimFilename(elevationpath)))
	{
		cout<<"warning: ����DEMʧ�ܣ�"<<endl;
		//ossimString strState = "warning: ����DEMʧ�ܣ�";
		ossimString strState = "ʧ��";
		time_t tm;
		tm = time(NULL);
		char tmp[64];
		strftime(tmp,sizeof(tmp),"%Y/%m/%d %X",localtime(&tm));
		ossimString strContent = strCurrentTask + "\t" + alosUti.getSceneID() + "\tAVNIR2\t" + tmp + "\t" + outfile + "\t" + ossimString(strState) + "\t����У��";
		addLogLine(logFile, strContent);
		return false;
	}
	/*if(!prj.theMgr->openMGH(EGMfile))
	{
	cout<<"warning: ���ش��ˮ׼������ʧ�ܣ�"<<endl;
	//ossimString strState = "warning: ���ش��ˮ׼������ʧ�ܣ�";
	ossimString strState = "ʧ��";
	time_t tm;
	tm = time(NULL);
	char tmp[64];
	strftime(tmp,sizeof(tmp),"%Y/%m/%d %X",localtime(&tm));
	ossimString strContent = strCurrentTask + "\t" + alosUti.getSceneID() + "\tAVNIR2\t" + tmp + "\t" + outfile + "\t" + ossimString(strState) + "\t����У��";
	addLogLine(logFile, strContent.c_str());
	return false;
	}*/
	prj.m_DemPath = ossimFilename(elevationpath);
	prj.m_MapProjection = alosUti.getKeywordlist();
	prj.m_MapPar = alosUti.getMapProjection();
	if(gcpfile.exists())
	{
		prj.ReadGcpAndProjection(ossimFilename(gcpfile));
		prj.GetElevations(prj.m_CtrlGptSet);
	}
	else
	{
		prj.m_CtrlGptSet = new ossimTieGptSet;
	}

	prj.m_ImgFileNameUnc = ossimFilename(alosUti.m_FileTIF);
	prj.m_OutBandList.clear();
	prj.m_OutBandList.push_back(2);
	prj.m_OutBandList.push_back(1);
	prj.m_OutBandList.push_back(0);

	prj.m_ModelType = RPCType;
	if(!prj.InitiateSensorModel(prj.m_ImgFileNameUnc))
	{
		//cout<<"warning: δ�ҵ��ʺ�\""<<prj.m_ImgFileNameUnc<<"\"��ģ�ͣ�"<<endl;
		//ossimString strState = "warning: δ�ҵ��ʺ�\"" + prj.m_ImgFileNameUnc + "\"��ģ�ͣ�";
		ossimString strState = "ʧ��";
		time_t tm;
		tm = time(NULL);
		char tmp[64];
		strftime(tmp,sizeof(tmp),"%Y/%m/%d %X",localtime(&tm));
		ossimString strContent = strCurrentTask + "\t" + alosUti.getSceneID() + "\tAVNIR2\t" + tmp + "\t" + outfile + "\t" + ossimString(strState) + "\t����У��";
		addLogLine(logFile, strContent);
		return false;
	}
	ossimRpcModel *rpcModel = new ossimRpcModel;
	rpcModel->setAttributes(alosUti.getRpcModelStruct());
	prj.m_sensorModel = rpcModel;
	prj.m_sensorModel->m_proj = prj.m_MapPar;
	prj.UpdateSensorModel(*prj.m_CtrlGptSet, prj.m_sensorModel, prj.geom);
	if(bReport)
		prj.OutputReport(reportfile, prj.m_sensorModel, prj.m_CtrlGptSet, prj.m_ChkGptSet);

	//cout<<prj.geom;

	if(!prj.Orthograph(outfile))
	{
		//ossimString strState = "warning: ����ͼ��\"" + outfile + "\"ʧ�ܣ�";
		ossimString strState = "ʧ��";
		time_t tm;
		tm = time(NULL);
		char tmp[64];
		strftime(tmp,sizeof(tmp),"%Y/%m/%d %X",localtime(&tm));
		ossimString strContent = strCurrentTask + "\t" + alosUti.getSceneID() + "\tPRISM\t" + tmp + "\t" + outfile + "\t" + strState + "\t����У��";
		addLogLine(logFile, strContent);
		return false;

	}
	ossimString strState = "�ɹ�";
	time_t tm;
	tm = time(NULL);
	char tmp[64];
	strftime(tmp,sizeof(tmp),"%Y/%m/%d %X",localtime(&tm));
	ossimString strContent = strCurrentTask + "\t" + alosUti.getSceneID() + "\tAVNIR2\t" + tmp + "\t" + outfile + "\t" + ossimString(strState) + "\t����У��";
	addLogLine(logFile, strContent);

	return true;
}

/*
* addLogLine �ں������һ����־��
* @Param
* wxString strLogFile: ��־�ļ���
* wxString strContet: ��ӵ�����
*/
bool AlosBatch::addLogLine(ossimFilename strLogFile, ossimFilename strContet)
{
	//wxTextFile logfile;
	//logfile.Create(strLogFile);
	//if( logfile.Open(strLogFile))
	//{
	//	logfile.AddLine( strContet );

	//	logfile.Write(wxTextFileType_None);
	//	logfile.Close();
	//	return true;
	//}
	//logfile.Write(wxTextFileType_None);
	//logfile.Close();
	return false;
}

/*
* setLogLine ����ĳ����־�е�����
* @Param
* wxString strLogFile: ��־�ļ���
* int nLine: �޸ĵ��кţ�0��ʾ��һ�У�
* wxString strContet: �޸ĵ�����
*/
bool AlosBatch::setLogLine(ossimFilename strLogFile, int nLine, ossimFilename strContet)
{

	//wxTextFile logfile;
	//logfile.Create(strLogFile);
	//if( logfile.Open(strLogFile))
	//{
	//	logfile.GetLine(nLine) = strContet;
	//	logfile.Write(wxTextFileType_None);
	//	logfile.Close();
	//	return true;
	//}
	//logfile.Write(wxTextFileType_None);
	//logfile.Close();
	return false;
}