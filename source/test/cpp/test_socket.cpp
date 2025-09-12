#include "ccore/c_target.h"
#include "csocket/c_socket.h"

#include "cunittest/cunittest.h"

using namespace ncore;

UNITTEST_SUITE_BEGIN(address_t)
{
	UNITTEST_FIXTURE(main)
	{
		UNITTEST_FIXTURE_SETUP() {}
		UNITTEST_FIXTURE_TEARDOWN() {}

        UNITTEST_ALLOCATOR;

		UNITTEST_TEST(new_socket)
		{
			socket_t* s = gCreateTcpBasedSocket(Allocator);
			crunes_t sname = make_crunes("Jurgen/CNSHAW1334/10.0.22.76:3823/virtuosgames.com");
			sockid_t sid;
			s->open(3823, sname, sid, 32);
			s->close();
			gDestroyTcpBasedSocket(s);
		}
	}
}
UNITTEST_SUITE_END
