#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

class CFileAndDirFinder
{
public:
	CFileAndDirFinder(void);
	~CFileAndDirFinder(void);


	// ���ҵ�ǰĿ¼�µ�����Ŀ¼(��������ǰĿ¼)
	void FindAllDir(const char* pCurDir, vector<string>& vtDirs);
	// ���ҵ�ǰĿ¼�µ������ļ�(��������Ŀ¼)���ƶ������ļ����ͣ��磺*.txt,*.lua,*.*
	void FindAllFile(const char* pCurDir, const char* pFileType, vector<string>& vtFiles);
	// ���ҵ�ǰĿ¼�µ������ļ�(��������Ŀ¼)
	void FindAllFileHere(const char* pFilter, vector<string>& vtFiles );
	// ���ҵ�ǰĿ¼�µ������ļ�(������Ŀ¼)���ƶ������ļ����ͣ��磺*.txt,*.lua,*.*
	void FindAllFileE(const char* pCurDir, const char* pFileType, vector<string>& vtFiles);


};