#include "events_def.h"
#include <iostream>
#include <sstream>
#include <string.h>

std::ostream& MobileCodeReqEv::dump(std::ostream& out) const {

	out << "MobileCodeReq sn =" << get_sn() << ",";
	out << "mobile... =" << (std::string *)get_eid() << ",";
	out << "mobile = " << msg_.mobile() << ",";
	out << "eid = " << get_eid() << std::endl;

	return out;
}

std::ostream& MobileCodeRspEv::dump(std::ostream& out) const {

	out << "MobileCodeRsp sn =" << get_sn() << ",";
	out << "code = " << msg_.code() << ",";
	out << "icode = " << msg_.icode() << ",";
	out << "describle = " << msg_.data() << ",";
	out << "eid = " << get_eid() << std::endl;

	return out;
}

std::ostream& LoginReqEv::dump(std::ostream& out) const {

	out << "MobileCodeRsp sn =" << get_sn() << ",";
	out << "icode = " << msg_.icode() << ",";
	out << "mobile = " << msg_.mobile() << ",";
	out << "eid = " << get_eid() << std::endl;

	return out;
}

std::ostream& LoginResEv::dump(std::ostream& out) const {

	out << "MobileCodeRsp sn =" << get_sn() << ",";
	out << "desc = " << msg_.desc() << ",";
	out << "code = " << msg_.code() << ",";
	out << "eid = " << get_eid() << std::endl;

	return out;
}