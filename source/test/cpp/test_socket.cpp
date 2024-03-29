#include "xbase\x_target.h"
#include "xsocket\x_socket.h"

#include "xunittest\xunittest.h"

using namespace ccore;

extern ccore::x_iallocator* gTestAllocator;

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
			s->open(3823, sname, sid, 32);
			s->close();
			delete s;
		}
	}
}
UNITTEST_SUITE_END