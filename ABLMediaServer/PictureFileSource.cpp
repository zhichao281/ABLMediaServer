/*
功能：
    1、装入老的历史图片文件名字
    2、对图片文件名字进行升序排列
	3、增加新图片文件文件，追加到list的尾部
	4、删除过期的图片文件
	5、根据 app\ stream 、时间段 查找出符合条件的所有图片文件名字 
	6、根据 app\ stream \ 一个图片名字 ，判断该文件是否存在  
	 
日期    2022-03-18
作者    罗家兄弟 
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "PictureFileSource.h"

extern MediaServerPort                       ABL_MediaServerPort;

CPictureFileSource::CPictureFileSource(char* app, char* stream)
{
	memset(m_app,0x00,sizeof(m_app));
	memset(m_stream, 0x00, sizeof(m_stream));
	memset(m_szShareURL, 0x00, sizeof(m_szShareURL));
 
	strcpy(m_app, app);
	strcpy(m_stream, stream);
	sprintf(m_szShareURL, "/%s/%s", app, stream);
}

CPictureFileSource::~CPictureFileSource()
{
	malloc_trim(0);
}

bool CPictureFileSource::AddPictureFile(char* szFileName)
{
	std::lock_guard<std::mutex> lock(PictureFileLock);

	memset(szBuffer, 0x00, sizeof(szBuffer));
	memcpy(szBuffer, szFileName, strlen(szFileName) - 4);

	fileList.push_back(atoll(szBuffer));
 
	return true;
}

void CPictureFileSource::Sort()
{
	std::lock_guard<std::mutex> lock(PictureFileLock);
 	fileList.sort();
}

//修改过期录像文件
bool  CPictureFileSource::UpdateExpirePictureFile(char* szNewFileName)
{
	std::lock_guard<std::mutex> lock(PictureFileLock);
	uint64_t nGetFile;
	uint64_t nSecond = 0; 
	char    szDateTime[128] = { 0 };
	bool    bUpdateFlag = false;

	if (fileList.size() <= 0 )
	{
		WriteLog(Log_Debug, "UpdateExpirePictureFile %s 尚未有图片文件 ,新名字为 %s ", m_szShareURL, szNewFileName);
		return true  ; 
	}

	while (fileList.size() > ABL_MediaServerPort.pictureMaxCount - 1)
	{
		nGetFile = fileList.front();
		sprintf(szDateTime, "%llu", nGetFile);
		 
		if (true)
		{
			fileList.pop_front();
#ifdef OS_System_Windows
			sprintf(szDeleteFile, "%s%s\\%s\\%s.jpg", ABL_MediaServerPort.picturePath, m_app, m_stream, szDateTime);
#else 
			sprintf(szDeleteFile, "%s%s/%s/%s.jpg", ABL_MediaServerPort.picturePath, m_app, m_stream, szDateTime);
#endif
			//如果修改失败，回收以后再次修改
			if (rename(szDeleteFile,szNewFileName) != 0 )
			{
				fileList.push_back(nGetFile); 
				WriteLog(Log_Debug, "UpdateExpirePictureFile %s 修改文件 %llu.jpg 失败，回收以后再修改 ", m_szShareURL, nGetFile);
				break;
 			}
			else
			{
			    bUpdateFlag = true;
			    break;
			}
   		}
		else
			break;
	}
   
	return bUpdateFlag ;
}

//查询录像文件是否存在 
bool  CPictureFileSource::queryPictureFile(char* szPictureFileName)
{
	std::lock_guard<std::mutex> lock(PictureFileLock);

	bool bRet = false;
	//文件名字长度有误
	if (strlen(szPictureFileName) != 16) 
		return false;

	//去掉扩展名 .jpg , .bmp , .png 
	if (strstr(szPictureFileName, ".jpg") != NULL || strstr(szPictureFileName, ".bmp") != NULL)
		szPictureFileName[strlen(szPictureFileName) - 4] = 0x00;
	if (strstr(szPictureFileName, ".png") != NULL )
		szPictureFileName[strlen(szPictureFileName) - 5] = 0x00;

#ifdef USE_BOOST
	//判断是否为数字
	if (!boost::all(szPictureFileName, boost::is_digit()))
		return false;
#else
	//判断是否为数字
	if (!ABL::is_digits(szPictureFileName))
		return false;
#endif

 
	list<uint64_t>::iterator it2;
	for (it2 = fileList.begin(); it2 != fileList.end(); it2++)
	{
		if (*it2 == atoll(szPictureFileName))
		{
			bRet = true;
			break;
		}
	}
	return bRet;
}
