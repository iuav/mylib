#ifndef ZY3_QVProcReader_H
#define ZY3_QVProcReader_H

#include "QVProcReader.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
using namespace std;

class ZY3QVProcReader : public QVProcReader
{
public:
	ZY3QVProcReader();

	typedef struct _tagRpcStruct
	{
		double LINE_OFF;
		double SAMP_OFF;
		double LAT_OFF;
		double LONG_OFF;
		double HEIGHT_OFF;
		double LINE_SCALE;
		double SAMP_SCALE;
		double LAT_SCALE;
		double LONG_SCALE;
		double HEIGHT_SCALE;
		double LINE_NUM_COEFF[20];
		double LINE_DEN_COEFF[20];
		double SAMP_NUM_COEFF[20];
		double SAMP_DEN_COEFF[20];
	} RPC_STRUCT;

	typedef struct 
	{
		char format_id[64];			// ��ʽId
		char version_id[16];		// �汾��
		char station_id[16];		//	���ո����ݵĵ���վ�ı�ʶ
		char satellite_id[64];	    //	��Ӧ�����Ǳ�ʶȫ��
		char sensor_id[512];		//	��Ӧ�Ĵ�������ʶȫ��
		char jobTaskID [32];	    //	�����ʶ
		int	 orbit_num;   		//  �����
		int	 channel_num;	 	//  ͨ����
		int	 totalfiles;	 	    //  �����񵥴�������ļ�����
		int	 files_num;	 	    //  ���δ�����ļ������������е��ļ����
		int	 desample_num;	    //	���п�������ʱ�Ľ�������,��һ���Ľ�������
		int image_mode;			// ͼ��ģʽ������ֵ=1ʱ����ͼ��Ϊ�Ҷ�ͼ�񣬷���Ϊ��ɫ�ϳ�ͼ���磺0
		_TIMESTAMP start_time;	// ���ݽ��յĿ�ʼʱ�䣬ΪUTCʱ��
		_TIMESTAMP end_time;	// ���ݽ��յĽ���ʱ�䣬ΪUTCʱ��
		LONLAT_POS station_pos;	//	��Ӧ�ĵ���վ�ڵ����ϵ�λ����Ϣ	����վ��λ��Ϣ����γ�ȣ�
	} QUIVIMAGE_HEAD_INFO;

	typedef struct _tagSceneGlobalInfo
	{// ��ȫ�ָ�����Ϣ
		char info_header[4];		// ��ȫ�ָ�����Ϣ֡ͷ
		int	 scene_count;			// ���������ó���ģʽ�¾��ļ���ֵ����1��ʼ���任����ģʽ֮�����´�1��ʼ����
		char satellite_id[64];		// ��Ӧ�����Ǳ�ʶ�����Ǳ�ʶ����ԭʼ���ݼ�¼�뽻����ʽ���Ĺ涨
		char sensor_id[512];		// ��Ӧ�Ĵ�������ʶ����������ʶ���ա�ԭʼ���ݼ�¼�뽻����ʽ���Ĺ涨
		int work_mode;				// �þ�ͼ����ģʽ��Ϣ�����ͬһ�������ɱ乤��ģʽ�������1��ʼ����������
		int scene_lines;			// �þ�ͼ���Ӧ����ͼ������
		int scene_samples;			// �þ�ͼ���Ӧ����ͼ������
		int qv_line_info_length;	// ����ͼ���и�����Ϣ���ֽ��� ��λΪ�ֽ�
		int bands;					// ������
		int sample_bit_count;		// ����λ��
		char interleave[4];			// �ನ��ͼ�����з�ʽ ��Բ�ͬ����ģʽ������Լ�����ֱ�ΪBSQ��Band Sequential format ����˳���ʽ����BIL��Band Interleaved by Line format��
									// ���ΰ���Ԫ�����ʽ����BIP��Band Interleaved by Pixel format�����ΰ��н����ʽ����ǰ��0x20
		_TIMESTAMP first_time;		// ����һ��ͼ��������ʼʱ�� ��ʱ��Ϊ����ʱ�� UTC
		_TIMESTAMP last_time;		// �����һ��ͼ��������ʼʱ�� ��ʱ��Ϊ����ʱ�� UTC
		int image_format;			// ͼ���ʽ������������ʾ��1��Raw��2��jpeg2000��3��tiff��4��gtif
		int compress_algorithm;		// �þ�ͼ��ʹ�õ�ѹ���㷨���������ֱ�ʾ��1��jpeg2000
		float compress_rate;		// �þ�ͼ���ѹ��������Ϣ����2��3��4
		unsigned long long scene_size;	// ��ͼ�����ݴ�С����λΪ�ֽڣ����ͼ���ʽΪRaw��Ϊѹ��ǰ���ݴ�С��������Ϊѹ�������ݴ�С��С�ֽ���
		LONLAT_POS left_upper;		// ���ϽǾ�γ��
		LONLAT_POS right_upper;		// ���ϽǾ�γ��
		LONLAT_POS left_lower;		// ���½Ǿ�γ��
		LONLAT_POS right_lower;		// ���½Ǿ�γ��
		LONLAT_POS scene_center;	// ������γ��
		RPC_STRUCT rpc_info;		// RPCϵ�������Ϣ
		char valid_flag[2];		// ԭʼ�����и�����Ϣ��ʶ����ֵΪ=1ʱ��Ч��������ԭʼ�����и�����Ϣ���ɹ�����ڳ�������в����ĸ�����Ϣ
		int line_info_size;			// ԭʼ�����и�����Ϣ��С
		int line_info_length;		// ԭʼ�����и�����Ϣ���ֽ���
	}SCENE_GLOBAL_INFO;

	typedef struct _tagQuivAuxInfo
	{
		int line_index;		//	����ͼ���и�����Ϣ���� ����ͼ���м�����1��ʼ���ÿ���ͼ������Ϣ������Ӧ�ں�������ͼ��������
		LONLAT_POS		nadir_pos;	//	��Ӧ�����µ��λ��
		LONLAT_POS		line_left_pos;//	����ͼ�����λ�ã����Ⱥ�γ�ȣ�
		LONLAT_POS		line_right_pos;	//	����ͼ���ң����Ⱥ�γ�ȣ�
		SAT_POS			satpos;     //���ǵ�x,y,z�����Լ�vx,vy,vz
		int				att_model;	// 0: r,p,y   1: q1,q2,q3
		SAT_ATT			satatt;     //���ǵ���̬����,��������ƫ����r,p,y
		double		line_time;			//	��ʱ��Ϊ����ʱ�䣬UTC
		double		att_time;			//	��UTC
		double		star_time;			//	��UTC
		//_TIMESTAMP		line_time;			//	��ʱ��Ϊ����ʱ�䣬UTC
	}QUIVIMAGE_AUX_INFO;

	bool readHeader(const char* header);
	bool extractHeader(const char* filename);

	bool read(const char* filename, const char* outPath);
	bool read_by_scene(const char* filename, const char* outPath);

	bool readHeadInfo(FILE* pfIn, QUIVIMAGE_HEAD_INFO& header, FILE* pfOut = NULL);

	bool readHeadInfo(ifstream& fs, QUIVIMAGE_HEAD_INFO& header);
	bool readTimeInfo(FILE* pfIn, _TIMESTAMP& t, FILE* pfOut = NULL);
	bool readTimeInfo(ifstream& fs, _TIMESTAMP& t);
	bool readSAT_POS(FILE* pfIn, SAT_POS& pos, FILE* pfOut = NULL);
	bool readSAT_POS(ifstream& fs, SAT_POS& pos);
	bool readSAT_ATT(FILE* pfIn, SAT_ATT& att, FILE* pfOut = NULL);
	bool readSAT_ATT(ifstream& fs, SAT_ATT& att);
	bool readSceneGlobalInfo(FILE* pfIn, SCENE_GLOBAL_INFO& scene_info, FILE* pfOut = NULL);
	bool readSceneGlobalInfo(ifstream& fs, SCENE_GLOBAL_INFO& scene_info);
	bool readRPC_INFO(FILE* pfIn, RPC_STRUCT& rpc_info, FILE* pfOut = NULL);
	bool readRPC_INFO(ifstream& fs, RPC_STRUCT& rpc_info);
	bool readQuivAuxInfo(FILE* pfIn, QUIVIMAGE_AUX_INFO& aux, FILE* pfOut = NULL);
	bool readQuivAuxInfo(ifstream& fs, QUIVIMAGE_AUX_INFO& aux);

	bool writeRPC(const char* filename, const RPC_STRUCT& rpc_info);


	void writeDescXML(const QUIVIMAGE_HEAD_INFO& headerInfo, const char* descXML)const;
	void writeDescXML(const QUIVIMAGE_HEAD_INFO& headerInfo,
		const SCENE_GLOBAL_INFO& sceneInfo,
		const vector<QUIVIMAGE_AUX_INFO>& auxList,
		const char* descXML,
		int line_offset = 0)const;

	bool time_ready;

protected:
	QUIVIMAGE_HEAD_INFO theHeaderInfo;
	int theLines;
	int theSamples;
	int theBands;
};

#endif