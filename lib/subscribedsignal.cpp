#include <cstring>
#include <stdint.h>
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include "json/writer.h"
#else
#include <jsoncpp/json/writer.h>
#endif

#include "signalextract.h"
#include "subscribedsignal.h"

namespace hbm {
	namespace streaming {

		// we use this as target for all values. Otherwise the compiler might optimize away a lot of functionality!
		static double sum = 0;

		SubscribedSignal::SubscribedSignal()
			: m_signalReference()
			, m_startTime()

			, m_signalRateSamples(0)
			, m_signalRateSamplesDelta()
			, m_signalRateDelta()

			, m_dataFormatPattern()
			, m_dataIsBigEndian(false)
			, m_dataValueType()
			, m_dataValueSize(0)
			, m_dataTimeType()
			, m_dataTimeSize(0)

			, m_valueCount(0)
		{
		}

		void SubscribedSignal::interpretValues(unsigned char *pData, size_t count)
		{
			if(m_dataIsBigEndian) {
				switch (m_dataValueType) {
					case DATATYPE_REAL32:
						for(size_t i=0; i<count; ++i) {
							sum += hbm::streaming::extract<float, hbm::streaming::big>()(&pData);
						}
						break;

					case DATATYPE_REAL64:
						for(size_t i=0; i<count; ++i) {
							sum += hbm::streaming::extract<double, hbm::streaming::big>()(&pData);
						}
						break;

					case DATATYPE_U32:
						for(size_t i=0; i<count; ++i) {
							sum += hbm::streaming::extract<uint32_t, hbm::streaming::big>()(&pData);
						}
						break;

					case DATATYPE_S32:
						for(size_t i=0; i<count; ++i) {
							sum += hbm::streaming::extract<int32_t, hbm::streaming::big>()(&pData);
						}
						break;

					case DATATYPE_U64:
						for(size_t i=0; i<count; ++i) {
							sum += hbm::streaming::extract<uint64_t, hbm::streaming::big>()(&pData);
						}
						break;

					case DATATYPE_S64:
						for(size_t i=0; i<count; ++i) {
							sum += hbm::streaming::extract<int64_t, hbm::streaming::big>()(&pData);
						}
						break;

					default:
						throw std::runtime_error("datatype not supported!");
						break;
				}
			} else {
				switch (m_dataValueType) {
					case DATATYPE_REAL32:
						for(size_t i=0; i<count; ++i) {
							sum += hbm::streaming::extract<float, hbm::streaming::little>()(&pData);
						}
						break;

					case DATATYPE_REAL64:
						for(size_t i=0; i<count; ++i) {
							sum += hbm::streaming::extract<double, hbm::streaming::little>()(&pData);
						}
						break;

					case DATATYPE_U32:
						for(size_t i=0; i<count; ++i) {
							sum += hbm::streaming::extract<uint32_t, hbm::streaming::little>()(&pData);
						}
						break;

					case DATATYPE_S32:
						for(size_t i=0; i<count; ++i) {
							sum += hbm::streaming::extract<int32_t, hbm::streaming::little>()(&pData);
						}
						break;

					case DATATYPE_U64:
						for(size_t i=0; i<count; ++i) {
							sum += hbm::streaming::extract<uint64_t, hbm::streaming::little>()(&pData);
						}
						break;

					case DATATYPE_S64:
						for(size_t i=0; i<count; ++i) {
							sum += hbm::streaming::extract<int64_t, hbm::streaming::little>()(&pData);
						}
						break;

					default:
						throw std::runtime_error("datatype not supported!");
						break;
				}
			}
		}

		void SubscribedSignal::interpreteTimestamp(unsigned char* pData)
		{
			if(m_dataTimeType == TIMETYPE_NTP) {
				uint64_t ntpTimestamp;
				if(m_dataIsBigEndian) {
					ntpTimestamp = be64toh(*reinterpret_cast < uint64_t* > (pData));
				} else {
					ntpTimestamp = le64toh(*reinterpret_cast < uint64_t* > (pData));
				}
			}
		}

		void SubscribedSignal::calculateTimestamp()
		{
			timeInfo_t delta(m_signalRateDelta.ntpTimeStamp()*m_valueCount);
			m_startTime.increment(delta);
			timeInfo_t time = m_startTime;
		}




		void SubscribedSignal::processData(unsigned char* pData, size_t size)
		{
			switch(m_dataFormatPattern) {
				case PATTERN_V:
					{
						size_t valueCount = size / m_dataValueSize;
						calculateTimestamp();
						interpretValues(pData, valueCount);
						m_valueCount += valueCount;
					}
					break;
				case PATTERN_TV:
					{
						// 1 time stamp, 1 value
						size_t tupleSize = m_dataTimeSize+m_dataValueSize;
						while(size>=tupleSize) {
							interpreteTimestamp(pData);
							pData += m_dataTimeSize;
							interpretValues(pData, 1);
							pData += m_dataValueSize;
							size -= tupleSize;
							++m_valueCount;
						}
					}
					break;
				case PATTERN_TB:
					// 1 time stamp n values
					if(size>=m_dataTimeSize+m_dataValueSize) {
						size_t valueCount = (size-m_dataTimeSize) / m_dataValueSize;
						interpreteTimestamp(pData);
						interpretValues(pData+m_dataTimeSize, valueCount);
						m_valueCount += valueCount;
					}
					break;
			}
		}

		void SubscribedSignal::processSignalMetaInformation(const std::string& method, const Json::Value& params)
		{
			if(method=="subscribe") {
				/// this is the first signal related meta information to arrive!
				if(params.empty()==false) {
					m_signalReference = params[0u].asString();
				}
			} else if(method=="time") {
				m_startTime.set(params);
			} else if(method=="data") {
				setDataFormat(params);
			} else if(method=="signalRate") {
				try {
					std::cout << Json::StyledWriter().write(params) << std::endl;
					m_signalRateSamples = params["samples"].asUInt();
					m_signalRateSamplesDelta.set(params["delta"]);
					m_signalRateDelta.setNtpTimestamp(m_signalRateSamplesDelta.ntpTimeStamp()/m_signalRateSamples);
				} catch(const std::runtime_error& e) {
					std::cerr << e.what();
				}
			} else {
				std::cout << "unhandled signal related meta information '" << method << "' for signal " << m_signalReference << " with parameters: " << Json::StyledWriter().write(params) << std::endl;
			}
		}


		int SubscribedSignal::setDataFormat(const Json::Value& params)
		{
			try {
				std::string dataFormatPattern = params["pattern"].asString();
				if(dataFormatPattern=="V") {
					m_dataFormatPattern = PATTERN_V;
				} else if(dataFormatPattern=="TV"){
					m_dataFormatPattern = PATTERN_TV;
				} else if(dataFormatPattern=="TB"){
					m_dataFormatPattern = PATTERN_TB;
				} else {
					return -1;
				}

				if(params["endian"].asString()=="big") {
					m_dataIsBigEndian = true;
				} else {
					m_dataIsBigEndian = false;
				}
				std::string dataValueType = params["valueType"].asString();
				if(dataValueType=="real32") {
					m_dataValueType = DATATYPE_REAL32;
					m_dataValueSize = 4;
				} else if(dataValueType=="u32") {
					m_dataValueType = DATATYPE_U32;
					m_dataValueSize = 4;
				} else if(dataValueType=="s32") {
					m_dataValueType = DATATYPE_S32;
					m_dataValueSize = 4;
				} else if(dataValueType=="real64") {
					m_dataValueType = DATATYPE_REAL64;
					m_dataValueSize = 8;
				} else if(dataValueType=="u64") {
					m_dataValueType = DATATYPE_U64;
					m_dataValueSize = 8;
				} else if(dataValueType=="s64") {
					m_dataValueType = DATATYPE_S64;
					m_dataValueSize = 8;
				}


				std::string dataTimeType = params["time"]["type"].asString();
				if(dataTimeType=="ntp") {
					m_dataTimeType = TIMETYPE_NTP;
				}
				m_dataTimeSize = params["time"]["size"].asUInt();

				return 0;
			} catch(const std::runtime_error& e) {
				std::cerr << e.what();
				return -1;
			}
		}
	}
}