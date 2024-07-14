# -..
项目描述: 基于Linux平台开发实现共享单车的用户管理核心功能、采用Libevent实现事件驱动架构，其中使用Protobuf进行数据序列化和反序列化实现网络数据传输的存储，在用户配置文件的处理采用Iniparser解析使用，使用Log4cpp进行调试打印。在实现过程中，将网络分发处理事件、用户事件请求以及公共模块进行封装，采用了单例、发布订阅者设计模式优化代码，使用了Nginx线程池将用户事件分发处理。