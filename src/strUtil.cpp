#include "strUtil.h"

namespace mylib{


	int SplitString(const wstring& input,
		const wstring& delimiter, vector<wstring>& results,
		bool includeEmpties)
	{
		int iPos = 0;
		int newPos = -1;
		int sizeS2 = (int)delimiter.size();
		int isize = (int)input.size();

		if (
			(isize == 0)
			||
			(sizeS2 == 0)
			)
		{
			return 0;
		}

		vector<int> positions;

		newPos = (int)input.find(delimiter, 0);

		if (newPos < 0)
		{
			return 0;
		}

		int numFound = 0;

		while (newPos >= iPos)
		{
			numFound++;
			positions.push_back(newPos);
			iPos = newPos;
			newPos = (int)input.find(delimiter, iPos + sizeS2);
		}

		if (numFound == 0)
		{
			return 0;
		}

		for (int i = 0; i <= (int)positions.size(); ++i)
		{
			wstring s(L"");
			if (i == 0)
			{
				s = input.substr(i, positions[i]);
				if (includeEmpties || (s.size() > 0))
				{
					results.push_back(s);
				}
				continue;
			}
			int offset = positions[i - 1] + sizeS2;
			if (offset < isize)
			{
				if (i == positions.size())
				{
					s = input.substr(offset);
				}
				else if (i > 0)
				{
					s = input.substr(positions[i - 1] + sizeS2,
						positions[i] - positions[i - 1] - sizeS2);
				}
			}
			if (includeEmpties || (s.size() > 0))
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
	const wstring& str �����ҵ��ַ���
	const vector<wstring>& delimiterListΪ�ָ����б�
	int* indexOfDelimiter Ϊ��һ�γ��ֵķָ������
	����ֵ��
	��һ�γ��ַָ�����λ�ã����û���ҵ��κ�һ���ָ������򷵻�-1
	*/
	/************************************************************************/
	int findDelimiter(const wstring& str, const vector<wstring>& delimiterList, int* indexOfDelimiter/* = NULL*/)
	{
		// ��ȡ�ָ����ĸ���
		int num = static_cast<int>(delimiterList.size());
		int iPos = -1;	//����һ���α�
		int index_ = 0;
		for (int i = 0; i < num; i++)
		{
			//���δβ��Ҹ��ָ���
			int tmp;
			if ((tmp = (int)str.find(delimiterList[i])) != -1)
			{
				//����ҵ�ĳ�ָ���
				if (-1 == iPos || tmp < iPos)
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
	const wstring& input ������ַ���
	const vector<wstring>& delimiterList Ϊ�ָ����б�
	�����vector<wstring>& results����Ų�ֽ��
	*/
	/************************************************************************/
	void splitString(const wstring& input,
		const vector<wstring>& delimiterList, vector<wstring>& results)
	{
		results.clear();

		int iPos = -1;   //����һ���α�
		int tmpPos = -1;

		wstring str = input;
		//ɾ���ַ����׵ķָ���
		for (int i = 0; i < (int)delimiterList.size(); ++i)
		{
			wstring delimiter = delimiterList[i];
			int pos;
			while ((pos = (int)str.find(delimiter, 0)) == 0)
			{
				str = str.substr(pos + delimiter.size(), str.size());
			}
		}

		while ((iPos = findDelimiter(str, delimiterList)) != -1) //�ҵ�һ��delimiter������ʱ��0��ʼ��
		{
			results.push_back(str.substr(0, iPos));//��ȡһ��Ԫ�أ�����������

			//ɾ����Ԫ��
			//str.erase(0,iPos+1);
			str = str.substr(iPos + 1, str.size());

			//ɾ������ķָ���
			int indexOfDelimiter;
			while (findDelimiter(str, delimiterList, &indexOfDelimiter) == 0)
			{
				// ����ַ����״��ڷָ���
				// ��ɾ���÷ָ���
				str = str.substr(delimiterList[indexOfDelimiter].size(), str.size());
			}
		}
		if (str != L"")
		{
			// �������һ��Ԫ�ز�Ϊ��
			// ������һ��Ԫ�ؼ�������
			results.push_back(str);
		}
	}

int SplitString(const string& input, 
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

	newPos = (int)input.find (delimiter, 0);

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
		newPos = (int)input.find (delimiter, iPos+sizeS2);
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
int findDelimiter(const string& str, const vector<string>& delimiterList, int* indexOfDelimiter/* = NULL*/)
{
	// ��ȡ�ָ����ĸ���
	int num = static_cast<int>(delimiterList.size());
	int iPos = -1;	//����һ���α�
	int index_ = 0;
	for(int i = 0;i < num;i++)
	{
		//���δβ��Ҹ��ָ���
		int tmp;
		if((tmp = (int)str.find(delimiterList[i])) != -1)
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
void splitString(const string& input, 
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
		while((pos = (int)str.find (delimiter, 0)) == 0)
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

string SBeforeLast(const string& str, char ch)
{
	size_t pos = str.find_last_of(ch);
	return str.substr(0, pos);
}

string SAfterFirst(const string& str, char ch)
{
	size_t pos = str.find_first_of(ch);
	return str.substr(pos + 1, str.size() - 1);
}

string SAfterLast(const string& str, char ch)
{
	size_t pos = str.find_last_of(ch);
	return str.substr(pos + 1, str.size() - 1);
}

string SBeforeFirst(const string& str, char ch)
{
	size_t pos = str.find_first_of(ch);
	return str.substr(0, pos);
}


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
int findChar(ossimString str, vector<char> chList)
{
	// ��ȡ�ָ����ĸ���
	int num = static_cast<int>(chList.size());
	int iPos = -1;	//����һ���α�
	for(int i = 0;i < num;i++)
	{
		//���δβ��Ҹ��ָ���
		int tmp;
		if((tmp = (int)str.find(chList[i])) != -1)
		{
			//����ҵ�ĳ�ָ���
			if(-1 == iPos || tmp < iPos)
			{
				iPos = tmp;
			}
		}
	}

	//���ص�һ���ָ�����λ�ã������û���ҵ��κ�һ���ָ������򷵻�-1
	return iPos;
}


/************************************************************************/
/* 
 ˵�����ö���ָ������ַ���
 ������
 ossimString str ������ַ���
 vector<char> chListΪ�ָ����б�
 �����vector<ossimString>& strArray����Ų�ֽ��
*/
/************************************************************************/
void splitString(ossimString str, vector<char> chList, vector<ossimString>& strArray)
{
	strArray.clear();

	int iPos=-1;   //����һ���α�
	int tmpPos = -1;

	//ɾ���ַ����׵ķָ���
	while((tmpPos=findChar(str, chList))==0)
	{
		str.erase(0, 1);
	}
	while((iPos=findChar(str, chList))!=-1) //�ҵ�һ��ch������ʱ��0��ʼ��
	{
		strArray.push_back(str.beforePos(iPos));//��ȡһ��Ԫ�أ�����������

		//ɾ����Ԫ��
		str.erase(0,iPos+1);

		//ɾ������ķָ���
		while(findChar(str, chList)==0)
		{
			// ����ַ����״��ڷָ���
			// ��ɾ���÷ָ���
			str.erase(0, 1);
		}
	}
	if(str != "")
	{
		// �������һ��Ԫ�ز�Ϊ��
		// ������һ��Ԫ�ؼ�������
		strArray.push_back(str);
	}
}


ossimString ossimSBeforeLast(const ossimString& str, char ch)
{
	size_t pos = str.find_last_of(ch);
	return str.substr(0, pos);
}

ossimString ossimSAfterFirst(const ossimString& str, char ch)
{
	size_t pos = str.find_first_of(ch);
	return str.substr(pos + 1, str.size() - 1);
}

ossimString ossimSAfterLast(const ossimString& str, char ch)
{
	size_t pos = str.find_last_of(ch);
	return str.substr(pos + 1, str.size() - 1);
}

ossimString ossimSBeforeFirst(const ossimString& str, char ch)
{
	size_t pos = str.find_first_of(ch);
	return str.substr(0, pos);
}


#ifdef USE_QT
/************************************************************************/
/* QString                                                              */
/************************************************************************/
QString QBeforeLast(const QString& str, QChar ch)
{
	QStringList fields = str.split(ch);
	fields.removeLast();
	return fields.join(ch);
}

QString QAfterFirst(const QString& str, QChar ch)
{
	QStringList fields = str.split(ch);
	fields.removeFirst();
	return fields.join(ch);
}

QString QAfterLast(const QString& str, QChar ch)
{
	QStringList fields = str.split(ch);
	return fields.takeLast();
}

QString QBeforeFirst(const QString& str, QChar ch)
{
	QStringList fields = str.split(ch);
	return fields.takeFirst();
}
#endif

std::string ConvertFromUtf16ToUtf8(const std::wstring& wstr)
{
	std::string convertedString;
	int requiredSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, 0, 0, 0, 0);
	if (requiredSize > 0)
	{
		std::vector<char> buffer(requiredSize);
		WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &buffer[0], requiredSize, 0, 0);
		convertedString.assign(buffer.begin(), buffer.end() - 1);
	}
	return convertedString;
}

std::wstring ConvertFromUtf8ToUtf16(const std::string& str)
{
	std::wstring convertedString;
	int requiredSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, 0, 0);
	if (requiredSize > 0)
	{
		std::vector<wchar_t> buffer(requiredSize);
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &buffer[0], requiredSize);
		convertedString.assign(buffer.begin(), buffer.end() - 1);
	}

	return convertedString;
}

std::string TCHAR2STRING(TCHAR *STR)
{

	int iLen = WideCharToMultiByte(CP_ACP, 0, STR, -1, NULL, 0, NULL, NULL);

	char* chRtn = new char[iLen*sizeof(char)];

	WideCharToMultiByte(CP_ACP, 0, STR, -1, chRtn, iLen, NULL, NULL);

	std::string str(chRtn);

	return str;
}

char* TCHAR2CHAR(TCHAR *STR)
{

	int iLen = WideCharToMultiByte(CP_ACP, 0, STR, -1, NULL, 0, NULL, NULL);

	char* chRtn = new char[iLen*sizeof(char)];

	WideCharToMultiByte(CP_ACP, 0, STR, -1, chRtn, iLen, NULL, NULL);

	return chRtn;
}

TCHAR* CHAR2TCHAR(CHAR *STR)
{
	int iLen = MultiByteToWideChar(CP_UTF8, 0, STR, -1, 0, 0);

	TCHAR *tchRtn = new TCHAR(iLen);
	MultiByteToWideChar(CP_UTF8, 0, STR, -1, &tchRtn[0], iLen);
	return tchRtn;
}

char* WCHAR2CHAR(wchar_t *STR)
{

	int iLen = WideCharToMultiByte(CP_ACP, 0, STR, -1, NULL, 0, NULL, NULL);

	char* chRtn = new char[iLen*sizeof(char)];

	WideCharToMultiByte(CP_ACP, 0, STR, -1, chRtn, iLen, NULL, NULL);

	return chRtn;
}


const wchar_t* CHAR2WCHAR(const char *STR)
{
	////int n = sizeof(STR);
	//int n = strlen(STR);
	//char *tmp = new char[n+1];
	//memcpy(tmp, STR, n);
	//tmp[n] = '\0';
	//int iLen = MultiByteToWideChar(CP_UTF8, 0, tmp, -1, 0, 0);

	//wchar_t *tchRtn = new wchar_t(iLen);
	//MultiByteToWideChar(CP_UTF8, 0, tmp, -1, tchRtn, iLen);
	//delete[]tmp;
	//return tchRtn;

	//int n = strlen(STR);
	//wchar_t *buf = new wchar_t[n];
	//size_t num_chars = mbstowcs(buf, STR, n);
	//wstring ws(buf, num_chars);
	//delete[] buf;
	//return ws.c_str();

	//std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	////std::string narrow = converter.to_bytes(wide_utf16_source_string);
	//std::wstring wide = converter.from_bytes(string(STR));
	//const wchar_t * s = wide.c_str();
	//return wide.c_str();

	//const size_t cSize = strlen(STR) + 1;
	//wchar_t* wc = new wchar_t[cSize];
	//mbstowcs(wc, STR, cSize);

	//return wc;

	const wchar_t *pwcsName;
	// required size
	int nChars = MultiByteToWideChar(CP_ACP, 0, STR, -1, NULL, 0);
	// allocate it
	pwcsName = new wchar_t[nChars];
	MultiByteToWideChar(CP_ACP, 0, STR, -1, (LPWSTR)pwcsName, nChars);
	// use it....

	//// delete it
	//delete[] pwcsName;
	return pwcsName;
}
} // end of namespace mylib