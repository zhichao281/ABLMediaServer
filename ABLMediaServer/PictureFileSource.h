#ifndef _PictureFileSource_
#define _PictureFileSource_

class CPictureFileSource
{
public:
   CPictureFileSource(char* app,char* stream);
   ~CPictureFileSource();
   
   bool          queryPictureFile(char* szPictureFileName);
   void          Sort();
   std::mutex    PictureFileLock;
   char          szDeleteFile[512];

   char          m_app[string_length_256];
   char          m_stream[string_length_512];
   char          m_szShareURL[string_length_512];
   char          szBuffer[256];

   bool          AddPictureFile(char* szFileName);
   bool          UpdateExpirePictureFile(char* szNewFileName);

   list<uint64_t> fileList;
};

#endif