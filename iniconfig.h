#ifndef SHBK_COMMON_INICONFIG_H_
#define SHBK_COMMON_INICONFIG_H_

#include <string>
#include "configdef.h"

class Iniconfig
{
protected:
		Iniconfig();
	public:
		~Iniconfig();
		bool loadfile(const std::string& path);
		const st_env_config& getconfig();
		static Iniconfig* getInstance();
		
	private:
		st_env_config _config;
		bool _isloaded;
		static Iniconfig* config_;
		
};


#endif // SHBK_COMMON_INICONFIG_H_
