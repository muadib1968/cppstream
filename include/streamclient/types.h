#ifndef _HBM__STREAMING__TYPES
#define _HBM__STREAMING__TYPES

#include <stdint.h>
#include <vector>
#include <json/value.h>

namespace hbm {
	static const char PARAMS[] = "params";
	static const char METHOD[] = "method";



	namespace streaming {
		typedef std::vector < std::string > signalReferences_t;

		static const std::string DAQSTREAM_PORT = "7411";

		static const char META_METHOD_AVAILABLE[] = "available";
		static const char META_METHOD_UNAVAILABLE[] = "unavailable";
		static const char META_METHOD_ALIVE[] = "alive";
		static const char META_METHOD_FILL[] = "fill";


		class timeInfo {
		public:
			timeInfo();
			timeInfo(uint64_t ntpTimeStamp);
			uint64_t ntpTimeStamp() const;
			uint32_t seconds() const;
			uint32_t fractions() const;
			void set(const Json::Value& StampNode);
			void setNtpTimestamp(uint64_t ntpTimeStamp);
			void clear();
			void increment(const timeInfo& op);

		private:
			/// the upper 32 byte are seconds
			/// the lower 32 bytes are fractions of seconds (0.5, 0.25, 0.125...)
			uint64_t m_ntpTimestamp;
		};
	}
}
#endif
