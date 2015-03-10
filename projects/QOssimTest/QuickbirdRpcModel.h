#pragma once
#include <ossim/projection/ossimQuickbirdRpcModel.h>
#include "RpcModel.h"

class QuickbirdRpcModel :
	public RpcModel
{
public:
	QuickbirdRpcModel(void);
	QuickbirdRpcModel(const ossimFilename& dataDir);
	~QuickbirdRpcModel();

	ossimString		m_FileHDR;	//ͷ�ļ�
	ossimString		m_FileTIF;	//TIF�ļ�
	ossimString		m_FileRPC;	//RPC�ļ�

	ossimRpcModel::rpcModelStruct	m_RpcStruct;

	virtual int executeOrth(ossimFilename outFile, ossimFilename elevationPath = "");

protected:
	virtual bool init(const ossimFilename& dataDir);
	virtual bool readRPCFile(ossimFilename rpcFile, ossimRpcModel::rpcModelStruct& rpcStruct);
	virtual bool readHeader(ossimFilename HDRfile);
	virtual ossimFilename GetOrthFileName(ossimFilename outPath){return "";};
	virtual ossimFilename GetGcpFileName(){return "";};
	virtual ossimFilename GetReportFileName(ossimFilename outPath){return "";};
	vector<ossimFilename> GetGcpFileNames();
	void SetOutputBands();
};