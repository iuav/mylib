#pragma once
#ifndef STR_UTIL_HPP
#define STR_UTIL_HPP

#include <ossim/base/ossimString.h>

#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <wtypes.h>
#include <atltypes.h>

#ifdef USE_QT
#include <QStringList>
#include <QString>
#endif

#include <string>
#include <vector>
#include <iostream>

#include <sstream>
#include <algorithm>
#include <iterator>

#include <locale>
#include <codecvt>
#include <string>

using namespace std;

namespace mylib{
//static wxString ossimString2wxString(const ossimString& rs)
//{
//	return wxString::FromUTF8(rs.c_str());
//}
//
//static ossimString wxString2ossimString(const wxString& ws)
//{
//	return ossimString(ws.mb_str());
//}

/************************************************************************/
/*           string                                                     */
/************************************************************************/
int SplitString(const string& input, 
				const string& delimiter, vector<string>& results, 
				bool includeEmpties);


/************************************************************************/
/* 
 ˵���������ַ����е�һ�γ��ַָ�����λ��
 ������
const string& str �����ҵ��ַ���
const vector<string>& delimiterListΪ�ָ����б�
int* indexOfDelimiter Ϊ��һ�γ��ֵķָ������
 ����ֵ��
 ��һ�γ��ַָ�����λ�ã����û���ҵ��κ�һ���ָ������򷵻�-1
*/
/************************************************************************/
int findDelimiter(const string& str, const vector<string>& delimiterList, int* indexOfDelimiter = NULL);

/************************************************************************/
/* 
 ˵�����ö���ָ������ַ���
 ������
const string& input ������ַ���
const vector<string>& delimiterList Ϊ�ָ����б�
 �����vector<string>& results����Ų�ֽ��
*/
/************************************************************************/
void splitString(const string& input, 
	const vector<string>& delimiterList, vector<string>& results);


/************************************************************************/
/*           wstring                                                     */
/************************************************************************/
int SplitString(const wstring& input,
	const wstring& delimiter, vector<wstring>& results,
	bool includeEmpties);


/************************************************************************/
/*
˵���������ַ����е�һ�γ��ַָ�����λ��
������
const wstring& str �����ҵ��ַ���
const vector<wstring>& delimiterListΪ�ָ����б�
int* indexOfDelimiter Ϊ��һ�γ��ֵķָ������
����ֵ��
��һ�γ��ַָ�����λ�ã����û���ҵ��κ�һ���ָ������򷵻�-1
*/
/************************************************************************/
int findDelimiter(const wstring& str, const vector<wstring>& delimiterList, int* indexOfDelimiter = NULL);

/************************************************************************/
/*
˵�����ö���ָ������ַ���
������
const wstring& input ������ַ���
const vector<wstring>& delimiterList Ϊ�ָ����б�
�����vector<wstring>& results����Ų�ֽ��
*/
/************************************************************************/
void splitString(const wstring& input,
	const vector<wstring>& delimiterList, vector<wstring>& results);

//std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
//	std::stringstream ss(s);
//	std::string item;
//	while (std::getline(ss, item, delim)) {
//		elems.push_back(item);
//	}
//	return elems;
//}
//
//
//std::vector<std::string> split(const std::string &s, char delim) {
//	std::vector<std::string> elems;
//	split(s, delim, elems);
//	return elems;
//}

string SBeforeLast(const string& str, char ch);

string SAfterFirst(const string& str, char ch);

string SAfterLast(const string& str, char ch);

string SBeforeFirst(const string& str, char ch);


/************************************************************************/
/* ossimString                                                           */
/************************************************************************/
/************************************************************************/
/* 
 ˵���������ַ����е�һ�γ��ַָ�����λ��
 ������
 CString str �����ҵ��ַ���
 vector<char> chListΪ�ָ����б�
 ����ֵ��
 ��һ�γ��ַָ�����λ�ã����û���ҵ��κ�һ���ָ������򷵻�-1
*/
/************************************************************************/
int findChar(ossimString str, vector<char> chList);


/************************************************************************/
/* 
 ˵�����ö���ָ������ַ���
 ������
 ossimString str ������ַ���
 vector<char> chListΪ�ָ����б�
 �����vector<ossimString>& strArray����Ų�ֽ��
*/
/************************************************************************/
void splitString(ossimString str, vector<char> chList, vector<ossimString>& strArray);

ossimString ossimSBeforeLast(const ossimString& str, char ch);

ossimString ossimSAfterFirst(const ossimString& str, char ch);

ossimString ossimSAfterLast(const ossimString& str, char ch);

ossimString ossimSBeforeFirst(const ossimString& str, char ch);

#ifdef USE_QT
/************************************************************************/
/* QString                                                              */
/************************************************************************/
QString QBeforeLast(const QString& str, QChar ch);

QString QAfterFirst(const QString& str, QChar ch);

QString QAfterLast(const QString& str, QChar ch);

QString QBeforeFirst(const QString& str, QChar ch);
#endif

std::string ConvertFromUtf16ToUtf8(const std::wstring& wstr);

std::wstring ConvertFromUtf8ToUtf16(const std::string& str);

std::string TCHAR2STRING(TCHAR *STR);

char* TCHAR2CHAR(TCHAR *STR);

TCHAR* CHAR2TCHAR(CHAR *STR);


char* WCHAR2CHAR(wchar_t *STR);
const wchar_t* CHAR2WCHAR(const char *STR);

}; // end of namespace mylib
#endif