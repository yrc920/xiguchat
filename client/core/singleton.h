#ifndef SINGLETON_H
#define SINGLETON_H

#include <memory>
#include <mutex>
/******************************************************************************
 *
 * @file       singleton.h
 * @brief      单例的模板类
 *
 * @author     西故
 * @date       2026/04/20
 * @history
 *****************************************************************************/

template <typename T>
class Singleton{
protected:
    Singleton() = default; //子类继承时可以构造基类
    //禁止拷贝构造和赋值操作
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator = (const Singleton<T>& st) = delete;
    static std::shared_ptr<T> _instance; //单例类的唯一实例

public:
    //返回单例实例的静态方法
    static std::shared_ptr<T> GetInstance(){
        static std::once_flag s_flag; //保证只执行一次的标志
        std::call_once(s_flag, [&](){
            _instance = std::shared_ptr<T>(new T); //创建单例实例
        });
        return _instance;
    }

    ~Singleton() = default;
};

//初始化静态成员变量
template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;

#endif // SINGLETON_H
