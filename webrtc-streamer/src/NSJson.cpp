
#include "NSJson.h"

#include <fstream>
#include <stdexcept>
#include <memory>
#include <iostream>
#include <stdio.h>
#include <string.h>

namespace ABL
{


	/*
	*	Function:		StrTrim
	*	Explanation:	去除字符串两边出现的字符(在strChars中出现的都删除)
	*	Input:			strBuf		字符串
	strChars	需要去除的字符
	*	Output:			strBuf		去除后的字符串
	*	Return:			去除后字符串的大小
	*/
	int StrTrim(std::string& strBuf, std::string strChars /* = " " */)
	{
		std::string::iterator				Iter;		// 正向迭代器
		std::string::reverse_iterator		reIter;		// 反向迭代器

		if (strBuf.empty())
		{
			return strBuf.size();
		}

		for (size_t sIndex = 0; sIndex < strBuf.size(); sIndex++)
		{
			Iter = strBuf.begin();

			if (NULL != strchr((char*)strChars.c_str(), *Iter))
			{
				strBuf.erase(Iter);
				continue;
			}

			break;
		}

		for (size_t sIndex = 0; sIndex < strBuf.size(); sIndex++)
		{
			reIter = strBuf.rbegin();

			if (NULL != strchr((char*)strChars.c_str(), *reIter))
			{
				strBuf.erase(strBuf.begin() + strBuf.size() - 1);
				continue;
			}

			break;
		}

		return strBuf.size();
	}









	/*
	 *	Function:		NSJsonObject
	 *	Explanation:	构造函数
	 */
	NSJsonObject::NSJsonObject()
	{

	}

	/*
	 *	Function:		~NSJsonObject
	 *	Explanation:	析构函数
	 */
	NSJsonObject::~NSJsonObject()
	{

	}

	/*
	 *	Function:		_GetValue
	 *	Explanation:	获取Json::Value对象
	 *	Return:			Json::Value对象
	 */
	Json::Value NSJsonObject::_GetValue()
	{
		return m_Obj;
	}

	/*
	 *	Function:		_SetValue
	 *	Explanation:	设置Json::Value
	 *	Input:			Value
	 *  Output:			输出
	 *	Return:			TRUE 成功 FALSE 失败
	 */
	void NSJsonObject::_SetValue(Json::Value Value)
	{
		m_Obj = Value;
	}

	/*
	 *	Function:		Put
	 *	Explanation:	添加数据
	 *	Input:			Key			键
						Value		值
	 */
	void NSJsonObject::Put(std::string Key, int Value, bool Isbool /* = false */)
	{
		if (!Isbool)
		{
			m_Obj[Key] = Value;
		}
		else
		{
			if (0 == Value)
			{
				m_Obj[Key] = false;
			}
			else
			{
				m_Obj[Key] = true;
			}
		}

	}

	void NSJsonObject::Put(std::string Key, unsigned int Value)
	{
		m_Obj[Key] = Value;
	}

	//void NSJsonObject::Put(std::string Key, int64_t Value)
	//{
	//	m_Obj[Key] = Value;
	//}
	//
	//void NSJsonObject::Put(std::string Key, unsigned int64_t Value)
	//{
	//	m_Obj[Key] = Value;
	//}

	void NSJsonObject::Put(std::string Key, double Value)
	{
		m_Obj[Key] = Value;
	}

	void NSJsonObject::Put(std::string Key, float Value)
	{
		m_Obj[Key] = Value;
	}

	void NSJsonObject::Put(std::string Key, std::string Value)
	{
		m_Obj[Key] = Value;
	}


	void NSJsonObject::Put(std::string Key, NSJsonObject JsonObj)
	{
		m_Obj[Key] = JsonObj._GetValue();
	}

	void NSJsonObject::Put(std::string Key, NSJsonArray JsonArr)
	{
		m_Obj[Key] = JsonArr._GetValue();
	}

	/*
	 *	Function:		IsInt
	 *	Explanation:	判断指定键的值的类型
	 *	Input:			Key			键
	 *	Return:			true 是 false 不是
	 */
	bool NSJsonObject::IsInt(std::string Key)
	{
		return m_Obj[Key].isInt();
	}

	bool NSJsonObject::IsUInt(std::string Key)
	{
		return m_Obj[Key].isUInt();
	}

	bool NSJsonObject::IsInt64(std::string Key)
	{
		return m_Obj[Key].isInt64();
	}

	bool NSJsonObject::IsUInt64(std::string Key)
	{
		return m_Obj[Key].isUInt64();
	}

	bool NSJsonObject::IsBool(std::string Key)
	{
		return m_Obj[Key].isBool();
	}

	bool NSJsonObject::IsDouble(std::string Key)
	{
		return m_Obj[Key].isDouble();
	}

	bool NSJsonObject::IsString(std::string Key)
	{
		return m_Obj[Key].isString();
	}

	bool NSJsonObject::IsObject(std::string Key)
	{
		return m_Obj[Key].isObject();
	}

	bool NSJsonObject::IsArray(std::string Key)
	{
		return m_Obj[Key].isArray();
	}

	/*
	 *	Function:		GetBool
	 *	Explanation:	获取值
	 *	Input:			Key			键
	 *	Return:			对应的值（）
	 */
	bool NSJsonObject::GetBool(std::string Key)
	{
		if (IsNull(Key) || m_Obj[Key].isBool() == false)
		{
			throw std::invalid_argument("key not exist");
			//_THROW(std::invalid_argument, "key not exist");
			return _GetDefaultBool(Key);
		}

		return m_Obj[Key].asBool();
	}

	int NSJsonObject::GetInt(std::string Key)
	{
		if (IsNull(Key) || m_Obj[Key].isInt() == false)
		{
			//_THROW(std::invalid_argument, "key not exist");
			return _GetDefaultInt(Key);
		}

		return m_Obj[Key].asInt();
	}

	unsigned int NSJsonObject::GetUInt(std::string Key)
	{
		if (IsNull(Key) || m_Obj[Key].isUInt() == false)
		{
			//_THROW(std::invalid_argument, "key not exist");
			return _GetDefaultInt(Key);
		}

		return m_Obj[Key].asUInt();
	}
	//
	//int64_t NSJsonObject::GetInt64(std::string Key)
	//{
	//	if (IsNull(Key))
	//	{
	//		//_THROW(std::invalid_argument, "key not exist");
	//		return _GetDefaultInt(Key);
	//	}
	//
	//	return m_Obj[Key].asInt64();
	//}
	//
	//unsigned int64_t NSJsonObject::GetUInt64(std::string Key)
	//{
	//	if (IsNull(Key))
	//	{
	//		//_THROW(std::invalid_argument, "key not exist");
	//		return _GetDefaultInt(Key);
	//	}
	//
	//	return m_Obj[Key].asUInt64();
	//}

	double NSJsonObject::GetDouble(std::string Key)
	{
		if (IsNull(Key) || m_Obj[Key].isDouble() == false)
		{
			//_THROW(std::invalid_argument, "key not exist");
			return _GetDefaultFloat(Key);
		}

		return m_Obj[Key].asDouble();
	}

	float NSJsonObject::GetFloat(std::string Key)
	{
		if (IsNull(Key))
		{
			//_THROW(std::invalid_argument, "key not exist");
			return _GetDefaultFloat(Key);
		}

		return m_Obj[Key].asFloat();
	}

	std::string NSJsonObject::GetString(std::string Key)
	{
		if (IsNull(Key) || m_Obj[Key].isString() == false)
		{
			//_THROW(std::invalid_argument, "key not exist");
			return _GetDefaultString(Key);
		}

		return m_Obj[Key].asString();
	}

	NSJsonObject NSJsonObject::Get(std::string Key)
	{
		NSJsonObject			Obj;

		if (IsNull(Key) || m_Obj[Key].isObject() == false)
		{
			//_THROW(std::invalid_argument, "key not exist");
			return _GetDefaultObject(Key);
		}

		Obj._SetValue(m_Obj[Key]);

		return Obj;
	}

	NSJsonArray NSJsonObject::GetArr(std::string Key)
	{
		NSJsonArray			Arr;

		if (IsNull(Key) || m_Obj[Key].isArray() == false)
		{
			//_THROW(std::invalid_argument, "key not exist");
			return _GetDefaultArray(Key);
		}

		Arr._SetValue(m_Obj[Key]);

		return Arr;
	}

	/*
	 *	Function:		OptGetBool
	 *	Explanation:	获取值（Key对应的键值可以不存在）
	 *	Input:			Key			键
						Default		默认值
	 *	Return:			值
	 */
	bool NSJsonObject::OptGetBool(std::string Key, bool Default /* = false */)
	{
		if (IsNull(Key) || m_Obj[Key].isBool() == false)
		{
			return Default;
		}

		return m_Obj[Key].asBool();
	}

	int NSJsonObject::OptGetInt(std::string Key, int Default /* = 0 */)
	{
		if (IsNull(Key) || m_Obj[Key].isInt() == false)
		{
			return Default;
		}

		return m_Obj[Key].asInt();
	}

	unsigned int NSJsonObject::OptGetUInt(std::string Key, unsigned int Default /* = 0 */)
	{
		if (IsNull(Key) || m_Obj[Key].isUInt() == false)
		{
			return Default;
		}

		return m_Obj[Key].asUInt();
	}
	//
	//int64_t NSJsonObject::OptGetInt64(std::string Key, int64_t Default /* = 0 */)
	//{
	//	if (IsNull(Key))
	//	{
	//		return Default;
	//	}
	//
	//	return m_Obj[Key].asInt64();
	//}
	//
	//unsigned int64_t NSJsonObject::OptGetUInt64(std::string Key, unsigned int64_t Default /* = 0 */)
	//{
	//	if (IsNull(Key))
	//	{
	//		return Default;
	//	}
	//
	//	return m_Obj[Key].asUInt64();
	//}

	double NSJsonObject::OptGetDouble(std::string Key, double Default /* = 0 */)
	{
		if (IsNull(Key) || m_Obj[Key].isDouble() == false)
		{
			return Default;
		}

		return m_Obj[Key].asDouble();
	}

	float NSJsonObject::OptGetFloat(std::string Key, float Default /* = 0 */)
	{
		if (IsNull(Key))
		{
			return Default;
		}

		return m_Obj[Key].asFloat();
	}

	std::string NSJsonObject::OptGetString(std::string Key, std::string Default /* = "" */)
	{
		if (IsNull(Key) || m_Obj[Key].isString() == false)
		{
			return Default;
		}

		return m_Obj[Key].asString();
	}

	NSJsonObject NSJsonObject::OptGet(std::string Key, NSJsonObject Default /* = NSJsonObj_NULL */)
	{
		NSJsonObject			Obj;

		if (IsNull(Key) || m_Obj[Key].isObject() == false)
		{
			return Default;
		}

		Obj._SetValue(m_Obj[Key]);

		return Obj;
	}

	/*
	*	Function:		IsNull
	*	Explanation:	判断指定键的值是否为空(若Name为空则判断当前对象)
	*	Input:			Key			键
	*	Return:			true 空 false 非空
	*/
	bool NSJsonObject::IsNull(std::string Key /* = "" */)
	{
		if (Key.empty())
		{
			return m_Obj.isNull();
		}
		else
		{
			return !m_Obj.isMember(Key);
		}
	}

	/*
	 *	Function:		GetKeys
	 *	Explanation:	获取所有Key
	 *	Return:			Key列表
	 */
	std::vector<std::string> NSJsonObject::GetKeys()
	{
		std::vector<std::string>		vecList;
		Json::Value::Members	memList;

		memList = m_Obj.getMemberNames();

		for (size_t stIndex = 0; stIndex < memList.size(); stIndex++)
		{
			vecList.push_back(memList[stIndex]);
		}

		return vecList;
	}

	/*
	 *	Function:		Remove
	 *	Explanation:	移除键值对
	 *	Input:			Key			键
	 */
	void NSJsonObject::Remove(std::string Key)
	{
		if (IsNull(Key))
		{
			return;
		}

		m_Obj.removeMember(Key);
	}

	/*
	 *	Function:		Size
	 *	Explanation:	获取键值对总数
	 *	Return:			键值对总数
	 */
	unsigned int NSJsonObject::Size()
	{
		return m_Obj.size();
	}

	/*
	 *	Function:		Compare
	 *	Explanation:	比较两个Obj
	 *	Input:			输入
	 *  Output:			输出
	 *	Return:			TRUE 成功 FALSE 失败
	 */
	int NSJsonObject::Compare(NSJsonObject JsonObj)
	{
		return m_Obj.compare(JsonObj._GetValue());
	}

	/*
	 *	Function:		Clear
	 *	Explanation:	清空
	 */
	void NSJsonObject::Clear()
	{
		m_Obj.clear();
	}

	/*
	 *	Function:		ToString
	 *	Explanation:	获取Json格式字符串
	 *	Return:			Json格式字符串
	 */
	std::string NSJsonObject::ToString(bool bStyled /* = true */)
	{
		if (bStyled)
		{
			return m_Obj.toStyledString();
		}

		Json::StreamWriterBuilder wbuilder;
		std::string jsonStr = Json::writeString(wbuilder, m_Obj);
		StrTrim(jsonStr, "\n");
		return jsonStr;
	}

	/*
	 *	Function:		ToArray
	 *	Explanation:	转换成NSJsonArray
	 *	Return:			NSJsonArray
	 */
	NSJsonArray NSJsonObject::ToArray()
	{
		NSJsonArray			JsonArr;

		JsonArr._SetValue(m_Obj);

		return JsonArr;
	}

	bool NSJsonObject::_GetDefaultBool(std::string key)
	{
		//GSLOG_WARN("key %s not exist", key.c_str());
		return false;
	}

	int NSJsonObject::_GetDefaultInt(std::string key)
	{
		//GSLOG_WARN("key %s not exist", key.c_str());
		return 0;
	}

	float NSJsonObject::_GetDefaultFloat(std::string key)
	{
		//GSLOG_WARN("key %s not exist", key.c_str());
		return 0.0;
	}

	std::string NSJsonObject::_GetDefaultString(std::string key)
	{
		//GSLOG_WARN("key %s not exist", key.c_str());
		return "";
	}

	NSJsonObject NSJsonObject::_GetDefaultObject(std::string key)
	{
		//GSLOG_WARN("key %s not exist", key.c_str());
		return NSJsonObject();
	}

	NSJsonArray NSJsonObject::_GetDefaultArray(std::string key)
	{
		//GSLOG_WARN("key %s not exist", key.c_str());
		return NSJsonArray();
	}





	/*
	 *	Function:		NSJsonArray
	 *	Explanation:	构造函数
	 */
	NSJsonArray::NSJsonArray() : m_Obj(Json::arrayValue)
	{
	}

	NSJsonArray::NSJsonArray(const std::vector<int>& vectList)
	{
		for (auto item : vectList)
		{
			m_Obj.append(item);
		}
	}

	NSJsonArray::NSJsonArray(const std::vector<std::string>& vectList)
	{
		for (auto item : vectList)
		{
			m_Obj.append(item);
		}
	}

	/*
	 *	Function:		~NSJsonArray
	 *	Explanation:	析构函数
	 */
	NSJsonArray::~NSJsonArray()
	{

	}

	/*
	*	Function:		_GetValue
	*	Explanation:	获取Json::Value对象
	*	Return:			Json::Value对象
	*/
	Json::Value NSJsonArray::_GetValue()
	{
		return m_Obj;
	}

	/*
	*	Function:		_SetValue
	*	Explanation:	设置Json::Value
	*	Input:			Value
	*  Output:			输出
	*	Return:			TRUE 成功 FALSE 失败
	*/
	void NSJsonArray::_SetValue(Json::Value Value)
	{
		m_Obj = Value;
	}

	/*
	 *	Function:		IsNull
	 *	Explanation:	是否为空对象
	 *	Return:			true 是 false 不是
	 */
	bool NSJsonArray::IsNull()
	{
		return m_Obj.isNull();
	}

	/*
	 *	Function:		Size
	 *	Explanation:	数组个数
	 *	Return:			数组个数
	 */
	unsigned int NSJsonArray::Size()
	{
		return m_Obj.size();
	}

	/*
	 *	Function:		Get
	 *	Explanation:	获取NSJsonObject
	 *	Input:			unIndex		索引
	 *	Return:			NSJsonObject
	 */
	NSJsonObject NSJsonArray::Get(unsigned int unIndex)
	{
		NSJsonObject			JsonObj;

		if (!m_Obj.isValidIndex(unIndex))
		{
			throw std::invalid_argument("beyond the index");
			//_THROW(std::invalid_argument, "beyond the index");
		}
		else
		{
			JsonObj._SetValue(m_Obj[unIndex]);
		}

		return JsonObj;
	}

	std::string NSJsonArray::GetString(unsigned int unIndex)
	{
		std::string				strValue;

		if (!m_Obj.isValidIndex(unIndex))
		{
			throw std::invalid_argument("beyond the index");
			//_THROW(std::invalid_argument, "beyond the index");
		}
		else
		{
			strValue = m_Obj[unIndex].asString();
		}

		return strValue;
	}

	int NSJsonArray::GetInt(unsigned int unIndex)
	{
		int						nValue;

		if (!m_Obj.isValidIndex(unIndex))
		{
			throw std::invalid_argument("beyond the index");
			//_THROW(std::invalid_argument, "beyond the index");
		}
		else
		{
			nValue = m_Obj[unIndex].asInt();
		}

		return nValue;
	}

	bool NSJsonArray::GetBool(unsigned int unIndex)
	{
		bool					bValue;

		if (!m_Obj.isValidIndex(unIndex))
		{
			throw std::invalid_argument("beyond the index");
			//_THROW(std::invalid_argument, "beyond the index");
		}
		else
		{
			bValue = m_Obj[unIndex].asBool();
		}

		return bValue;
	}

	/*
	 *	Function:		Add
	 *	Explanation:	添加
	 *	Input:			JsonObj		json对象
	 */
	void NSJsonArray::Add(NSJsonObject JsonObj)
	{
		if (!m_Obj.isArray())
		{
			return;
		}

		m_Obj.append(JsonObj._GetValue());
	}

	void NSJsonArray::AddInt(int nValue)
	{
		m_Obj.append(nValue);
	}

	void NSJsonArray::AddString(std::string strValue)
	{
		m_Obj.append(strValue);
	}

	/*
	 *	Function:		Remove
	 *	Explanation:	移除
	 *	Input:			unIndex		索引
	 */
	void NSJsonArray::Remove(unsigned int unIndex)
	{
		m_Obj.removeIndex(unIndex, NULL);
	}

	/*
	 *	Function:		Clear
	 *	Explanation:	清空
	 */
	void NSJsonArray::Clear()
	{
		m_Obj.clear();
	}

	/*
	 *	Function:		ToObject
	 *	Explanation:	转换成NSJsonObject
	 *	Return:			NSJsonObject
	 */
	NSJsonObject NSJsonArray::ToObject()
	{
		NSJsonObject			JsonObj;

		JsonObj._SetValue(m_Obj);

		return JsonObj;
	}

	/*
	 *	Function:		ToString
	 *	Explanation:	获取Json格式字符串
	 *	Return:			Json格式字符串
	 */
	std::string NSJsonArray::ToString(bool bStyled /* = true */)
	{
		if (bStyled)
		{
			return m_Obj.toStyledString();
		}
		Json::StreamWriterBuilder wbuilder;
		std::string out = Json::writeString(wbuilder, m_Obj);
		StrTrim(out, "\n");
		return out;
	}



	/*
	 *	Function:		NSJson
	 *	Explanation:	构造函数
	 */
	NSJson::NSJson()
	{

	}

	/*
	 *	Function:		~NSJson
	 *	Explanation:	析构函数
	 */
	NSJson::~NSJson()
	{

	}

	/*
	 *	Function:		ParseStr
	 *	Explanation:	解析Json格式字符串
	 *	Input:			strJson
	 *	Return:			NSJsonObject
	 */
	NSJsonObject NSJson::ParseStr(const std::string& strJson)
	{
		NSJsonObject			JsonObj;
		Json::CharReaderBuilder builder;
		Json::Value				jsonItem;

		if (strJson.empty())
		{
			return JsonObj;
		}
		//builder["collectComments"] = false;
		JSONCPP_STRING errs;
		std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
		if (!reader->parse(strJson.data(), strJson.data() + strJson.size(), &jsonItem, &errs)) //从jsonStr中读取数据到jsonRoot
		{
			return  JsonObj;
		}
		JsonObj._SetValue(jsonItem);

		return JsonObj;
	}

	/*
	 *	Function:		ParaseFile
	 *	Explanation:	读取文件，解析json
	 *	Input:			strFile		文件路径
	 *	Return:			NSJsonObject
	 */
	NSJsonObject NSJson::ParaseFile(const std::string& strFile)
	{
		NSJsonObject			JsonObj;
		Json::Value				jsonItem;
		std::ifstream			Stream; //标准输入流


		Stream.open((char*)strFile.c_str(), std::ios::binary);
		if (false == Stream.is_open())
		{
			return JsonObj;
		}

		jsonItem.clear();

		Json::CharReaderBuilder builder;
		builder["collectComments"] = false;
		JSONCPP_STRING errs;
		if (!parseFromStream(builder, Stream, &jsonItem, &errs)) //从ifs中读取数据到jsonRoot
		{
			JsonObj._SetValue(jsonItem);
			Stream.close();
			return JsonObj;
		}
		JsonObj._SetValue(jsonItem);
		Stream.close();
		return JsonObj;
	}



	/*
	 *	Function:		SaveToFile
	 *	Explanation:	保存到文件
	 *	Input:			strFile		文件路径
						JsonObj		Json对象
	 *	Return:			true 成功 false 失败
	 */
	bool NSJson::SaveToFile(const std::string& strFile, NSJsonObject JsonObj)
	{
		std::ofstream Stream(strFile, std::ios::binary | std::ios::trunc);

		if (!Stream.is_open()) {
			return false;
		}

		std::string strJson = JsonObj.ToString();
		Stream.write(strJson.c_str(), strJson.size());
		Stream.close();
		return true;
	}
}