/*
���ܣ�
    1��װ���ϵ���ʷ¼���ļ�����
    2����¼���ļ����ֽ�����������
	3��������¼���ļ��ļ���׷�ӵ�list��β��
	4��ɾ�����ڵ�¼���ļ�
	5������ app\ stream ��ʱ��� ���ҳ���������������¼���ļ����� 
	6������ app\ stream \ һ��¼������ ���жϸ��ļ��Ƿ����  
	 
����    2022-01-13
����    �޼��ֵ� 
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "RecordFileSource.h"
extern MediaServerPort                       ABL_MediaServerPort;
#ifdef USE_BOOST
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
#else
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
#endif
extern CMediaFifo                            pMessageNoticeFifo;          //��Ϣ֪ͨFIFO

CRecordFileSource::CRecordFileSource(char* app, char* stream)
{
	memset(m_app,0x00,sizeof(m_app));
	memset(m_stream, 0x00, sizeof(m_stream));
	memset(m_szShareURL, 0x00, sizeof(m_szShareURL));
 
	fileKeepMaxTime = ABL_MediaServerPort.fileKeepMaxTime ; 
	strcpy(m_app, app);
	strcpy(m_stream, stream);
	sprintf(m_szShareURL, "/%s/%s", app, stream);
}

CRecordFileSource::~CRecordFileSource()
{
	malloc_trim(0);
}

bool CRecordFileSource::AddRecordFile(char* szFileName)
{
	std::lock_guard<std::mutex> lock(RecordFileLock);

	memset(szBuffer, 0x00, sizeof(szBuffer));
	memcpy(szBuffer, szFileName, strlen(szFileName) - 4);
	uint64_t nSecond = GetCurrentSecond() - GetCurrentSecondByTime(szBuffer);

	fileList.push_back(atoll(szBuffer));
 
	return true;
}

void CRecordFileSource::Sort()
{
	std::lock_guard<std::mutex> lock(RecordFileLock);
 	fileList.sort();
}

//�޸Ĺ���¼���ļ�
bool  CRecordFileSource::UpdateExpireRecordFile(char* szNewFileName,int* nFileSize)
{
	std::lock_guard<std::mutex> lock(RecordFileLock);
	uint64_t nGetFile;
	uint64_t nSecond = 0; 
	char    szDateTime[128] = { 0 };
	bool    bUpdateFlag = false;
	*nFileSize = 0;

	if (fileList.size() <= 0 )
	{
		WriteLog(Log_Debug, "UpdateExpireRecordFile %s ��δ��¼���ļ� ,������Ϊ %s ", m_szShareURL, szNewFileName);
		return false ; 
	}

	while (fileList.size() > 0 )
	{
		nGetFile = fileList.front();
		sprintf(szDateTime, "%llu", nGetFile);
		nSecond = GetCurrentSecond() - GetCurrentSecondByTime(szDateTime);
		if (nSecond > (fileKeepMaxTime * 3600))
		{
			fileList.pop_front();
#ifdef OS_System_Windows
			sprintf(szDeleteFile, "%s%s\\%s\\%s.mp4", ABL_MediaServerPort.recordPath, m_app, m_stream, szDateTime);
			struct _stat64 fileBuf;
 			int error = _stat64(szDeleteFile, &fileBuf);
			if (error == 0)
				*nFileSize = fileBuf.st_size;
#else 
			sprintf(szDeleteFile, "%s%s/%s/%s.mp4", ABL_MediaServerPort.recordPath, m_app, m_stream, szDateTime);
			struct stat fileBuf;
			int error = stat(szDeleteFile, &fileBuf);
			if (error == 0)
				*nFileSize = fileBuf.st_size;
#endif
			//����޸�ʧ�ܣ������Ժ��ٴ��޸�
			if (rename(szDeleteFile,szNewFileName) != 0 )
			{
				fileList.push_back(nGetFile); 
				WriteLog(Log_Debug, "UpdateExpireRecordFile %s �޸��ļ� %llu.mp4 ʧ�ܣ������Ժ����޸� ", m_szShareURL, nGetFile);
				break;
			}
			else
			{
			  bUpdateFlag = true;

			  //���һ������һ��mp4�ļ�֪ͨ 
			  if (ABL_MediaServerPort.hook_enable == 1 )
			  {
				  MessageNoticeStruct msgNotice;
				  msgNotice.nClient = NetBaseNetType_HttpClient_DeleteRecordMp4;
				  sprintf(msgNotice.szMsg, "{\"eventName\":\"on_delete_record_mp4\",\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\",\"fileName\":\"%s.mp4\"}", m_app, m_stream, ABL_MediaServerPort.mediaServerID, szDateTime);
				  pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
			  }
			  break;
			}
   		}
		else
			break;
	}

	if(!bUpdateFlag)
		WriteLog(Log_Debug, "UpdateExpireRecordFile %s û��¼���ļ����� ,������Ϊ %s ", m_szShareURL, szNewFileName);
 
	return bUpdateFlag ;
}

//��ѯ¼���ļ��Ƿ���� 
bool  CRecordFileSource::queryRecordFile(char* szRecordFileName)
{
	std::lock_guard<std::mutex> lock(RecordFileLock);

	bool bRet = false;
	//�ļ����ֳ�������
	if (strlen(szRecordFileName) != 14) 
		return false;

	//ȥ����չ�� .flv , .mp4 , .m3u8 
	if (strstr(szRecordFileName, ".flv") != NULL || strstr(szRecordFileName, ".mp4") != NULL)
		szRecordFileName[strlen(szRecordFileName) - 4] = 0x00;
	if (strstr(szRecordFileName, ".m3u8") != NULL )
		szRecordFileName[strlen(szRecordFileName) - 5] = 0x00;

#ifdef USE_BOOST

	//�ж��Ƿ�Ϊ����
	if (!boost::all(szRecordFileName, boost::is_digit()))
		return false;
#else
	//�ж��Ƿ�Ϊ����
	if (!ABL::is_digits(szRecordFileName))
		return false;
#endif

	list<uint64_t>::iterator it2;
	for (it2 = fileList.begin(); it2 != fileList.end(); it2++)
	{
		if (*it2 == atoll(szRecordFileName))
		{
			bRet = true;
			break;
		}
	}

	//�����Ҳ���
	if (ABL_MediaServerPort.hook_enable == 1 && bRet == false )
	{
		MessageNoticeStruct msgNotice;
		msgNotice.nClient = NetBaseNetType_HttpClient_Not_found;
		sprintf(msgNotice.szMsg, "{\"eventName\":\"on_stream_not_found\",\"app\":\"%s\",\"stream\":\"%s___ReplayFMP4RecordFile__%s\",\"mediaServerId\":\"%s\"}", m_app, m_stream, szRecordFileName, ABL_MediaServerPort.mediaServerID);
		pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
	}

	return bRet;
}

m3u8FileList::m3u8FileList(char* szName)
{
	memset(m3u8Name, 0x00, sizeof(m3u8Name));
	strcpy(m3u8Name, szName);
	lastTime = GetTickCount64();
}

m3u8FileList::~m3u8FileList()
{
	char szDeleFile[string_length_512] = { 0 };
	sprintf(szDeleFile, "%s%s", ABL_MediaServerPort.recordPath, m3u8Name+1);
	ABLDeleteFile(szDeleFile);
	WriteLog(Log_Debug, " �Ѿ�ɾ�� m3u8 �ļ� %s ", m3u8Name);
}

//��map����m3u8�ļ����� 
bool  CRecordFileSource::AddM3u8FileToMap(char* szM3u8Name)
{
	std::lock_guard<std::mutex> lock(m3u8NameMutex);

	m3u8FileList_ptr m3u8Prt = NULL;
	m3u8FileList_ptrMap::iterator it;
 
	it = m_m3u8FileMap.find(szM3u8Name);
	if (it != m_m3u8FileMap.end())
		return false; //�Ѿ����� 
	else 
	{
#ifdef USE_BOOST
		m3u8Prt = boost::make_shared<m3u8FileList>(szM3u8Name);
#else
		m3u8Prt = std::make_shared<m3u8FileList>(szM3u8Name);
#endif
	    m_m3u8FileMap.insert(std::make_pair(szM3u8Name, m3u8Prt));
 	    WriteLog(Log_Debug, " ���� m3u8 �ļ� %s ", szM3u8Name);
	    return true;
	}
}

//���� m3u8 �ļ�����ʱ�� 
bool  CRecordFileSource::UpdateM3u8FileTime(char* szM3u8Name)
{
	std::lock_guard<std::mutex> lock(m3u8NameMutex);

	m3u8FileList_ptr m3u8Prt = NULL;
	m3u8FileList_ptrMap::iterator it;
  
	it = m_m3u8FileMap.find(szM3u8Name);
	if (it != m_m3u8FileMap.end())
	{
		m3u8Prt = (*it).second;
		m3u8Prt->lastTime = GetTickCount64();
		return true ;  
 	}
	else
	{
  		return false ;
	}
}

//ɾ�����ڵ�m3u8�ļ� 
int  CRecordFileSource::DeleteM3u8ExpireFile()
{
	std::lock_guard<std::mutex> lock(m3u8NameMutex);

	uint64_t tCurTime = GetTickCount64();
	int      nDelCount  = 0;
	m3u8FileList_ptr m3u8Ptr = NULL ;

	for (m3u8FileList_ptrMap::iterator it = m_m3u8FileMap.begin(); it != m_m3u8FileMap.end();)
	{
		m3u8Ptr = (*it).second;
		if (tCurTime - m3u8Ptr->lastTime > max_hls_replay_time)
		{
			m_m3u8FileMap.erase(it++);
		}
		else
			it++;
	}

	return nDelCount;
}
