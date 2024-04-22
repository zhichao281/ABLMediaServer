/* ---------------------------------------------------------- 
文件名称：BASE64_API.h 
 
作者：秦建辉 
 
MSN：splashcn@msn.com 
 
当前版本：V1.1 
 
历史版本： 
    V1.1    2010年05月11日 
            修正BASE64解码的Bug。 
 
    V1.0    2010年05月07日 
            完成正式版本。 
 
功能描述： 
    BASE64编码和解码 
 
接口函数： 
    Base64_Encode 
    Base64_Decode 
 
说明： 
    1.  参考openssl-1.0.0。 
    2.  改进接口，以使其适应TCHAR字符串。 
    3.  修正EVP_DecodeBlock函数解码时未去掉填充字节的缺陷。 
 ------------------------------------------------------------ */  
#pragma once  
  
#include <windows.h>  
  
#ifdef  __cplusplus  
extern "C" {  
#endif  
  
void encodetribyte(unsigned char * in, unsigned char * out, int len);
int decodetribyte(unsigned char * in, unsigned char * out);
int Base64Encode(unsigned char * b64, const unsigned char * input, ULONG stringlen = 0);
int Base64Decode(unsigned char * output, const unsigned char * b64, ULONG codelen = 0);

#ifdef  __cplusplus  
}  
#endif  
