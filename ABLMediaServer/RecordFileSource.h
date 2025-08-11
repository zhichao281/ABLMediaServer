#ifndef _RecordFileSource_
#define _RecordFileSource_

//��¼��ѯ������m3u8�ļ�
class m3u8FileList
{
public:
	m3u8FileList(char* szName);
	~m3u8FileList();

	char      m3u8Name[string_length_512]; //����
	int64_t   lastTime;                    //���ۿ�ʱ�� 
};

#define     max_hls_replay_time            (1000 * 3600) * 48  // hls��ѯ����������48Сʱ�ڹۿ������������ 

#ifdef USE_BOOST
typedef boost::shared_ptr<m3u8FileList>                    m3u8FileList_ptr;
typedef boost::unordered_map<string, m3u8FileList_ptr>     m3u8FileList_ptrMap;
#else
typedef std::shared_ptr<m3u8FileList>                    m3u8FileList_ptr;
typedef std::unordered_map<string, m3u8FileList_ptr>     m3u8FileList_ptrMap;
#endif


class CRecordFileSource
{
public:
   CRecordFileSource(char* app,char* stream);
   ~CRecordFileSource();

   uint64_t                fileKeepMaxTime;//¼����󱣴�ʱ�� ����ȷ��ÿһ·ý��Դ���ȴ������ļ���ȡĬ��ֵ��addStreamProxy ��openRtpServer ���������޸� 
   m3u8FileList_ptrMap     m_m3u8FileMap;
   std::mutex              m3u8NameMutex; 
   bool                    AddM3u8FileToMap(char* szM3u8Name);
   bool                    UpdateM3u8FileTime(char* szM3u8Name);
   int                     DeleteM3u8ExpireFile();

   bool          queryRecordFile(char* szRecordFileName);
   void          Sort();
   std::mutex    RecordFileLock;
   char          szDeleteFile[512];

   char          m_app[string_length_256];
   char          m_stream[string_length_512];
   char          m_szShareURL[string_length_512];
   char          szBuffer[string_length_4096];
   char          szJson[string_length_4096];

   bool   AddRecordFile(char* szFileName);
   bool   UpdateExpireRecordFile(char* szNewFileName, int* nFileSize);

   list<uint64_t> fileList;
};

#endif