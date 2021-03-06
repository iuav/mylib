#ifndef HJ1QVProcReader_H
#define HJ1QVProcReader_H

#include "QVProcReader.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
using namespace std;

class HJ1QVProcReader: public QVProcReader
{
public:
	HJ1QVProcReader();

	typedef struct 
	{
		char  station_id[16];		//	接收该数据的地面站的标识
		char	 satellite_id[16];	    //	对应的卫星标识
		char   sensor_id[16];		//	对应的传感器标识
		char	 work_mode[16];		//	对应的工作模式信息
		char	 JobTaskID [20];	    //	任务标识
		int	 orbit_num;   		//  轨道号
		int	 channel_num;	 	//  通道号
		int	 totalfiles;	 	    //  本任务单传输快视文件总数
		int	 files_num;	 	    //  本次传输的文件在所属任务中的文件序号
		int	 desample_num;	    //	进行快数处理时的降采样数,上一步的降采样数
		int	 data_width;			//	上一步传进来的宽度,(如果我在做降采样的话),这里会进一步变小
		int	 sample_bit_count;	//	快视处理后的图像量化比特数，只有8Bit和16Bit两种情况
		int	 gray_image_flag;	//	当此值为真时，该图像为单通道灰度图像，否则为多通道伪彩色图像
		_TIMESTAMP		start_time;	//	数据接收的开始时间
		_TIMESTAMP		end_time;		//	数据接收的结束时间
		LONLAT_POS		station_pos;	//	对应的地面站在地球上的位置信息	地面站的位信息

		int  sample_num;
		int  line_num;
		int  band_num;
	} QUIVIMAGE_HEAD_INFO;

	typedef struct _tagQuivAuxInfo
	{
		int		   valid_flag;		//	该值为=1时有效，其他情况下辅助信息为无效值
		LONLAT_POS		nadir_pos;	//	对应的星下点的位置
		LONLAT_POS		line_left_pos;//	该距离线对应的左侧的位置（精度和纬度）
		LONLAT_POS		line_right_pos;	//	该距离线对应的右侧的位置（精度和纬度）
		SAT_POS			satpos;     //卫星的x,y,z坐标以及vx,vy,vz
		SAT_ATT			satatt;     //卫星的姿态参数,俯仰滚动偏航，r,p,y
		unsigned short line_count;
		_TIMESTAMP		line_time;			//	照射该脉冲的时间信息
		double      line_num;
		double		att_time;			//	姿态数据的时间信息
		double		gps_time;			//	GPS定位数据时间
		double      star_time;			//  整星星务对时数据
		double      gps_c_time;			//  GPS对时码
	}QUIVIMAGE_AUX_INFO;

	virtual bool read(const char* filename, const char* outPath);
	virtual bool read_band_seperate(const char* filename, const char* outPath);
	virtual bool read_by_scene(const char* filename, const char* outPath);
	virtual bool read_by_scene_band_seperate(const char* filename, const char* outPath);

	bool readHeadInfo(ifstream& fs, QUIVIMAGE_HEAD_INFO& header);
	bool readTimeInfo(ifstream& fs, _TIMESTAMP& t);
	bool readSAT_POS(ifstream& fs, SAT_POS& pos);
	bool readSAT_ATT(ifstream& fs, SAT_ATT& att);
	bool readQuivAuxInfo(ifstream& fs, QUIVIMAGE_AUX_INFO& aux);

	virtual void writeDescXML(const QUIVIMAGE_HEAD_INFO& theHeaderInfo, const char* descXML)const;
	virtual void writeDescXML(const QUIVIMAGE_HEAD_INFO& theHeaderInfo, const vector<QUIVIMAGE_AUX_INFO>& auxList, const char* descXML, int line_offset = 0)const;

	bool time_ready;

protected:
};

#endif