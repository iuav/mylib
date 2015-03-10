#pragma once
#ifndef strUtil_H
#define strUtil_H

#include <string>
#include <vector>
#include <iostream>
using namespace std;

static int SplitString(const string& input, 
				const string& delimiter, vector<string>& results, 
				bool includeEmpties)
{
	int iPos = 0;
	int newPos = -1;
	int sizeS2 = (int)delimiter.size();
	int isize = (int)input.size();

	if( 
		( isize == 0 )
		||
		( sizeS2 == 0 )
		)
	{
		return 0;
	}

	vector<int> positions;

	newPos = input.find (delimiter, 0);

	if( newPos < 0 )
	{ 
		return 0; 
	}

	int numFound = 0;

	while( newPos >= iPos )
	{
		numFound++;
		positions.push_back(newPos);
		iPos = newPos;
		newPos = input.find (delimiter, iPos+sizeS2);
	}

	if( numFound == 0 )
	{
		return 0;
	}

	for( int i=0; i <= (int)positions.size(); ++i )
	{
		string s("");
		if( i == 0 ) 
		{ 
			s = input.substr( i, positions[i] ); 
			if( includeEmpties || ( s.size() > 0 ) )
			{
				results.push_back(s);
			}
			continue;
		}
		int offset = positions[i-1] + sizeS2;
		if( offset < isize )
		{
			if( i == positions.size() )
			{
				s = input.substr(offset);
			}
			else if( i > 0 )
			{
				s = input.substr( positions[i-1] + sizeS2, 
					positions[i] - positions[i-1] - sizeS2 );
			}
		}
		if( includeEmpties || ( s.size() > 0 ) )
		{
			results.push_back(s);
		}
	}
	return numFound;
}


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
static int findDelimiter(const string& str, const vector<string>& delimiterList, int* indexOfDelimiter = NULL)
{
	// ��ȡ�ָ����ĸ���
	int num = static_cast<int>(delimiterList.size());
	int iPos = -1;	//����һ���α�
	int index_ = 0;
	for(int i = 0;i < num;i++)
	{
		//���δβ��Ҹ��ָ���
		int tmp;
		if((tmp = str.find(delimiterList[i])) != -1)
		{
			//����ҵ�ĳ�ָ���
			if(-1 == iPos || tmp < iPos)
			{
				index_ = i;
				iPos = tmp;
			}
		}
	}

	if (indexOfDelimiter)
	{
		*indexOfDelimiter = index_;
	}
	//���ص�һ���ָ�����λ�ã������û���ҵ��κ�һ���ָ������򷵻�-1
	return iPos;
}

/************************************************************************/
/* 
 ˵�����ö���ָ������ַ���
 ������
const string& input ������ַ���
const vector<string>& delimiterList Ϊ�ָ����б�
 �����vector<string>& results����Ų�ֽ��
*/
/************************************************************************/
static void splitString(const string& input, 
	const vector<string>& delimiterList, vector<string>& results)
{
	results.clear();

	int iPos=-1;   //����һ���α�
	int tmpPos = -1;

	string str = input;
	//ɾ���ַ����׵ķָ���
	for (int i = 0;i < (int)delimiterList.size();++i)
	{
		string delimiter = delimiterList[i];
		int pos;
		while((pos = str.find (delimiter, 0)) == 0)
		{
			str = str.substr( pos+delimiter.size(), str.size() ); 
		}
	}

	while((iPos=findDelimiter(str, delimiterList))!=-1) //�ҵ�һ��delimiter������ʱ��0��ʼ��
	{
		results.push_back(str.substr(0, iPos));//��ȡһ��Ԫ�أ�����������

		//ɾ����Ԫ��
		//str.erase(0,iPos+1);
		str = str.substr(iPos+1, str.size());

		//ɾ������ķָ���
		int indexOfDelimiter;
		while(findDelimiter(str, delimiterList, &indexOfDelimiter) == 0)
		{
			// ����ַ����״��ڷָ���
			// ��ɾ���÷ָ���
			str = str.substr(delimiterList[indexOfDelimiter].size(),  str.size());
		}
	}
	if(str != "")
	{
		// �������һ��Ԫ�ز�Ϊ��
		// ������һ��Ԫ�ؼ�������
		results.push_back(str);
	}
}

static string SBeforeLast(const string& str, char ch)
{
	int pos = str.find_last_of(ch);
	return str.substr(0, pos);
}

static string SAfterFirst(const string& str, char ch)
{
	int pos = str.find_first_of(ch);
	return str.substr(pos + 1, str.size() - 1);
}

static string SAfterLast(const string& str, char ch)
{
	int pos = str.find_last_of(ch);
	return str.substr(pos + 1, str.size() - 1);
}

static string SBeforeFirst(const string& str, char ch)
{
	int pos = str.find_first_of(ch);
	return str.substr(0, pos);
}

#endif