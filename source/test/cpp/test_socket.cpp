#include "xbase\x_target.h"
#include "xhash\x_crc.h"

#include "xunittest\xunittest.h"

using namespace xcore;

UNITTEST_SUITE_BEGIN(xcrc)
{
	UNITTEST_FIXTURE(main)
	{
		UNITTEST_FIXTURE_SETUP() {}
		UNITTEST_FIXTURE_TEARDOWN() {}

		UNITTEST_TEST(CRC_CRC32)
		{
			s32 buff[100]={1,2,3,4,5,6,7,8,9};
			s32 buff2[100]={1,1,2,3,5,8,13,21};
			s32 len=100;
			CHECK_NOT_EQUAL(xcrc::crc32(buff,len,1321),xcrc::crc32(buff,len,654321));
			CHECK_NOT_EQUAL(xcrc::crc32(buff,254,110),xcrc::crc32(buff,25,110));
			CHECK_EQUAL(xcrc::crc32(buff,len,0),xcrc::crc32(buff,len));
			CHECK_NOT_EQUAL(xcrc::crc32(buff,len,1321),xcrc::crc32(buff2,len,654321));
		}
		UNITTEST_TEST(CRC_Adler16)
		{
			s32 buff[100]={1,2,3,4,5,6,7,8,9};
			s32 buff2[100]={1,1,2,3,5,8,13,21};
			s32 len=100;
			CHECK_EQUAL(xcrc::adler16(buff,len,1),xcrc::adler16(buff,len));
			CHECK_NOT_EQUAL(xcrc::adler16(buff,len,110),xcrc::adler16(buff,len,5));
			CHECK_NOT_EQUAL(xcrc::adler16(buff,len,110),xcrc::adler16(buff2,len,110));
			CHECK_NOT_EQUAL(xcrc::adler16(buff,len,110),xcrc::adler16(buff,50,110));
			CHECK_EQUAL(xcrc::adler16(buff,0,358),((358& 0xFF)|((358>> 8)<<8)));
		}
		UNITTEST_TEST(CRC_Adler32)
		{
			s32 buff[100]={1,2,3,51,54,654684,985};
			s32 buff2[100]={1,1,2,3,5,8,13,21};
			s32 len=100;
			CHECK_EQUAL(xcrc::adler32(buff,len,1),xcrc::adler32(buff,len));
			CHECK_NOT_EQUAL(xcrc::adler32(buff,len,110),xcrc::adler32(buff,len,5));
			CHECK_NOT_EQUAL(xcrc::adler32(buff,len,123456),xcrc::adler32(buff2,len,123456));
			CHECK_NOT_EQUAL(xcrc::adler32(buff,len,110),xcrc::adler32(buff,50,110));
			CHECK_EQUAL(xcrc::adler32(buff,0,112358),((112358& 0xFFFF)|((112358>> 16)<<16)));
		}
	}
}
UNITTEST_SUITE_END