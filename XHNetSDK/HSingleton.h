#pragma once

#include <iostream>
class singleton {
public:
	// 获取单实例对象
	static singleton& getInstance() {
		static singleton instance;
		return instance;
	}
private:
	// 禁止外部构造
	singleton() = default;
	// 禁止外部析构
	~singleton() = default;
	// 禁止外部复制构造
	singleton(const singleton&) = delete;
	// 禁止外部赋值操作
	singleton& operator=(const singleton&) = delete;
};



template<typename T>
class Singleton {
public:
	static T& getInstance() {
		static T instance;
		return instance;
	}

	virtual ~Singleton() {
		std::cout << "destructor called!" << std::endl;
	}

	Singleton(const Singleton&) = delete;
	Singleton& operator =(const Singleton&) = delete;

protected:
	Singleton() {
		std::cout << "constructor called!" << std::endl;
	}
};

/********************************************/
// Example:
// 1.friend class declaration is requiered!
// 2.constructor should be private

class DerivedSingle : public Singleton<DerivedSingle> {
	// !!!! attention!!!
	// needs to be friend in order to
	// access the private constructor/destructor
	friend class Singleton<DerivedSingle>;

public:
	DerivedSingle(const DerivedSingle&) = delete;
	DerivedSingle& operator =(const DerivedSingle&) = delete;

private:
	DerivedSingle() = default;
};

//int testmain(int argc, char* argv[]) {
//	DerivedSingle& instance1 = DerivedSingle::getInstance();
//	DerivedSingle& instance2 = DerivedSingle::getInstance();
//	return 0;
//}
