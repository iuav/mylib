#include "FileAndDirFinder.h"
#include "util.h"
#include <stdio.h>


CFileAndDirFinder::CFileAndDirFinder(void)
{
}


CFileAndDirFinder::~CFileAndDirFinder(void)
{
}


void CFileAndDirFinder::FindAllDir( const char* pCurDir, vector<string>& vtDirs )
{
	// ��ǰĿ¼
	char szDir[MAX_PATH] = {0};
	
	sprintf_s(szDir, MAX_PATH, "%s\\*.*", pCurDir);


	WIN32_FIND_DATA findFileData = {0};
	HANDLE hFind = FindFirstFile(char2TCHAR(szDir), &findFileData);


	if (INVALID_HANDLE_VALUE == hFind)
	{
		return ;
	}


	do
	{
		/* ���ص��ļ����л����"."��".."����.'����Ŀ¼��".."������һ��Ŀ¼��
		һ���������Ҫ�����������ƹ��˵�������Ҫ�����ļ�ɾ������
		*/
		if (findFileData.cFileName[0] != '.')//���ǵ�ǰ·�����߸�Ŀ¼�Ŀ�ݷ�ʽ
		{
			if(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// ����һ����ͨĿ¼
				char tmpDir[MAX_PATH] = {0};
				sprintf_s(tmpDir, MAX_PATH, "%s\\%s", pCurDir, TCHAR2char(findFileData.cFileName));
				vtDirs.push_back(tmpDir);


				// �ݹ���ò�����Ŀ¼
				FindAllDir(tmpDir, vtDirs);
			}
		}
	}while (FindNextFile(hFind, &findFileData));


	FindClose(hFind);
	hFind = INVALID_HANDLE_VALUE;
}


void CFileAndDirFinder::FindAllFile( const char* pCurDir, const char* pFilter, vector<string>& vtFiles )
{
	// ��ǰĿ¼
	char szDir[MAX_PATH] = {0};
	sprintf_s(szDir, MAX_PATH, "%s\\%s", pCurDir, pFilter);


	WIN32_FIND_DATA findFileData = {0};
	HANDLE hFind = FindFirstFile(char2TCHAR(szDir), &findFileData);


	if (INVALID_HANDLE_VALUE == hFind)
	{
		return ;
	}


	do
	{
		/* ���ص��ļ����л����"."��".."����.'����Ŀ¼��".."������һ��Ŀ¼��
		һ���������Ҫ�����������ƹ��˵�������Ҫ�����ļ�ɾ������
		*/
		if (findFileData.cFileName[0] != '.')//���ǵ�ǰ·�����߸�Ŀ¼�Ŀ�ݷ�ʽ
		{
			// ����һ���ļ�
			char tmpFile[MAX_PATH] = {0};
			sprintf_s(tmpFile, MAX_PATH, "%s\\%s", pCurDir, TCHAR2char(findFileData.cFileName));


			vtFiles.push_back(tmpFile);
		}
	}while (FindNextFile(hFind, &findFileData));


	FindClose(hFind);
	hFind = INVALID_HANDLE_VALUE;
}

void CFileAndDirFinder::FindAllFileHere(const char* pFilter, vector<string>& vtFiles )
{
	// ��ǰĿ¼
	WIN32_FIND_DATA findFileData = {0};
	HANDLE hFind = FindFirstFile(char2TCHAR(pFilter), &findFileData);


	if (INVALID_HANDLE_VALUE == hFind)
	{
		return ;
	}


	do
	{
		/* ���ص��ļ����л����"."��".."����.'����Ŀ¼��".."������һ��Ŀ¼��
		һ���������Ҫ�����������ƹ��˵�������Ҫ�����ļ�ɾ������
		*/
		if (findFileData.cFileName[0] != '.')//���ǵ�ǰ·�����߸�Ŀ¼�Ŀ�ݷ�ʽ
		{
			// ����һ���ļ�
			char tmpFile[MAX_PATH] = {0};
			sprintf_s(tmpFile, MAX_PATH, "%s", TCHAR2char(findFileData.cFileName));


			vtFiles.push_back(tmpFile);
		}
	}while (FindNextFile(hFind, &findFileData));


	FindClose(hFind);
	hFind = INVALID_HANDLE_VALUE;
}


void CFileAndDirFinder::FindAllFileE( const char* pCurDir, const char* pFilter, vector<string>& vtFiles )
{
	// ��ȡ��Ŀ¼
	vector<string> vtDirs;
	FindAllDir(pCurDir, vtDirs);


	// ���뵱ǰĿ¼
	vtDirs.push_back(pCurDir);


	vector<string>::iterator itDir = vtDirs.begin();
	for (; itDir!=vtDirs.end(); ++itDir)
	{
		FindAllFile(itDir->c_str(), pFilter, vtFiles);
	}
}