#include "stdafx.h"
#include "ABLSipParse.h"

CABLSipParse::CABLSipParse()
{
	strcpy(szSplitStr[0], ";");
	strcpy(szSplitStr[1], ",");
	nOrder = 1;
}

//�ͷ�sip�ַ����ڴ� 
bool CABLSipParse::FreeSipString()
{
 	for (SipFieldStructMap::iterator iterator1 = sipFieldValueMap.begin(); iterator1 != sipFieldValueMap.end(); ++iterator1)
	{
		SipFieldStruct * sipKey = (*iterator1).second;

		delete sipKey;
		sipKey = NULL;
 	}
	sipFieldValueMap.clear();

 	memset(szSipBodyContent, 0x00, sizeof(szSipBodyContent));
 	return true;
}

CABLSipParse::~CABLSipParse()
{
	std::lock_guard<std::mutex> lock(sipLock);
 	FreeSipString();
}
	
//��sipͷȫ��װ��map���棬�������ҳ���ϸ��������
bool CABLSipParse::ParseSipString(char* szSipString)
{
	std::lock_guard<std::mutex> lock(sipLock);
	nOrder = 1;
	FreeSipString();

	if (strlen(szSipString) <= 4)
		return false;

	SipFieldStruct * sipKey = NULL;
	SipFieldStruct * sipKey2;
	string           strSipStringFull = szSipString;
	int              nPosBody;

	//���ҳ�body����
	memset(szSipBodyContent, 0x00, sizeof(szSipBodyContent));
	nPosBody = strSipStringFull.find("\r\n\r\n", 0);
	if (nPosBody > 0 && nPosBody != string::npos && strlen(szSipString) - (nPosBody + 4) > 0 && strlen(szSipString) - (nPosBody + 4) < MaxSipBodyContentLength)
	{//��body��������
		memcpy(szSipBodyContent, szSipString + nPosBody + 4, strlen(szSipString) - (nPosBody + 4));
		szSipString[nPosBody + 4] = 0x00;
	}

	string strSipString = szSipString;
	string strLineSting;
	string strFieldValue;

	sipKey = NULL;
	int nLineCount = 0;
	int nPos1 = 0 ,  nPos2 = 0,nPos3;
	bool bFindFlag = false;
	while (true)
	{
		bFindFlag = false;
		nPos2 = strSipString.find("\r\n", nPos1);
		if(nPos2 == std::string::npos)
		{   
			nPos2 = strSipString.find("\n", nPos1);
			if (nPos2 == std::string::npos)
 				nPos2 = strSipString.find("\r", nPos1);
 			bFindFlag = true;
		}
 		if (nPos2 > 0 && nPos2 != string::npos && nPos2 - nPos1 > 0 )
		{
			memset(szLineString, 0x00, sizeof(szLineString));
			memcpy(szLineString, szSipString + nPos1, nPos2 - nPos1);

 			sipKey = new SipFieldStruct;
 
			strLineSting = szLineString;
			if (nLineCount == 0)
			{
				nPos3 = strLineSting.find(" ", 0);
				if (nPos3 > 0 && nPos3 != string::npos)
				{
					memcpy(sipKey->szKey, szLineString, nPos3);
					memcpy(sipKey->szValue, szLineString + nPos3 + 1, strlen(szLineString) - (nPos3 + 1));
				}
			}
			else
			{
				nPos3 = strLineSting.find(":", 0);
				if (nPos3 > 0 && nPos3 != string::npos)
				{
					memcpy(sipKey->szKey, szLineString, nPos3);
					memcpy(sipKey->szValue, szLineString + nPos3 + 1, strlen(szLineString) - (nPos3 + 1));
#if 1
					//����boost:string trim ����ȥ���ո�
					string strTrimLeft = sipKey->szValue;
			
#ifdef USE_BOOST
					boost::trim(strTrimLeft);
#else
					ABL::trim(strTrimLeft);
#endif
					strcpy(sipKey->szValue, strTrimLeft.c_str());
#endif
				}
			}

			if (strlen(sipKey->szKey) > 0)
			{
				InsertToMap(sipKey);

				//�ѷ�����Ϊһ���ؼ��֣�KEY���洢����
				if (nLineCount == 0)
				{
 					sipKey2 = new SipFieldStruct;

					strcpy(sipKey2->szKey, "Method");
					strcpy(sipKey2->szValue, sipKey->szKey);
					InsertToMap(sipKey2);
				}

				//��������
				int  nPos4=0, nPos5,nPos6;
				char             subKeyValue[32*1024] = { 0 };
				string           strSubKeyValue;

				strFieldValue = sipKey->szValue;
				for (int i = 0; i < 2; i++)
				{//ѭ������2�Σ�һ�� ;��һ�� ,

					while (true)
					{
						nPos5 = strFieldValue.find(szSplitStr[i], nPos4);
						if (nPos5 > 0 && nPos5 != string::npos && nPos5 - nPos4 > 0)
						{
							memset(subKeyValue, 0x00, sizeof(subKeyValue));
							memcpy(subKeyValue, sipKey->szValue + nPos4, nPos5 - nPos4);

							strSubKeyValue = subKeyValue;
							nPos6 = strSubKeyValue.find("=", 0);
							if (nPos6 > 0 && nPos6 != string::npos)
							{
  								sipKey2 = new SipFieldStruct;

								memcpy(sipKey2->szKey, subKeyValue, nPos6);
								memcpy(sipKey2->szValue, subKeyValue + nPos6 + 1, strlen(subKeyValue) - (nPos6 + 1));

#if 1
								//����boost:string trim ����ȥ���ո�
								string strTrimLeft = sipKey2->szKey;

#ifdef USE_BOOST
								boost::trim(strTrimLeft);
#else
								ABL::trim(strTrimLeft);
#endif
								strcpy(sipKey2->szKey, strTrimLeft.c_str());

								//ɾ��˫����
								strTrimLeft = sipKey2->szValue;
							
#ifdef USE_BOOST


								boost::erase_all(strTrimLeft, "\"");
#else
								ABL::erase_all(strTrimLeft, "\"");
#endif
								strcpy(sipKey2->szValue, strTrimLeft.c_str());
#endif
								InsertToMap(sipKey2);
							}
						}
						else
						{
							if (nPos4 > 0 && strstr(sipKey->szValue, szSplitStr[i]) != NULL && strlen(sipKey->szValue) > nPos4)
							{
								memset(subKeyValue, 0x00, sizeof(subKeyValue));
								memcpy(subKeyValue, sipKey->szValue + nPos4, strlen(sipKey->szValue) - nPos4);

								strSubKeyValue = subKeyValue;
								nPos6 = strSubKeyValue.find("=", 0);
								if (nPos6 > 0 && nPos6 != string::npos)
								{
  									sipKey2 = new SipFieldStruct;

									memcpy(sipKey2->szKey, subKeyValue, nPos6);
									memcpy(sipKey2->szValue, subKeyValue + nPos6 + 1, strlen(subKeyValue) - (nPos6 + 1));

#if 1
									//����boost:string trim ����ȥ���ո�
									string strTrimLeft = sipKey2->szKey;
#ifdef USE_BOOST


									boost::trim(strTrimLeft);
#else
									ABL::trim(strTrimLeft);
#endif
									strcpy(sipKey2->szKey, strTrimLeft.c_str());

									//ɾ��˫����
									strTrimLeft = sipKey2->szValue;
								
#ifdef USE_BOOST

									boost::erase_all(strTrimLeft, "\"");

#else
									ABL::erase_all(strTrimLeft, "\"");
#endif
									strcpy(sipKey2->szValue, strTrimLeft.c_str());
#endif
									InsertToMap(sipKey2);
								}
							}

							break;
						}

						nPos4 = nPos5 + 1;
					}

				}//for (int i = 0; i < 2; i++)
			}
			else
			{
				if (sipKey)
				{
				  delete sipKey;
				  sipKey = NULL;
 				}
			}

			if(bFindFlag == false)
			  nPos1 = nPos2+2;
			else 
			  nPos1 = nPos2 + 1;

			nLineCount ++;
		}
		else
			break;
	}

	return true;
}

bool CABLSipParse::GetFieldValue(char* szKey, char* szValue)
{
	std::lock_guard<std::mutex> lock(sipLock);

	SipFieldStruct * sipKey = NULL;
	SipFieldStructMap::iterator iterator1 = sipFieldValueMap.find(szKey);

	if (iterator1 != sipFieldValueMap.end())
	{
		sipKey = (*iterator1).second;
		strcpy(szValue, sipKey->szValue);
		 
		return true;
	}
	else
		return false;
}

bool CABLSipParse::AddFieldValue(char* szKey, char* szValue)
{
	std::lock_guard<std::mutex> lock(vectorLock);

	SipFieldStruct  sipKey ;
	strcpy(sipKey.szKey, szKey);
	strcpy(sipKey.szValue, szValue);
	sipFieldValueVector.push_back(sipKey);

	return true;
}

bool CABLSipParse::GetFieldValueString(char* szSipString)
{
	std::lock_guard<std::mutex> lock(vectorLock);

	if (sipFieldValueVector.size() == 0)
		return false;

	int nSize = sipFieldValueVector.size();
	int i;
	for (i = 0; i < nSize; i++)
	{
		sprintf(szSipLineBuffer, "%s: %s\r\n", sipFieldValueVector[i].szKey, sipFieldValueVector[i].szValue);
		if (i == 0)
			strcpy(szSipString, szSipLineBuffer);
		else
			strcat(szSipString, szSipLineBuffer);
	}
	strcat(szSipString, "\r\n");
	sipFieldValueVector.clear();

	return true;
}

int   CABLSipParse::GetSize()
{
	std::lock_guard<std::mutex> lock(vectorLock);
	return sipFieldValueMap.size();
}

bool CABLSipParse::InsertToMap(SipFieldStruct * inSipKey)
{
	SipFieldStructMap::iterator iterator1 = sipFieldValueMap.find(inSipKey->szKey);
	if (iterator1 != sipFieldValueMap.end())
	{
		sprintf(szTemp2, "%llu_%X_%d", nOrder,this,rand());
		strcat(inSipKey->szKey, szTemp2);
		sipFieldValueMap.insert(SipFieldStructMap::value_type(inSipKey->szKey, inSipKey));

		nOrder++;	
	}
	else
	{
	   sipFieldValueMap.insert(SipFieldStructMap::value_type(inSipKey->szKey, inSipKey));

	}
	return true;
}
