#ifndef NS_EVENT_H_
#define NS_EVENT_H_

#include "glo_def.h"
#include <string.h>
#include <iostream>
#include "eventtype.h"

class iEvent
{
public:
	iEvent(u32 eid, u32 sn);

	virtual ~iEvent();
	virtual std::ostream& dump(std::ostream& out = std::cout) const { return out; }
	virtual i32 ByteSize() { return 0; }
	virtual bool SerializeToArray(char* buf, int len) { return true; }

	u32 generateSeqNo();
	u32 get_eid() const { return eid_; }
	u32 get_sn() const { return sn_; }

	void set_eid(u32 eid) { eid_ = eid; }

	void set_args(void* args) { args_ = args; }
	void* get_args() { return args_; }

private:
	void* args_;
	u32 eid_;
	u32 sn_;
};




#endif // !NS_EVENT_H_

