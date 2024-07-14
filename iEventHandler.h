#ifndef NS_IEVENT_HANDLER_H_
#define NS_IEVENT_HANDLER_H_

#include "ievent.h"
#include "eventtype.h"

class iEventHandler
{
public:
	virtual ~iEventHandler() {};
	virtual iEvent* handle(const iEvent* ev) { return NULL; }
	iEventHandler(const char* name) :name_(name) {}
	std::string& get_name() { return name_; }

private:
	std::string name_;
};

#endif // !NS_IEVENT_HANDLER_H_
