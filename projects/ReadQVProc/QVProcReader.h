#ifndef QVProcReader_H
#define QVProcReader_H

#include <fstream>
#include <iostream>
#include <vector>
#include <string>

#include "pugixml\pugiconfig.hpp"
#include "pugixml\pugixml.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
using namespace std;

static void posEci2Ecr(double& x, double &y, double &z, double t)
{
	//��=7292115��10-11rads-1��0.150��10-11rads-1
	double omega = 7.292115e-5;
	double a = t * omega;
	double sa = sin(a);
	double ca = cos(a);

	double xx = ca *x + sa * y;
	double yy = -sa * x + ca * y;

	x = xx;
	y = yy;
}

static void velEci2Ecr(double x, double y, double z,
	double &vx, double &vy, double &vz, double t)
{
	//��=7292115��10-11rads-1��0.150��10-11rads-1
	double omega = 7.292115e-5;
	double a = t * omega;
	double sa = sin(a);
	double ca = cos(a);

	double vxx = ca *vx + sa * vy;
	double vyy = -sa * vx + ca * vy;

	double xx = -omega*sa*x + omega*ca * y;
	double yy = -omega*ca * x - omega*sa * y;

	vx = vxx + xx;
	vy = vyy + yy;
}

static void ecef2lla(double x, double y, double z,
	double& lat, double& lon, double& alt)
{
	double b, ep, p, th, n;

	/* Constants (WGS ellipsoid) */
	const double a = 6378137;
	const double e = 8.1819190842622e-2;
	const double pi = 3.141592653589793;

	/* Calculation */
	b = sqrt(pow(a, 2)*(1 - pow(e, 2)));
	ep = sqrt((pow(a, 2) - pow(b, 2)) / pow(b, 2));
	p = sqrt(pow(x, 2) + pow(y, 2));
	th = atan2(a*z, b*p);
	lon = atan2(y, x);
	lat = atan2((z + ep*ep*b*pow(sin(th), 3)), (p - e*e*a*pow(cos(th), 3)));
	n = a / sqrt(1 - e*e*pow(sin(lat), 2));
	alt = p / cos(lat) - n;
	lat = (lat * 180) / pi;
	lon = (lon * 180) / pi;
}

class QVProcReader
{
public:
	QVProcReader();

	typedef struct _tagBL
	{
		float		longitude;
		float		latitude;
	}LONLAT_POS;

	typedef struct _tagTimeInfo
	{
		int			year;
		int			mon;
		int			day;
		int			hour;
		int			min;
		int			sec;		// ��
		int			nsec;
		int			millsec;	// ����
		int			microsec;	// ΢��
	}_TIMESTAMP;

	struct SAT_POS
	{
		float x;
		float y;
		float z;
		float vx;
		float vy;
		float vz;
	};

	struct SAT_ATT
	{
		float roll;		//q1
		float pitch;	//q2
		float yaw;		//q3
		float vroll;
		float vpitch;
		float vyaw;
	};

	typedef struct 
	{
		char  station_id[16];		//	���ո����ݵĵ���վ�ı�ʶ
		char	 satellite_id[16];	    //	��Ӧ�����Ǳ�ʶ
		char   sensor_id[16];		//	��Ӧ�Ĵ�������ʶ
		char	 work_mode[16];		//	��Ӧ�Ĺ���ģʽ��Ϣ
		char	 JobTaskID [20];	    //	�����ʶ
		int	 orbit_num;   		//  �����
		int	 channel_num;	 	//  ͨ����
		int	 totalfiles;	 	    //  �����񵥴�������ļ�����
		int	 files_num;	 	    //  ���δ�����ļ������������е��ļ����
		int	 desample_num;	    //	���п�������ʱ�Ľ�������,��һ���Ľ�������
		int	 data_width;			//	��һ���������Ŀ��,(����������������Ļ�),������һ����С
		int	 sample_bit_count;	//	���Ӵ�����ͼ��������������ֻ��8Bit��16Bit�������
		int	 gray_image_flag;	//	����ֵΪ��ʱ����ͼ��Ϊ��ͨ���Ҷ�ͼ�񣬷���Ϊ��ͨ��α��ɫͼ��
		_TIMESTAMP		start_time;	//	���ݽ��յĿ�ʼʱ��
		_TIMESTAMP		end_time;		//	���ݽ��յĽ���ʱ��
		LONLAT_POS		station_pos;	//	��Ӧ�ĵ���վ�ڵ����ϵ�λ����Ϣ	����վ��λ��Ϣ

		int  sample_num;
		int  line_num;
		int  band_num;
	} QUIVIMAGE_HEAD_INFO;

	typedef struct _tagQuivAuxInfo
	{
		int		   valid_flag;		//	��ֵΪ=1ʱ��Ч����������¸�����ϢΪ��Чֵ
		LONLAT_POS		nadir_pos;	//	��Ӧ�����µ��λ��
		LONLAT_POS		line_left_pos;//	�þ����߶�Ӧ������λ�ã����Ⱥ�γ�ȣ�
		LONLAT_POS		line_right_pos;	//	�þ����߶�Ӧ���Ҳ��λ�ã����Ⱥ�γ�ȣ�
		SAT_POS			satpos;     //���ǵ�x,y,z�����Լ�vx,vy,vz
		SAT_ATT			satatt;     //���ǵ���̬����,��������ƫ����r,p,y
		unsigned short line_count;
		_TIMESTAMP		line_time;			//	����������ʱ����Ϣ
		double      line_num;
		double		att_time;			//	��̬���ݵ�ʱ����Ϣ
		double		gps_time;			//	GPS��λ����ʱ��
		double      star_time;			//  ���������ʱ����
		double      gps_c_time;			//  GPS��ʱ��
	} QUIVIMAGE_AUX_INFO;

	virtual bool read(const char* filename, const char* outPath);
	virtual bool read_band_seperate(const char* filename, const char* outPath);
	virtual bool read_by_scene(const char* filename, const char* outPath);
	virtual bool read_by_scene_band_seperate(const char* filename, const char* outPath);

	bool readHeadInfo(ifstream& fs, QUIVIMAGE_HEAD_INFO& header);
	bool readTimeInfo(ifstream& fs, _TIMESTAMP& t);
	bool readSAT_POS(ifstream& fs, SAT_POS& pos);
	bool readSAT_ATT(ifstream& fs, SAT_ATT& att);
	bool readQuivAuxInfo(ifstream& fs, QUIVIMAGE_AUX_INFO& aux);

	int timeCompare(const _TIMESTAMP& t1, const _TIMESTAMP& t2)const;

	virtual void writeDescXML(const QUIVIMAGE_HEAD_INFO& theHeaderInfo, const char* descXML)const;
	virtual void writeDescXML(const QUIVIMAGE_HEAD_INFO& theHeaderInfo, const vector<QUIVIMAGE_AUX_INFO>& auxList, const char* descXML, int line_offset = 0)const;
	
	bool time_ready;

	void BitInverse(char* chrs, int nBit);

	template <class T>
	void BitInverse(T &data)
	{
		int nBit = sizeof(T);
		BitInverse((char*)&data, nBit);
	};

	string time2String(const _TIMESTAMP& t)const;
	void convertTimeStamp(const string& time_stamp,
		double& ti) const;
	double convertTimeStamp(const string& time_stamp) const;

protected:
	//int theTotalLines;
	//int theSamples;
	//int theBands;
	double theReferenceTime;
	double theReferenceLine;
};

#endif