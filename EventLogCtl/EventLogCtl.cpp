
#include <Windows.h>

#include <string>
#include <cstdio>
#include <iostream>

#include "IChannelPathEnumerator.h"
#include "IChannelConfig.h"
#include "IPublisherMetadata.h"
#include "IPublisherEnumerator.h"
#include "IEventReader.h"

using Windows::EventLog::IChannelConfig;
using Windows::EventLog::IPublisherMetadata;
using Windows::EventLog::IChannelPathEnumerator;
using Windows::EventLog::IPublisherEnumerator;
using Windows::EventLog::IPublisherChannelArray;
using Windows::EventLog::IPublisherLevelArray;
using Windows::EventLog::IPublisherTaskArray;
using Windows::EventLog::IPublisherOpcodeArray;
using Windows::EventLog::IPublisherKeywordArray;
using Windows::EventLog::IEventMetadataEnumerator;
using Windows::EventLog::IEventReader;
using Windows::EventLog::IEventRecord;
using Windows::EventLog::Direction;
using Windows::Ref;
using Windows::RefPtr;


static constexpr char nl[] = { '\n' };
static constexpr char tab[] = { '\t' };

std::string to_string(const std::optional<std::string> &v) 
{
	return v.has_value() ? v.value() : std::string{};
}

template<typename T>
std::string to_string(const std::optional<T> &opt)
{
	using std::to_string;
	using Windows::to_string;
	using Windows::EventLog::to_string;
	if (opt.has_value())
		return to_string(opt.value());
	return {};
}

static std::string to_string(const std::vector<std::string> &v)
{
	std::string result{};
	size_t n = v.size();
	if (n > 0)
	{
		result.append(v[0]);
		for (size_t i = 1; i < n; ++i)
		{
			result.append(", ").append(v[i]);
		}
	}
	return result;
}


static 
std::string to_hex_string(uint16_t n)
{
	// 0x1234'\0'
	static constexpr int BUF_SIZE = 7;
	char buf[BUF_SIZE] = {};
	buf[0] = '0'; buf[1] = 'x';
	int count = snprintf(&buf[2], size_t(BUF_SIZE) - 2u, "%4.4x", n);
	if (count >= BUF_SIZE)
		return {};
	return { buf };
}

static 
std::string to_hex_string(uint32_t n)
{
	// 0x12341234'\0'
	static constexpr int BUF_SIZE = 11;
	char buf[BUF_SIZE] = {};
	buf[0] = '0'; buf[1] = 'x';
	int count = snprintf(& buf[2], size_t(BUF_SIZE) - 2u, "%8.8x", n);
	if (count >= BUF_SIZE)
		return {};
	return { buf };
}

static
std::string to_hex_string(uint64_t value)
{
	// Ox1234123412341234'\0' (4*4=16+2=18+1=19)
	static constexpr int BUF_SIZE = 19;
	char buf[BUF_SIZE] = {};
	buf[0] = '0'; buf[1] = 'x';
	int count = snprintf(&buf[2], size_t(BUF_SIZE) - 2u, "%16.16llx", value);
	if (count >= BUF_SIZE)
		return {};
	return { buf };
}

template<typename  T,
	typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool> > >
std::string to_hex_string(const std::optional<T> &v)
{
	if (v.has_value()) 
		return to_hex_string(v.value());
	return {};
}


class EventLogCtl
{
public:
	EventLogCtl() {}
	~EventLogCtl() {}

	void run(int argc, char *argv[]);

private:

	void showChannelList();
	void showChannelConfig(const std::string &channelPath);
	void showPublishers();
	void showPublisherMetadata(const std::string &publisher);

	void queryChannel(const std::string &channel, const std::string &xpath);
	void queryFile(const std::string &filePath, const std::string &xpath);
	void query(const std::string &xml);

	void usage();

	EventLogCtl &operator=(const EventLogCtl &) = delete;
	EventLogCtl(const EventLogCtl &) = delete;

};

void EventLogCtl::usage()
{
	static const char *usageMsg =
		"eventlogctl COMMAND [OPTIONS...] \n\n"
		"Windows Event Log utility.\n"
		"\nCommands:\n"
		"  channel         Channel\n"
		"  publisher       Publisher\n"
		"  query           Perform a query\n";

	std::cout << usageMsg;
}

void EventLogCtl::showChannelList()
{
	Ref<IChannelPathEnumerator> e = IChannelPathEnumerator::create();
	while (e->next())
	{
		std::string path = e->getCurrent();
		std::cout << path << nl;
	}
}

static void printEventRecord(const IEventRecord &rec)
{
	std::cout
		<< "Provider Name: " << to_string(rec.getProviderName()) << nl
		<< "Provider GUID: " << to_string(rec.getProviderGuid()) << nl
		<< "Event Id: " << to_string(rec.getEventId()) << nl
		<< "Qualifiers: " << to_hex_string(rec.getQualifers()) << nl
		<< "Level Value: " << to_string(rec.getLevel()) << nl
		<< "Task Value: " << to_string(rec.getTask()) << nl 
		<< "Opcode Value: " << to_string(rec.getOpcode()) << nl
		<< "Keywords Mask: " << to_string(rec.getKeywords()) << nl
		<< "Creation Time: " << to_string(rec.getTimeCreated()) << nl
		<< "Record Id: " << to_string(rec.getRecordId()) << nl
		<< "Activity Id: " << to_string(rec.getActivityId()) << nl
		<< "Process Id: " << to_string(rec.getProcessId()) << nl
		<< "Thread Id: " << to_string(rec.getThreadId()) << nl
		<< "Channel: " << to_string(rec.getChannel()) << nl
		<< "Computer: " << to_string(rec.getComputer()) << nl
		<< "User: " << to_string(rec.getUser()) << nl
		<< "Version: " << to_string(rec.getVersion()) << nl
		<< "Level: " << rec.getLevelDisplay() << nl
		<< "Task: " << rec.getTaskDisplay() << nl
		<< "Opcode: " << rec.getOpcodeDisplay() << nl
		<< "Keywords: " << to_string(rec.getKeywordsDisplay()) << nl
		<< "Channel Message: " << to_string(rec.getChannelMessage()) << nl
		<< "Publisher Message: " << to_string(rec.getProviderMessage()) << nl
		<< "Message: " << nl << rec.getMessage() << nl;
		;
}

static void print(IEventReader &reader)
{
	static const std::string sepLine(80, '=');

	if (reader.next())
	{
		printEventRecord(reader.getRecord());

		while (reader.next())
		{
			std::cout << sepLine << nl;
			printEventRecord(reader.getRecord());
		}
	}
}

void EventLogCtl::queryChannel(const std::string &channel, const std::string &xpath)
{
	Ref<IEventReader> reader = IEventReader::openChannel(channel, xpath, Direction::Reverse);
	print(reader);	
}

void EventLogCtl::queryFile(const std::string &filePath, const std::string &xpath)
{
	Ref<IEventReader> reader = IEventReader::openFile(filePath, xpath, Direction::Reverse);
	print(reader);
}

void EventLogCtl::query(const std::string &xml)
{
	Ref<IEventReader> reader = IEventReader::openStructuredXML(xml, Direction::Reverse);
	print(reader);
}

static void printChannelConfig(const std::string &channelPath, IChannelConfig &channelConfig)
{
	using ::to_string;
	using std::to_string;
	using Windows::to_string;
	using Windows::EventLog::to_string;

	std::cout
		<< "Channel: " << channelPath << nl  
		<< "  Enabled: " << to_string(channelConfig.getConfigEnabled()) << nl
		<< "  Isolation: " << to_string(channelConfig.getConfigIsolation()) << nl
		<< "  Type: " << to_string(channelConfig.getConfigType()) << nl
		<< "  Publisher: " << channelConfig.getConfigOwningPublisher() << nl
		<< "  Is Classic: " << channelConfig.getConfigClassicEventLog() << nl
		<< "  Access: " << channelConfig.getConfigAccess() << nl
		<< "  Retention: " << to_string(channelConfig.getLoggingConfigRetention()) << nl
		<< "  File Max Size (bytes): " << to_string(channelConfig.getLoggingConfigMaxSize()) << nl
		<< "  Log File Path: " << channelConfig.getLoggingConfigLogFilePath() << nl
		<< "  Level: " << to_string(channelConfig.getPublishingConfigLevel()) << nl
		<< "  Keywords: " << to_string(channelConfig.getPublishingConfigKeywords()) << nl
		<< "  Control GUID: " << to_string(channelConfig.getPublishingConfigControlGuid()) << nl
		<< "  Buffer Size: " << to_string(channelConfig.getPublishingConfigBufferSize()) << nl
		<< "  Min Buffers: " << to_string(channelConfig.getPublishingConfigMinBuffers()) << nl
		<< "  Max Buffer: " << to_string(channelConfig.getPublishingConfigMaxBuffers()) << nl
		<< "  Latency: " << to_string(channelConfig.getPublishingConfigLatency()) << nl
		<< "  Clock Type: " << to_string(channelConfig.getPublishingConfigClockType()) << nl
		<< "  SID Type: " << to_string(channelConfig.getPublishingConfigSidType()) << nl
		<< "  Publishers: " << to_string(channelConfig.getPublisherList()) << nl
		<< "  File Max: " << to_string(channelConfig.getPublishingConfigFileMax()) << nl
		;
}

void EventLogCtl::showPublishers()
{
	Ref<IPublisherEnumerator> e = IPublisherEnumerator::create();
	while (e->next())
	{
		std::cout << e->getCurrent() << nl;
	}
}

void EventLogCtl::showChannelConfig(const std::string &channelPath)
{
	using std::to_string;
	using Windows::to_string;
	using Windows::EventLog::to_string;

	// Dump them all.
	// TODO: some kind of regex filter?
	if (channelPath.compare("*") == 0)
	{
		Ref<IChannelPathEnumerator> channelEnum = IChannelPathEnumerator::create();
		while (channelEnum->next())
		{
			std::string path = channelEnum->getCurrent();
			try
			{
				Ref<IChannelConfig> channelConfig = IChannelConfig::create(path);
				printChannelConfig(path, channelConfig);
			}
			catch (std::exception &)
			{
				// TODO: better error handling.
				std::cerr << "Error opening: " << path << nl;
			}
		}
	}
	else
	{
		try
		{
			Ref<IChannelConfig> channelConfig = IChannelConfig::create(channelPath);
			printChannelConfig(channelPath, channelConfig);
		}
		catch (std::exception &)
		{
			// TODO: need better error handling.
			std::cerr << "Error opening: " << channelPath << nl;
		}
	}
}

static void printPublisherMetadata(const IPublisherMetadata &publisherMeta)
{
	std::cout 
		<< "Publisher GUID: " << to_string(publisherMeta.getPublisherGuid()) << nl
		<< "Parameters File Path: " << to_string(publisherMeta.getParametersFilePath()) << nl
		<< "Message File Path" << to_string(publisherMeta.getMessageFilePath()) << nl
		<< "Help Link" << to_string(publisherMeta.getHelpLink()) << nl
		<< "Publisher Message: " << publisherMeta.getPublisherMessage() << nl;

	Ref<IPublisherChannelArray> channels = publisherMeta.getChannels();
	auto size = channels->getSize();
	if (size > 0)
	{
		std::cout << "Channels:" << nl;
		for (uint32_t i = 0u; i < size; ++i)
		{
			auto channelInfo = channels->getChannelInfo(i);
			std::cout 
				<< "  [" << i << "] Path" << channelInfo.getChannelReferencePath() << nl
				<< "  [" << i << "] Index" << channelInfo.getChannelReferenceIndex() << nl
				<< "  [" << i << "] Flags" << to_hex_string(channelInfo.getChannelReferenceFlags()) << nl
				<< "  [" << i << "] Message: " << channelInfo.getMessage() << nl;
		}
	}

	Ref<IPublisherLevelArray> levels = publisherMeta.getLevels();
	size = levels->getSize();
	if (size > 0)
	{
		std::cout << "Levels:" << nl;
		for (uint32_t i = 0u; i < size; ++i)
		{
			auto levelInfo = levels->getLevelInfo(i);
			std::cout
				<< "  [" << i << "] Name: " << levelInfo.getName() << nl
				<< "  [" << i << "] Value: " << levelInfo.getValue() << nl
				<< "  [" << i << "] Message: " << levelInfo.getMessage() << nl
				;
		}
	}

	Ref<IPublisherTaskArray> tasks = publisherMeta.getTasks();
	size = tasks->getSize();
	if (size > 0)
	{
		std::cout << "Tasks:" << nl;
		for (uint32_t i = 0u; i < size; ++i)
		{
			auto taskInfo = tasks->getTaskInfo(i);
			std::cout
				<< "  [" << i << "] Name: " << taskInfo.getName() << nl
				<< "  [" << i << "] GUID: " << Windows::to_string(taskInfo.getEventGuid()) << nl
				<< "  [" << i << "] Value: " << taskInfo.getValue() << nl
				<< "  [" << i << "] Message: " << taskInfo.getMessage() << nl
				;				
		}
	}

	Ref<IPublisherOpcodeArray> opcodes = publisherMeta.getOpcodes();
	size = opcodes->getSize();
	if (size > 0)
	{
		std::cout << "Opcodes:" << nl;
		for (uint32_t i = 0u; i < size; ++i)
		{
			auto opcodeInfo = opcodes->getOpcodeInfo(i);
			std::cout
				<< "  [" << i << "] Name: " << opcodeInfo.getName() << nl
				<< "  [" << i << "] Value: " << opcodeInfo.getValue() << nl
				<< "  [" << i << "] Message: " << opcodeInfo.getMessage() << nl;
		}
	}

	Ref<IPublisherKeywordArray> keywords = publisherMeta.getKeywords();
	size = keywords->getSize();
	if (size > 0)
	{
		std::cout << "Keywords:" << nl;
		for (uint32_t i = 0u; i < size; ++i)
		{
			auto keywordsInfo = keywords->getKeywordInfo(i);
			std::cout
				<< "  [" << i << "] Name: " << keywordsInfo.getName() << nl
				<< "  [" << i << "] Value: " << keywordsInfo.getValue() << nl
				<< "  [" << i << "] Message: " << keywordsInfo.getMessage() << nl;
		}
	}

	Ref<IEventMetadataEnumerator> eventsEnum = publisherMeta.openEventMetadataEnum();
	while (eventsEnum->next())
	{
		auto em = eventsEnum->getCurrent();

		auto channelValue = em->getChannel();
		auto levelValue = em->getLevel();
		auto opcodeValue = em->getOpcode();
		auto taskValue = em->getTask();
		auto keywordValue = em->getKeyword();

		auto msg = em->getMessageDisplay();
		auto tmpl = em->getTemplate();

		std::cout 
			<< "Event:" << nl
			<< "  ID: " << to_string(em->getId()) << nl
			<< "  Version: " << to_string(em->getVersion()) << nl
			<< "  Channel: " << to_string(channelValue) << nl
			<< "    String: " << em->getChannelDisplay() << nl
			<< "  Level: " << to_string(levelValue) << nl
			<< "    String: " << em->getLevelDisplay() << nl
			<< "  Opcode: " << to_string(opcodeValue) << nl
			<< "    String: " << em->getOpcodeDisplay() << nl
			<< "  Task: " << to_string(taskValue) << nl
			<< "    String: " << em->getTaskDisplay() << nl
			<< "  Keywords: " << to_hex_string(keywordValue) << nl
			<< "    Strings: " << to_string(em->getKeywordsDisplay()) << nl
			<< "  Template: " << to_string(em->getTemplate()) << nl
			<< "  Message ID: " << to_hex_string(em->getMessageID()) << nl
			<< "  Message: " << em->getMessageDisplay() << nl;
	}
}

void EventLogCtl::showPublisherMetadata(const std::string &publisher)
{
	if (publisher.compare("*") == 0)
	{
		Ref<IPublisherEnumerator> e = IPublisherEnumerator::create();
		while (e->next())
		{
			std::string publisherName = e->getCurrent();
			try
			{
				RefPtr<IPublisherMetadata> publisherMeta = IPublisherMetadata::cacheOpenProvider(publisherName);
				if (publisherMeta)
				{
					printPublisherMetadata(*publisherMeta);
				}
			}
			catch (std::exception &)
			{
				// TODO: need way better error handling and messages.
				std::cerr << "Error. Unable to show metadata for " << publisherName << nl; 
			}
		}
	}
	else
	{
		RefPtr<IPublisherMetadata> publisherMeta = IPublisherMetadata::cacheOpenProvider(publisher);
		if (publisherMeta)
			printPublisherMetadata(*publisherMeta);
	}
}

void EventLogCtl::run(int argc, char *argv[])
{
	//
	// This is not pretty.
	// 

	if (argc < 1)
		return;
	int index = 1;
	while (index < argc)
	{
		// channel -list 
		// channel -showconfig channel_path   
		if (strcmp("channel", argv[index]) == 0)
		{
			index += 1;
			
			if (index < argc)
			{ 
				if (strcmp("-list", argv[index]) == 0)
				{
					index += 1;

					showChannelList();
				}
				else if (strcmp("-showconfig", argv[index]) == 0)
				{
					index += 1;
					if (index < argc)
					{
						std::string channelPath(argv[index]);
						showChannelConfig(channelPath);
						index += 1;
					}
				}
				else
				{
					usage();
					index = argc;
				}
			}
			else
			{
				usage();
				index = argc;
			}
		}
		// publisher -list
		// publisher -showmetadata [*|publisher_name]
		else if (strcmp("publisher", argv[index]) == 0)
		{
			index += 1;
			if (index < argc)
			{
				if (strcmp("-list", argv[index]) == 0)
				{
					index += 1;
					showPublishers();
				}
				else if (strcmp("-showmetadata", argv[index]) == 0)
				{
					index += 1; // next arg
					if (index < argc)  // if there is an arg?
					{
						std::string publisher(argv[index]);
						showPublisherMetadata(publisher);
						index += 1;
					}
				}
				else
				{
					usage();
					index = argc;
				}
			}
			else
			{
				usage();
				index = argc;
			}
		}
		// query [-channel name] query 
		// query [-file archive_filepath] query
		// query [-xml xml_filepath]
		// TODO: direction flag at end? 
		else if (strcmp("query", argv[index]) == 0)
		{
			index += 1;
			if (index < argc)
			{
				if (strcmp("-channel", argv[index]) == 0)
				{
					index += 1;
					if (index < argc)
					{
						std::string channelName(argv[index]);
						
						index += 1;
						if (index < argc)
						{
							std::string xpath(argv[index]);
							queryChannel(channelName, xpath);
							index += 1;
						}
					}
					else
					{
						usage();
						index = argc;
					}
				}
				else if (strcmp("-file", argv[index]) == 0)
				{
					index += 1;
					if (index < argc)
					{
						std::string filePath(argv[index]);
						if (index < argc)
						{
							std::string xpath(argv[index]);
							queryFile(filePath, xpath);
							index += 1;
						}
					}
					else
					{
						usage();
						index = argc;
					}
				}
				else if (strcmp("-xml", argv[index]) == 0)
				{
					index += 1;
					if (index < argc)
					{
						std::string xml(argv[index]);
						query(xml);
						index += 1;
					}
					else
					{
						usage();
						index = argc;
					}
				}
				else
				{
					usage();
					index = argc;
				}
			}
		}
		else
		{
			usage();
			index = argc;
		}
	}

}

int main(int argc, char *argv[])
{
	EventLogCtl cli;
	cli.run(argc, argv);

	return 0;
}