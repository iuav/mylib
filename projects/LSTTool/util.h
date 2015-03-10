#pragma once
// ʹ��UNICODE�ַ���
#ifndef UNICODE
#define UNICODE
#endif
// �����ʹ��Unicode���Խ�������ģ�
//#undef UNICODE
//#undef _UNICODE
#include <windows.h>
#include <string>
#include <vector>
// �汾����
// ��Ϊ�Ҳ�֪��VC7.1��ǰ�İ汾�Ƿ���tchar.h
#if _MSC_VER < 1310
#error "ע�⣬����ʹ��VC7.1�������ϰ汾����˳���"
#endif
#include <tchar.h>
#pragma message("��ע�⣺��������ļ��������������ļ������ļ��Ļ������ʹ��UNICODE�ַ���")
// �ṩ�ַ����Ķ��������ĵ�ʱ�����ʹ��wstring
#ifdef UNICODE
typedef std::wstring String;
#else
typedef std::string String;
#endif

/* 
*********************************************************************** 
* ������ TCHAR2Char 
* ��������TCHAR* ת��Ϊ char* 
* ���ڣ�
*********************************************************************** 
*/ 
static char* TCHAR2char(TCHAR* tchStr) 
{ 
	int iLen = 2*wcslen(tchStr);//CString,TCHAR������һ���ַ�����˲�����ͨ���㳤�� 
	char* chRtn = new char[iLen+1];
	size_t converted = 0;
	wcstombs_s(&converted, chRtn, iLen+1, tchStr, _TRUNCATE);//ת���ɹ�����Ϊ�Ǹ�ֵ 
	return chRtn; 
} 

static char* TCHAR2char(const TCHAR* tchStr) 
{ 
	int iLen = 2*wcslen(tchStr);//CString,TCHAR������һ���ַ�����˲�����ͨ���㳤�� 
	char* chRtn = new char[iLen+1];
	size_t converted = 0;
	wcstombs_s(&converted, chRtn, iLen+1, tchStr, _TRUNCATE);//ת���ɹ�����Ϊ�Ǹ�ֵ 
	return chRtn; 
} 

/*
*********************************************************************** 
* ������ char2tchar
* �������� char* ת��Ϊ TCHAR*
* ���ڣ�
*********************************************************************** 
*/ 
static TCHAR *char2TCHAR(char *str)
{
	int iLen = strlen(str);
	TCHAR *chRtn = new TCHAR[iLen+1];
	//mbstowcs(chRtn, str, iLen+1); return chRtn;
	size_t converted = 0;
	mbstowcs_s(&converted, chRtn, iLen+1, str, _TRUNCATE);//ת���ɹ�����Ϊ�Ǹ�ֵ
	return chRtn;
}

static TCHAR *char2TCHAR(const char *str)
{
	int iLen = strlen(str);
	TCHAR *chRtn = new TCHAR[iLen+1];
	//mbstowcs(chRtn, str, iLen+1); return chRtn;
	size_t converted = 0;
	mbstowcs_s(&converted, chRtn, iLen+1, str, _TRUNCATE);//ת���ɹ�����Ϊ�Ǹ�ֵ
	return chRtn;
}


static LPWSTR GetAbsolutePathName(const char *relativePathName)
{	
	TCHAR *fullPath = new TCHAR[MAX_PATH];
	TCHAR *tchar_ = char2TCHAR(relativePathName);
	GetFullPathName(tchar_, MAX_PATH, fullPath, NULL);
	return fullPath;
}

static LPWSTR GetAbsolutePathName(char *relativePathName)
{	
	TCHAR *fullPath = new TCHAR[MAX_PATH];
	TCHAR *tchar_ = char2TCHAR(relativePathName);
	GetFullPathName(tchar_, MAX_PATH, fullPath, NULL);
	return fullPath;
}

static LPWSTR GetAbsolutePathName(TCHAR *relativePathName)
{	
	TCHAR *fullPath = new TCHAR[MAX_PATH];
	TCHAR *tchar_ = NULL;
	GetFullPathName(relativePathName, MAX_PATH, fullPath, NULL);
	return fullPath;
}

static LPWSTR GetAbsolutePathName(const TCHAR *relativePathName)
{	
	TCHAR *fullPath = new TCHAR[MAX_PATH];
	TCHAR *tchar_ = NULL;
	GetFullPathName(relativePathName, MAX_PATH, fullPath, NULL);
	return fullPath;
}

// �����ļ����Ķ���
typedef std::vector<LPWSTR> FilesVec;
// ���ҵ�ǰĿ¼�µ������ļ����������ļ����ļ�������FilesVec�����õķ�ʽ����
// pathName Ϊ·����
static void FindFiles( const TCHAR * pszFilter ,FilesVec &files )
{
   WIN32_FIND_DATA FindFileData;
   HANDLE hFind = INVALID_HANDLE_VALUE;
   //TCHAR PathBuffer[ _MAX_PATH ];
  
   //_tcscpy_s( PathBuffer,_MAX_PATH, pathName );
   //_tcscat_s( PathBuffer,_MAX_PATH, _T("\\*") ); // ��Ҳ��֪��ΪʲôҪ��ӣ�����MSDN����ô���ģ���ֻ����������
   hFind = ::FindFirstFile( pszFilter ,&FindFileData );
   if( INVALID_HANDLE_VALUE == hFind )      // ���������ĳ���쳣��ֱ���׳����
   {
      //char buffer[56];
      //sprintf_s( buffer,"Invalid File Handle.Error is %u\n",GetLastError() );
      //throw std::exception( buffer );
	   // û���ҵ�
	   return;
   }
   else // Ȼ���ٽ��Ų�����ȥ
   {
      files.push_back( GetAbsolutePathName(FindFileData.cFileName) );  
     
      while( 0 != FindNextFile( hFind,&FindFileData ) )
      {
		  LPWSTR fullPath = GetAbsolutePathName(FindFileData.cFileName);
         files.push_back( fullPath );
      }
      FindClose( hFind );
   }
}