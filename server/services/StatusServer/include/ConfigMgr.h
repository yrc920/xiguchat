#pragma once
// 配置管理类, 用于读取和存储配置文件中的数据
//

#include "const.h"
struct SectionInfo; //前置声明

//用于管理配置文件中的所有section和对应的key-value对
class ConfigMgr
{
public:
	~ConfigMgr();
	//禁止拷贝构造和赋值操作
	ConfigMgr(const ConfigMgr&) = delete;
	ConfigMgr& operator=(const ConfigMgr&) = delete;
	
	//获取配置管理器实例的静态方法, 使用局部静态变量来实现单例模式, 确保全局只有一个配置管理器实例
	static ConfigMgr& Inst() {
		static ConfigMgr cfg_mgr;
		return cfg_mgr;
	}
	
	SectionInfo operator[](const std::string& section); //重载[]运算符

private:
	ConfigMgr();

	// 存储section和key-value对的map  
	std::map<std::string, SectionInfo> _config_map;
};

//用于存储每个section中的key-value对
struct SectionInfo {
	SectionInfo() {}
	//拷贝构造函数和赋值操作符, 用于正确复制_section_datas
	SectionInfo(const SectionInfo& src) { _section_datas = src._section_datas; }
	SectionInfo& operator = (const SectionInfo& src) {
		if (&src == this)
			return *this;
		this->_section_datas = src._section_datas;
		return *this;
	}
	~SectionInfo() { _section_datas.clear(); }

	//重载[]运算符, 通过key获取对应的value, 如果key不存在则返回空字符串
	std::string  operator[](const std::string& key) {
		if (_section_datas.find(key) == _section_datas.end()) {
			return "";
		} 
		return _section_datas[key];
	}

	std::map<std::string, std::string> _section_datas; //存储section中的key-value对
};
