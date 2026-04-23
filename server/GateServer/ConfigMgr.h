#pragma once
// 配置管理类, 用于读取和存储配置文件中的数据
//
#include "const.h"

//用于存储每个section中的key-value对
struct SectionInfo {
	SectionInfo() {}
	//拷贝构造函数和赋值操作符, 用于正确复制_section_datas
	SectionInfo(const SectionInfo& src) { _section_datas = src._section_datas; }
	SectionInfo& operator = (const SectionInfo& src) {
		if (&src == this)
			return *this;
		this->_section_datas = src._section_datas;
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

//用于管理配置文件中的所有section和对应的key-value对
class ConfigMgr
{
public:
	ConfigMgr();
	//拷贝构造函数和赋值操作符, 用于正确复制_config_map
	ConfigMgr& operator=(const ConfigMgr& src) {
		if (&src == this)
			return *this;

		this->_config_map = src._config_map;
	};
	ConfigMgr(const ConfigMgr& src) { this->_config_map = src._config_map; }
	~ConfigMgr() { _config_map.clear(); }
	
	//重载[]运算符, 通过section获取对应的SectionInfo对象, 如果section不存在则返回一个空的SectionInfo对象
	SectionInfo operator[](const std::string& section) {
		if (_config_map.find(section) == _config_map.end()) {
			return SectionInfo();
		}
		return _config_map[section];
	}
private:
	// 存储section和key-value对的map  
	std::map<std::string, SectionInfo> _config_map;
};
