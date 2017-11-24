#include "xbase\x_target.h"
#include "xsocket\x_socket.h"

#include "xunittest\xunittest.h"

using namespace xcore;

extern xcore::x_iallocator* gTestAllocator;

UNITTEST_SUITE_BEGIN(xaddress)
{
	UNITTEST_FIXTURE(main)
	{
		UNITTEST_FIXTURE_SETUP() {}
		UNITTEST_FIXTURE_TEARDOWN() {}

		UNITTEST_TEST(new_socket)
		{
			xsocket* s = gCreateTcpBasedSocket(gTestAllocator);
			const char* sname = "Jurgen/CNSHAW1334/10.0.22.76:3823/virtuosgames.com";
			xsockid sid;
			sid.generate_from(sname);
			s->open(3823, sname, sid, 32);
		}
	}
}
UNITTEST_SUITE_END