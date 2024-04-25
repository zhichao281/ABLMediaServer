#pragma once

#include <unordered_set>
#include <string.h>

//对象池模板类
template<class T>
class class_obj_pool
{
public:
	enum
	{
		MAX_INIT_NUM = 100000,	// 初始化最大数量，暂定10万
	};

	class_obj_pool()
	{
		clear();
	}

	~class_obj_pool()
	{
		clear();
	}
public:
	// 初始化(初始数量，一次增长的数量， 单次收缩的数量)
	bool init(int initNum = 200, int increaseNum = 100, int reduceNum = 50)
	{
		// 参数检测
		if (initNum <= 0 || increaseNum <= 0 || reduceNum <= 0)
		{
			printf("init(), param error, initNum=%d, nIncreaseNum=%d, reduceNum=%d\n", initNum, increaseNum, reduceNum);
			return false;
		}

		nInitNum = initNum;
		nIncreaseNum = increaseNum;
		nShrinkNum = reduceNum;

		// 参数修正
		nInitNum = nInitNum > MAX_INIT_NUM ? MAX_INIT_NUM : nInitNum;			// 初始个数
		nIncreaseNum = nIncreaseNum > MAX_INIT_NUM ? MAX_INIT_NUM : nIncreaseNum;	// 一次增长的数量

		// 初始化对象
		for (int i = 0; i < nInitNum; ++i)
		{
			T* pObj = new(std::nothrow)T();
			if (pObj == NULL)
			{
				printf("new obj failed \n");
				continue;
			}

			// T对象需提供init函数进行初始化
			if (pObj->init() == false)
			{
				printf("pObj->init() failed \n");
				delete pObj;
				continue;
			}

			free_list.insert(pObj);
		}

		return true;
	}

	// 分配一个新对象
	T* alloc()
	{
		T* pRet = NULL;

		// 空闲列表中有，则取一个
		if (free_list.size() > 0)
		{
			auto iter = free_list.begin();
			pRet = *iter;

			free_list.erase(iter);
			used_list.insert(pRet);

			// 超过最大使用记录量，则更新
			if (used_list.size() > nUsedMax)
			{
				nUsedMax = used_list.size();
			}

			return pRet;
		}

		// 空闲列表中没有时，则新建一批
		for (int i = 0; i < nIncreaseNum; i++)
		{
			T* pObj = new(std::nothrow)T();
			if (pObj == NULL)
			{
				printf("alloc(), new obj failed \n");
				continue;
			}

			// T对象需提供init函数进行初始化
			if (pObj->init() == false)
			{
				printf("pObj->init() failed \n");
				delete pObj;
				continue;
			}

			// 没赋值的先赋值；已赋值的压入空闲列表
			if (pRet == NULL)
			{
				pRet = pObj;
				used_list.insert(pObj);
			}
			else
			{
				free_list.insert(pObj);
			}
		}

		// 超过最大使用记录量，则更新
		if (used_list.size() > nUsedMax)
		{
			nUsedMax = used_list.size();
		}

		return pRet;
	}

	// 回收一个对象
	void dealloc(T* pObj)
	{
		if (pObj == NULL)
		{
			printf("dealloc(), pObj == NULL");
			return;
		}

		// 不存在
		auto it = used_list.find(pObj);
		if (it == used_list.end())
		{
			printf("dealloc(), find pObj failed \n");
			return;
		}

		// T对象需提供reset()函数进行重置
		pObj->reset();

		free_list.insert(pObj);
		used_list.erase(pObj);
	}

	// 执行一次回收，用来使用高峰过后的缩减
	void dorecycle()
	{
		// 空闲列表中有对象，且超过一次增量时，才进行回收
		int nfree = free_list.size();
		if (nfree <= nIncreaseNum)
		{
			return;
		}

		// 要回收的数量
		nfree = (nfree - nIncreaseNum);
		int i = 0;
		for (auto iter : free_list )
		{
			T* pObj = iter;
			if (pObj != NULL)
			{
				delete pObj;
			}

			free_list.erase(iter++);

			// 超过一次回收的最大量，就跳出
			i++;
			if (i >= nShrinkNum)
			{
				break;
			}
		}
	}

	// 打印出各个变量
	void showinfo()
	{
		printf("free_list.size=%d, used_list.size=%d, nUsedMax=%d \n", free_list.size(), used_list.size(), nUsedMax);
	}

	// 回收
	void clear()
	{
		for (auto iter: free_list)
		{
			T* pObj = iter;
			if (pObj != NULL)
			{
				delete pObj;
			}
		}
		free_list.clear();
		for (auto iter : free_list)		
		{
			T* pObj = iter;
			if (pObj != NULL)
			{
				delete pObj;
			}
		}
		used_list.clear();

		nInitNum = 0;	// 初始个数
		nUsedMax = 0;	// 曾经使用的最大个数
		nIncreaseNum = 0;	// 一次增长的数量
		nShrinkNum = 0;	// 单次回收的最大数量
	}
private:
	std::unordered_set<T*> 	free_list;		// 空闲列表
	std::unordered_set<T*> 	used_list;		// 使用中的列表
	int 					nInitNum;		// 初始个数
	int 					nUsedMax;		// 曾经使用的最大个数
	int 					nIncreaseNum;	// 一次增长的数量
	int 					nShrinkNum;		// 单次回收的最大数量
};