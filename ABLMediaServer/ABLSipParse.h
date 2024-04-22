#ifndef _ABLSipParse_H
#define _ABLSipParse_H

#include <map>
#include <vector>

using namespace std;

#define  MaxSipBodyContentLength   64*1024 //最大的body数据长度

struct SipFieldStruct
{
	char szKey[512];
	char szValue[1024*32];
	SipFieldStruct()
	{
		memset(szKey, 0x00, sizeof(szKey));
		memset(szValue, 0x00, sizeof(szValue));
	}
};

struct SipBodyHead
{
	char CmdType[128];
	char SN[128];
	char DeviceID[128];
	char Status[128];
	SipBodyHead()
	{
		memset(CmdType, 0x00, sizeof(CmdType));
		memset(SN, 0x00, sizeof(SN));
		memset(DeviceID, 0x00, sizeof(DeviceID));
		memset(Status, 0x00, sizeof(Status));
	}
};


typedef  map<string, SipFieldStruct*, less<string> > SipFieldStructMap;
typedef  vector<SipFieldStruct > SipFieldStructVector;

class CABLSipParse
{
	public:
	CABLSipParse();
	~CABLSipParse();

	int          GetSize();
	SipBodyHead  sipBodyHead;
	bool  FreeSipString();

	bool ParseSipString(char* szSipString);
	bool GetFieldValue(char* szKey, char* szValue);

	bool AddFieldValue(char* szKey, char* szValue);
	bool GetFieldValueString(char* szSipString);

	std::mutex           sipLock;
	std::mutex           vectorLock;
	SipFieldStructMap    sipFieldValueMap;

	SipFieldStructVector sipFieldValueVector;
	char                 szSipLineBuffer[MaxSipBodyContentLength];
	char                 szLineString[MaxSipBodyContentLength];

	char                 szSplitStr[2][64];

	char                 szSipBodyContent[MaxSipBodyContentLength];//SipBody数据
};

#endif
