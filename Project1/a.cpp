#include "a.h" 
#include "afx.h" 
#include <string>// 注意是<string>，不是<string.h>，带.h的是C语言中的头文件
 
using namespace std;
a::a()
{
}
a::~a()
{
}
#define MAX_REC_BUFF		100000	//最大缓存记录条数
#define WAVE_LEN		300			//波形长度
#define REC_VALID		0x55AA55AA	//记录有效性标识
#define REC_SIZE		(sizeof(REC_T))	//记录大小，每条记录包含两个REC_ITEM结构体，第一个是基准波形，第二个是检测波形
//数据帧长度
#define DEFAULT_PRO_CMD_LEN		7	//默认串口指令长度（不带数据的指令）
#define FRM_LEN_GS 				(DEFAULT_PRO_CMD_LEN + CH_NUM * sizeof(PM_STATE) + 4)	//4:程序版本号
#define FRM_LEN_GC 				(DEFAULT_PRO_CMD_LEN + sizeof(CFG_T))
#define FRM_LEN_SM				8
#define FRM_LEN_RA				8
#define FRM_LEN_AL				8	//alarm
#define FRM_LEN_RP				(DEFAULT_PRO_CMD_LEN + sizeof(REC_T))
#define FRM_LEN_GR				(DEFAULT_PRO_CMD_LEN + sizeof(REC_T))
#define FRM_LEN_SC				(DEFAULT_PRO_CMD_LEN + sizeof(CFG_T))
#define FRM_LEN_ST				(DEFAULT_PRO_CMD_LEN + sizeof(RTC_DATE_TIME))	//20180902
#define FRM_LEN_MS				(DEFAULT_PRO_CMD_LEN + 2)						//20190618	Model Select

#define MAX_CMD_LEN	4096	//最大帧长度

//#define CHECKSUM_EN		//使能校验和
#ifndef	CHECKSUM_EN
#define DEFAULT_SUM					0x55	//备用校验和
#endif


#define WAVE_LEN		300			//波形长度

//全局数据类型定义
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


//报警状态定义
typedef enum tagJudgeType {
	JUDGE_NONE = 0,
	JUDGE_BAD,
	JUDGE_OK
}JUDGE_TYPE;

//记录项目结构体定义，描述记录中的一个波形，基准波形或被测波形
typedef struct tagRecItem {
	u16 wave[WAVE_LEN];					//当前波形
	RTC_DateTypeDef RTC_DateStruct;
	RTC_TimeTypeDef RTC_TimeStruct;

	u16 ch;								//通道号			//20190618
	u16 modelIdx;						//基准波形序号		//20190618
	int totalCnt;						//总计个数
	int alarmCnt;						//次品个数
	int badCnt;							//实际不良个数
	float cpk;							//CPK		ANAL_CalcCPK()
	float stability;					//安定性		ANAL_CalcStb()
	float load;							//Load settings in configuration, to comvert peakVal to Kg
	u16 peakVal;						//压力的峰值(AD值，在使用前跟据需要转换单位) ANAL_Align()
	u16 sampTime;						//采样窗口宽度

	int tolIdx;							//公差组序号						ANAL_IsAlm()

	float areaMax, areaMin, areaErr;	//面积判定结果(＋公差/－公差/DATA)	ANAL_IsAlm()
	float peakMax, peakMin, peakErr;	//峰值判定结果(＋公差/－公差/DATA)	ANAL_IsAlm()
	float shiftMax, shiftMin, shiftErr;	//偏移值(＋公差/－公差/DATA)		ANAL_IsAlm()
	float SCMax, SCErr;					//SC(波形偏差)（公差/DATA）			ANAL_IsAlm()
	BOOL almArea, almPeak, almShift, almSC, isAlm;	//报警状态				ANAL_IsAlm()

	u16 triggerThresh;					//触发阈值 本通道 model记录的触发阈值为自动触发模式的触发阈值		ANAL_FirstLearnAutoMode()

	//以下参数是分析波形时会用到
	int peakIdx;						//峰值点位置		ANAL_Align()
	int alignIdx;						//对齐点位置		ANAL_Align()	20170611
	float peakAvr;						//搜索相对于压力波形的最大值的90%以上的所有压力的检测点、并计算出它们的平均值、然后将这个平均值作为压力的峰值进行判定	ANAL_GetPeakAvr()
	float peakAvrRef;					//peakAvr的备份。当基准値补偿机能有效时，peakAvr在每次采样后会被重新计算，peakAvrRef中保存未补偿的基准波形的peakAvr值

	int idxAreaStart;					//面积判定的起点，仅在“面积判定的范围”areaJudgeZone=0时起作用  areaJudgeZone指定面积判定的开始位置。初期值为0、表示面
										//积判定计算的开始位置自动决定。如果设置为其它数值的时候、表示是从波形峰值左侧90%的位置开始向左到指定的点数作为面积判定计算的开始位置。
										//ANAL_GetAreaSum(REC_ITEM *)中计算,  ANAL_GetAreaSum(FRAME_T *)中使用
	int idxAreaEnd;						//面积判定的终点。
	float areaSum;						//面积判定值		ANAL_GetAreaSum(REC_ITEM *)中计算

	int idxSCStart;						//SC判定起点。指定波形偏差量计算开始位置。初期値为0、表示计算的开始位置自动决定。如果是其它数值表示是从波形峰值右侧80%的位置向左到指定的点数作为偏差计算的开始位置。
										//ANAL_CalcSCStart()中赋值，在基准波形采集成功时调用该函数
	int idxSCEnd;						//SC判定终点。
	float SCSum;						//SC判定面积求和

	BOOL isModelAdp;					//是否更新过基准波形。当adaptive=CFG_ENABLE，以最新的10 根对基准波形进行修正。如果在设置的时间以内不进行压着作业、将对基准波形进行的补偿复位、恢复到最初的基准波形。

	BOOL isJudge;						//是否经过人工判定	0：数据未定，1：确定。
	JUDGE_TYPE judge;					//人工判定结果	0：无，1：真正的坏，2：在决定

	//记录有效性标识
	u32 valid;							//记录有效性标识
}REC_ITEM;
//将时间转换为字符串
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
//记录结构体定义，描述一条完整的记录
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
			if (REC_VALID == m_rec.m_wave.valid)	//采集波形记录有效
			{
			}
		}
	}
	string m_model_wave = "[";				//基准波形
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
	string m_wave_wave = "[";				//被测波形
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
