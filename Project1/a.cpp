#include "a.h" 
#include "afx.h" 
#include <string>// ע����<string>������<string.h>����.h����C�����е�ͷ�ļ�
 
using namespace std;
a::a()
{
}
a::~a()
{
}
#define MAX_REC_BUFF		100000	//��󻺴��¼����
#define WAVE_LEN		300			//���γ���
#define REC_VALID		0x55AA55AA	//��¼��Ч�Ա�ʶ
#define REC_SIZE		(sizeof(REC_T))	//��¼��С��ÿ����¼��������REC_ITEM�ṹ�壬��һ���ǻ�׼���Σ��ڶ����Ǽ�Ⲩ��
//����֡����
#define DEFAULT_PRO_CMD_LEN		7	//Ĭ�ϴ���ָ��ȣ��������ݵ�ָ�
#define FRM_LEN_GS 				(DEFAULT_PRO_CMD_LEN + CH_NUM * sizeof(PM_STATE) + 4)	//4:����汾��
#define FRM_LEN_GC 				(DEFAULT_PRO_CMD_LEN + sizeof(CFG_T))
#define FRM_LEN_SM				8
#define FRM_LEN_RA				8
#define FRM_LEN_AL				8	//alarm
#define FRM_LEN_RP				(DEFAULT_PRO_CMD_LEN + sizeof(REC_T))
#define FRM_LEN_GR				(DEFAULT_PRO_CMD_LEN + sizeof(REC_T))
#define FRM_LEN_SC				(DEFAULT_PRO_CMD_LEN + sizeof(CFG_T))
#define FRM_LEN_ST				(DEFAULT_PRO_CMD_LEN + sizeof(RTC_DATE_TIME))	//20180902
#define FRM_LEN_MS				(DEFAULT_PRO_CMD_LEN + 2)						//20190618	Model Select

#define MAX_CMD_LEN	4096	//���֡����

//#define CHECKSUM_EN		//ʹ��У���
#ifndef	CHECKSUM_EN
#define DEFAULT_SUM					0x55	//����У���
#endif


#define WAVE_LEN		300			//���γ���

//ȫ���������Ͷ���
typedef unsigned			char uint8_t;
typedef unsigned short		int uint16_t;
typedef unsigned			int uint32_t;
typedef uint32_t			u32;
typedef uint16_t			u16;
typedef uint8_t				u8;

typedef   signed			char int8_t;
typedef   signed short		int int16_t;
typedef   signed			int int32_t;
typedef int32_t				s32;
typedef int16_t				s16;
typedef int8_t				s8;

typedef s32	BOOL;

typedef struct tagRtcDateType
{
	uint8_t RTC_WeekDay; /*!< Specifies the RTC Date WeekDay.
						  This parameter can be a value of @ref RTC_WeekDay_Definitions */

	uint8_t RTC_Month;   /*!< Specifies the RTC Date Month (in BCD format).
						  This parameter can be a value of @ref RTC_Month_Date_Definitions */

	uint8_t RTC_Date;     /*!< Specifies the RTC Date.
						  This parameter must be set to a value in the 1-31 range. */

	uint8_t RTC_Year;     /*!< Specifies the RTC Date Year.
						  This parameter must be set to a value in the 0-99 range. */
}RTC_DateTypeDef;
typedef struct tagRtcTimeType
{
	uint8_t RTC_Hours;    /*!< Specifies the RTC Time Hour.
						  This parameter must be set to a value in the 0-12 range
						  if the RTC_HourFormat_12 is selected or 0-23 range if
						  the RTC_HourFormat_24 is selected. */

	uint8_t RTC_Minutes;  /*!< Specifies the RTC Time Minutes.
						  This parameter must be set to a value in the 0-59 range. */

	uint8_t RTC_Seconds;  /*!< Specifies the RTC Time Seconds.
						  This parameter must be set to a value in the 0-59 range. */

	uint8_t RTC_H12;      /*!< Specifies the RTC AM/PM Time.
						  This parameter can be a value of @ref RTC_AM_PM_Definitions */
}RTC_TimeTypeDef;


//����״̬����
typedef enum tagJudgeType {
	JUDGE_NONE = 0,
	JUDGE_BAD,
	JUDGE_OK
}JUDGE_TYPE;

//��¼��Ŀ�ṹ�嶨�壬������¼�е�һ�����Σ���׼���λ򱻲Ⲩ��
typedef struct tagRecItem {
	u16 wave[WAVE_LEN];					//��ǰ����
	RTC_DateTypeDef RTC_DateStruct;
	RTC_TimeTypeDef RTC_TimeStruct;

	u16 ch;								//ͨ����			//20190618
	u16 modelIdx;						//��׼�������		//20190618
	int totalCnt;						//�ܼƸ���
	int alarmCnt;						//��Ʒ����
	int badCnt;							//ʵ�ʲ�������
	float cpk;							//CPK		ANAL_CalcCPK()
	float stability;					//������		ANAL_CalcStb()
	float load;							//Load settings in configuration, to comvert peakVal to Kg
	u16 peakVal;						//ѹ���ķ�ֵ(ADֵ����ʹ��ǰ������Ҫת����λ) ANAL_Align()
	u16 sampTime;						//�������ڿ��

	int tolIdx;							//���������						ANAL_IsAlm()

	float areaMax, areaMin, areaErr;	//����ж����(������/������/DATA)	ANAL_IsAlm()
	float peakMax, peakMin, peakErr;	//��ֵ�ж����(������/������/DATA)	ANAL_IsAlm()
	float shiftMax, shiftMin, shiftErr;	//ƫ��ֵ(������/������/DATA)		ANAL_IsAlm()
	float SCMax, SCErr;					//SC(����ƫ��)������/DATA��			ANAL_IsAlm()
	BOOL almArea, almPeak, almShift, almSC, isAlm;	//����״̬				ANAL_IsAlm()

	u16 triggerThresh;					//������ֵ ��ͨ�� model��¼�Ĵ�����ֵΪ�Զ�����ģʽ�Ĵ�����ֵ		ANAL_FirstLearnAutoMode()

	//���²����Ƿ�������ʱ���õ�
	int peakIdx;						//��ֵ��λ��		ANAL_Align()
	int alignIdx;						//�����λ��		ANAL_Align()	20170611
	float peakAvr;						//���������ѹ�����ε����ֵ��90%���ϵ�����ѹ���ļ��㡢����������ǵ�ƽ��ֵ��Ȼ�����ƽ��ֵ��Ϊѹ���ķ�ֵ�����ж�	ANAL_GetPeakAvr()
	float peakAvrRef;					//peakAvr�ı��ݡ�����׼������������Чʱ��peakAvr��ÿ�β�����ᱻ���¼��㣬peakAvrRef�б���δ�����Ļ�׼���ε�peakAvrֵ

	int idxAreaStart;					//����ж�����㣬���ڡ�����ж��ķ�Χ��areaJudgeZone=0ʱ������  areaJudgeZoneָ������ж��Ŀ�ʼλ�á�����ֵΪ0����ʾ��
										//���ж�����Ŀ�ʼλ���Զ��������������Ϊ������ֵ��ʱ�򡢱�ʾ�ǴӲ��η�ֵ���90%��λ�ÿ�ʼ����ָ���ĵ�����Ϊ����ж�����Ŀ�ʼλ�á�
										//ANAL_GetAreaSum(REC_ITEM *)�м���,  ANAL_GetAreaSum(FRAME_T *)��ʹ��
	int idxAreaEnd;						//����ж����յ㡣
	float areaSum;						//����ж�ֵ		ANAL_GetAreaSum(REC_ITEM *)�м���

	int idxSCStart;						//SC�ж���㡣ָ������ƫ�������㿪ʼλ�á����ڂ�Ϊ0����ʾ����Ŀ�ʼλ���Զ������������������ֵ��ʾ�ǴӲ��η�ֵ�Ҳ�80%��λ������ָ���ĵ�����Ϊƫ�����Ŀ�ʼλ�á�
										//ANAL_CalcSCStart()�и�ֵ���ڻ�׼���βɼ��ɹ�ʱ���øú���
	int idxSCEnd;						//SC�ж��յ㡣
	float SCSum;						//SC�ж�������

	BOOL isModelAdp;					//�Ƿ���¹���׼���Ρ���adaptive=CFG_ENABLE�������µ�10 ���Ի�׼���ν�����������������õ�ʱ�����ڲ�����ѹ����ҵ�����Ի�׼���ν��еĲ�����λ���ָ�������Ļ�׼���Ρ�

	BOOL isJudge;						//�Ƿ񾭹��˹��ж�	0������δ����1��ȷ����
	JUDGE_TYPE judge;					//�˹��ж����	0���ޣ�1�������Ļ���2���ھ���

	//��¼��Ч�Ա�ʶ
	u32 valid;							//��¼��Ч�Ա�ʶ
}REC_ITEM;
//��ʱ��ת��Ϊ�ַ���
CString RTC_String(RTC_DateTypeDef RTC_DateStruct, RTC_TimeTypeDef RTC_TimeStruct)
{
	CString str;
	str.Empty();
	if (!((0 == RTC_DateStruct.RTC_Year) && (0 == RTC_DateStruct.RTC_Month)))
		str.Format("%04d/%02d/%02d %02d:%02d:%02d", RTC_DateStruct.RTC_Year + 2000, RTC_DateStruct.RTC_Month, RTC_DateStruct.RTC_Date,
			RTC_TimeStruct.RTC_Hours, RTC_TimeStruct.RTC_Minutes, RTC_TimeStruct.RTC_Seconds);
	return str;
}
string date_String(RTC_DateTypeDef RTC_DateStruct )
{
	string str=""; 
	int a = RTC_DateStruct.RTC_Year + 2000;
	int b=RTC_DateStruct.RTC_Month;
	int c = RTC_DateStruct.RTC_Date; 
	 str = str+to_string(a) + "/" + to_string(b) + "/" + to_string(c);
	return str;
}
string time_String( RTC_TimeTypeDef RTC_TimeStruct)
{ 
	string str = "";
	int a = RTC_TimeStruct.RTC_Hours;
	int b = RTC_TimeStruct.RTC_Minutes;
	int c = RTC_TimeStruct.RTC_Seconds;
	str = str + to_string(a) + ":" + to_string(b) + ":" + to_string(c);
	return str;
}
//��¼�ṹ�嶨�壬����һ�������ļ�¼
typedef struct tagRec
{
	REC_ITEM m_model;
	REC_ITEM m_wave;
} REC_T;
typedef unsigned char Byte;

string anlzRP(unsigned char buf[])
{
	REC_T m_rec;  
	u16 len = *(u16*)(buf + 1);
	if (FRM_LEN_RP == len)
	{
		REC_T *p = (REC_T *)(buf + 5);
		int ch = p->m_wave.ch;
		if ((ch >= 0) && (ch < 1))
		{
			memcpy(&m_rec, buf + 5, REC_SIZE);
			if (REC_VALID == m_rec.m_wave.valid)	//�ɼ����μ�¼��Ч
			{
			}
		}
	}
	string m_model_wave = "[";				//��׼����
	for (int i = 0; i < WAVE_LEN; i++)
	{
		if (i == 0)
		{ 
		}
		else {
			m_model_wave = m_model_wave + ",";
		}
			m_model_wave = m_model_wave + to_string(m_rec.m_model.wave[i]);
	}
	m_model_wave = m_model_wave + "]";
	string m_wave_wave = "[";				//���Ⲩ��
	for (int i = 0; i < WAVE_LEN; i++)
	{
		if (i == 0)
		{
		}
		else {
			m_wave_wave = m_wave_wave + ",";
		}
		m_wave_wave = m_wave_wave + to_string(m_rec.m_wave.wave[i]);
	}
	m_wave_wave = m_wave_wave + "]";

	string strbase = "";
	strbase = strbase 
		+ "{ \"m_model\":"+ 
		" {\"wave\":"+ m_model_wave+
		",\"RTC_DateStruct\":\""+ date_String(m_rec.m_wave.RTC_DateStruct)+
		"\",\"RTC_TimeStruct\":\"" + time_String(m_rec.m_wave.RTC_TimeStruct)+
		"\",\"ch\":\"" + to_string(m_rec.m_wave.ch) +
		"\",\"modelIdx\":\"" + to_string(m_rec.m_wave.modelIdx) +
		"\",\"totalCnt\":\"" + to_string(m_rec.m_wave.totalCnt) +
		"\",\"alarmCnt\":\"" + to_string(m_rec.m_wave.alarmCnt) +
		"\",\"badCnt\":\"" + to_string(m_rec.m_wave.badCnt) +
		"\",\"cpk\":\"" + to_string(m_rec.m_wave.cpk) +
		"\",\"stability\":\"" + to_string(m_rec.m_wave.stability) +
		"\",\"load\":\"" + to_string(m_rec.m_wave.load) +
		"\",\"peakVal\":\"" + to_string(m_rec.m_wave.peakVal) +
		"\",\"sampTime\":\"" + to_string(m_rec.m_wave.sampTime) +
		"\",\"tolIdx\":\"" + to_string(m_rec.m_wave.tolIdx) +
		"\",\"areaMax\":\"" + to_string(m_rec.m_wave.areaMax)+
		"\",\"areaMin\":\"" + to_string(m_rec.m_wave.areaMin)+
		"\",\"areaErr\":\"" + to_string(m_rec.m_wave.areaErr) +
		"\",\"peakMax\":\"" + to_string(m_rec.m_wave.peakMax) +
		"\",\"peakMin\":\"" + to_string(m_rec.m_wave.peakMin) +
		"\",\"peakErr\":\"" + to_string(m_rec.m_wave.peakErr) +
		"\",\"shiftMax\":\"" + to_string(m_rec.m_wave.shiftMax) +
		"\",\"shiftMin\":\"" + to_string(m_rec.m_wave.shiftMin) +
		"\",\"shiftErr\":\"" + to_string(m_rec.m_wave.shiftErr) +
		"\",\"SCMax\":\"" + to_string(m_rec.m_wave.SCMax) +
		"\",\"SCErr\":\"" + to_string(m_rec.m_wave.SCErr) +
		"\",\"almArea\":\"" + to_string(m_rec.m_wave.almArea) +
		"\",\"almPeak\":\"" + to_string(m_rec.m_wave.almPeak) +
		"\",\"almShift\":\"" + to_string(m_rec.m_wave.almShift) +
		"\",\"almSC\":\"" + to_string(m_rec.m_wave.almSC) +
		"\",\"isAlm\":\"" + to_string(m_rec.m_wave.isAlm) +
		"\",\"triggerThresh\":\"" + to_string(m_rec.m_wave.triggerThresh) +
		"\",\"peakIdx\":\"" + to_string(m_rec.m_wave.peakIdx) +
		"\",\"alignIdx\":\"" + to_string(m_rec.m_wave.alignIdx) +
		"\",\"peakAvr\":\"" + to_string(m_rec.m_wave.peakAvr) +
		"\",\"peakAvrRef\":\"" + to_string(m_rec.m_wave.peakAvrRef) +
		"\",\"idxAreaStart\":\"" + to_string(m_rec.m_wave.idxAreaStart) +
		"\",\"idxAreaEnd\":\"" + to_string(m_rec.m_wave.idxAreaEnd) +
		"\",\"areaSum\":\"" + to_string(m_rec.m_wave.areaSum) +
		"\",\"idxSCStart\":\"" + to_string(m_rec.m_wave.idxSCStart) +
		"\",\"idxSCEnd\":\"" + to_string(m_rec.m_wave.idxSCEnd) +
		"\",\"SCSum\":\"" + to_string(m_rec.m_wave.SCSum) +
		"\",\"isModelAdp\":\"" + to_string(m_rec.m_wave.isModelAdp) +
		"\",\"isJudge\":\"" + to_string(m_rec.m_wave.isJudge) +
		"\",\"judge\":\"" + to_string(m_rec.m_wave.judge) +
		"\",\"valid\":\"" + to_string(m_rec.m_wave.valid) +
		"\"},"
		+ "\"m_wave\":{\"wave\":" + m_wave_wave+
		",\"RTC_DateStruct\":\"" + date_String(m_rec.m_wave.RTC_DateStruct) +
		"\",\"RTC_TimeStruct\":\"" + time_String(m_rec.m_wave.RTC_TimeStruct) +
		"\",\"ch\":\"" + to_string(m_rec.m_model.ch) +
		"\",\"modelIdx\":\"" + to_string(m_rec.m_model.modelIdx) +
		"\",\"totalCnt\":\"" + to_string(m_rec.m_model.totalCnt) +
		"\",\"alarmCnt\":\"" + to_string(m_rec.m_model.alarmCnt) +
		"\",\"badCnt\":\"" + to_string(m_rec.m_model.badCnt) +
		"\",\"cpk\":\"" + to_string(m_rec.m_model.cpk) +
		"\",\"stability\":\"" + to_string(m_rec.m_model.stability) +
		"\",\"load\":\"" + to_string(m_rec.m_model.load) +
		"\",\"peakVal\":\"" + to_string(m_rec.m_model.peakVal) +
		"\",\"sampTime\":\"" + to_string(m_rec.m_model.sampTime) +
		"\",\"tolIdx\":\"" + to_string(m_rec.m_model.tolIdx) +
		"\",\"areaMax\":\"" + to_string(m_rec.m_model.areaMax) +
		"\",\"areaMin\":\"" + to_string(m_rec.m_model.areaMin) +
		"\",\"areaErr\":\"" + to_string(m_rec.m_model.areaErr) +
		"\",\"peakMax\":\"" + to_string(m_rec.m_model.peakMax) +
		"\",\"peakMin\":\"" + to_string(m_rec.m_model.peakMin) +
		"\",\"peakErr\":\"" + to_string(m_rec.m_model.peakErr) +
		"\",\"shiftMax\":\"" + to_string(m_rec.m_model.shiftMax) +
		"\",\"shiftMin\":\"" + to_string(m_rec.m_model.shiftMin) +
		"\",\"shiftErr\":\"" + to_string(m_rec.m_model.shiftErr) +
		"\",\"SCMax\":\"" + to_string(m_rec.m_model.SCMax) +
		"\",\"SCErr\":\"" + to_string(m_rec.m_model.SCErr) +
		"\",\"almArea\":\"" + to_string(m_rec.m_model.almArea) +
		"\",\"almPeak\":\"" + to_string(m_rec.m_model.almPeak) +
		"\",\"almShift\":\"" + to_string(m_rec.m_model.almShift) +
		"\",\"almSC\":\"" + to_string(m_rec.m_model.almSC) +
		"\",\"isAlm\":\"" + to_string(m_rec.m_model.isAlm) +
		"\",\"triggerThresh\":\"" + to_string(m_rec.m_model.triggerThresh) +
		"\",\"peakIdx\":\"" + to_string(m_rec.m_model.peakIdx) +
		"\",\"alignIdx\":\"" + to_string(m_rec.m_model.alignIdx) +
		"\",\"peakAvr\":\"" + to_string(m_rec.m_model.peakAvr) +
		"\",\"peakAvrRef\":\"" + to_string(m_rec.m_model.peakAvrRef) +
		"\",\"idxAreaStart\":\"" + to_string(m_rec.m_model.idxAreaStart) +
		"\",\"idxAreaEnd\":\"" + to_string(m_rec.m_model.idxAreaEnd) +
		"\",\"areaSum\":\"" + to_string(m_rec.m_model.areaSum) +
		"\",\"idxSCStart\":\"" + to_string(m_rec.m_model.idxSCStart) +
		"\",\"idxSCEnd\":\"" + to_string(m_rec.m_model.idxSCEnd) +
		"\",\"SCSum\":\"" + to_string(m_rec.m_model.SCSum) +
		"\",\"isModelAdp\":\"" + to_string(m_rec.m_model.isModelAdp) +
		"\",\"isJudge\":\"" + to_string(m_rec.m_model.isJudge) +
		"\",\"judge\":\"" + to_string(m_rec.m_model.judge) +
		"\",\"valid\":\"" + to_string(m_rec.m_model.valid) +
		"\"} }";
	 
	return strbase;
}


extern "C" __declspec(dllexport) void getResult(Byte  *buf, Byte *result, int size);
void  __declspec(dllexport) getResult(Byte *buf, Byte *result, int size)
{
	string str=anlzRP(buf);
	const char *p = str.c_str();
	for (int i = 0; i < str.size(); i++)
	{
		result[i] =(Byte) p[i];
	}
	return;
}

extern "C" __declspec(dllexport) int add(int a, int b);
int add(int a, int b)
{
	return a + b;
}
