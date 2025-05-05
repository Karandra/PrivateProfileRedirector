#include "stdafx.h"
#include "INIWrapper.h"
#include <kxf/FileSystem/NativeFileSystem.h>
#include <kxf/System/Win32Error.h>
#include <kxf/IO/MemoryStream.h>
#include <kxf/IO/StreamReaderWriter.h>
#include <kxf/IO/NativeFileStream.h>
#include <kxf/Utility/CallbackAdapters.h>

namespace
{
	constexpr uint8_t BOM_UTF8[] = {0xEF, 0xBB, 0xBF};
	constexpr uint8_t BOM_UTF16_LE[] = {0xFF, 0xFE};
	constexpr uint8_t BOM_UTF16_BE[] = {0xFE, 0xFF};
	constexpr uint8_t BOM_UTF32_LE[] = {0xFF, 0xFE, 0x00, 0x00};
	constexpr uint8_t BOM_UTF32_BE[] = {0x00, 0x00, 0xFE, 0xFF};

	template<class T>
	bool TestBOM(const kxf::MemoryStreamBuffer& buffer, const T& bom) noexcept
	{
		if (buffer.GetBufferSize() >= std::size(bom))
		{
			return std::memcmp(buffer.GetBufferStart(), bom, std::size(bom)) == 0;
		}
		return false;
	}

	template<class T>
	bool TestAndSkipBOM(kxf::MemoryStreamBuffer& buffer, const T& bom) noexcept
	{
		if (TestBOM(buffer, bom))
		{
			buffer.Seek(std::size(bom), kxf::IOStreamSeek::FromStart);
			return true;
		}
		return false;
	}
}

namespace PPR
{
	std::optional<kxf::String> INIWrapper::ExtractValue(kxf::String value) const
	{
		return std::move(value);
	}

	bool INIWrapper::Load(const kxf::FSPath& path, kxf::FlagSet<kxf::INIDocumentOption> options)
	{
		KXF_SCOPEDLOG_ARGS(path.GetFullPath(), options);

		m_INI.ClearNode();
		m_INI.SetOptions(options);

		kxf::NativeFileStream fileStream;
		if (fileStream.Open(path, kxf::IOStreamAccess::Read, kxf::IOStreamDisposition::OpenExisting, kxf::IOStreamShare::Read))
		{
			kxf::MemoryOutputStream memoryStream;
			if (fileStream.Read(memoryStream).LastRead() != 0)
			{
				// Release the input stream as fast as possible
				fileStream.Close();

				auto& buffer = memoryStream.GetStreamBuffer();
				buffer.Rewind();

				auto LoadUTF8 = [&]()
				{
					m_Encoding = Encoding::UTF8;
					return m_INI.Load(std::span{reinterpret_cast<const char8_t*>(buffer.GetBufferCurrent()), buffer.GetBytesLeft()});
				};
				auto LoadUTF16LE = [&]()
				{
					m_Encoding = Encoding::UTF16LE;
					return m_INI.Load(kxf::String(reinterpret_cast<const wchar_t*>(buffer.GetBufferCurrent()), buffer.GetBytesLeft() / sizeof(wchar_t)));
				};
				auto LoadUTF32LE = [&]()
				{
					kxf::MemoryInputStream stream(buffer.GetBufferCurrent(), buffer.GetBytesLeft());
					kxf::IO::InputStreamReader reader(stream);

					m_Encoding = Encoding::UTF32LE;
					return m_INI.Load(reader.ReadStringUTF32(buffer.GetBytesLeft() / sizeof(uint32_t)));
				};

				if (TestAndSkipBOM(buffer, BOM_UTF8))
				{
					KXF_SCOPEDLOG.Info() << "UTF-8 BOM detected";

					m_Options.Add(Options::WithBOM);
					if (LoadUTF8())
					{
						KXF_SCOPEDLOG.LogReturn(true);
						return true;
					}
				}
				else if (TestAndSkipBOM(buffer, BOM_UTF32_LE))
				{
					KXF_SCOPEDLOG.Info() << "UTF-32LE BOM detected";

					m_Options.Add(Options::WithBOM);
					if (LoadUTF32LE())
					{
						KXF_SCOPEDLOG.LogReturn(true);
						return true;
					}
				}
				else if (TestAndSkipBOM(buffer, BOM_UTF32_BE))
				{
					KXF_SCOPEDLOG.Info() << "UTF-32BE BOM detected";

					m_Options.Add(Options::WithBOM);
					m_Encoding = Encoding::UTF32BE;

					KXF_SCOPEDLOG.Error() << "Unsupported encoding";
				}
				else if (TestAndSkipBOM(buffer, BOM_UTF16_LE))
				{
					KXF_SCOPEDLOG.Info() << "UTF-16LE BOM detected";

					if (LoadUTF16LE())
					{
						KXF_SCOPEDLOG.LogReturn(true);
						return true;
					}
				}
				else if (TestAndSkipBOM(buffer, BOM_UTF16_BE))
				{
					KXF_SCOPEDLOG.Info() << "UTF-16BE BOM detected";

					m_Options.Add(Options::WithBOM);
					m_Encoding = Encoding::UTF16BE;

					KXF_SCOPEDLOG.Error() << "Unsupported encoding";
				}
				else
				{
					KXF_SCOPEDLOG.Info() << "No known BOM detected, trying to load as UTF-8";
					if (LoadUTF8())
					{
						KXF_SCOPEDLOG.LogReturn(true);
						return true;
					}
				}
			}
		}
		else
		{
			KXF_SCOPEDLOG.Error().Format("Can't open file to read: {}", kxf::Win32Error::GetLastError());
		}

		KXF_SCOPEDLOG.LogReturn(false);
		KXF_SCOPEDLOG.SetFail();
		return false;
	}
	bool INIWrapper::Save(const kxf::FSPath& path, Encoding encoding)
	{
		KXF_SCOPEDLOG_ARGS(path.GetFullPath(), encoding);

		kxf::NativeFileStream fileStream;
		if (!fileStream.Open(path, kxf::IOStreamAccess::Write, kxf::IOStreamDisposition::CreateAlways, kxf::IOStreamShare::Read))
		{
			KXF_SCOPEDLOG.Error().Format("Can't open file to write: {}", kxf::Win32Error::GetLastError());
			KXF_SCOPEDLOG.LogReturn(false);
			KXF_SCOPEDLOG.SetFail();

			return false;
		}

		if (encoding == Encoding::None)
		{
			encoding = m_Encoding;
		}

		const bool addSignature = m_Options.Contains(Options::WithBOM);
		switch (encoding)
		{
			case Encoding::None:
			case Encoding::Auto:
			case Encoding::UTF8:
			{
				KXF_SCOPEDLOG.Info() << "Saving as UTF-8";

				if (addSignature)
				{
					KXF_SCOPEDLOG.Info() << "Writing BOM";
					fileStream.Write(BOM_UTF8, std::size(BOM_UTF8));
				}
				
				if (m_INI.Save(fileStream))
				{
					KXF_SCOPEDLOG.LogReturn(true);
					return true;
				}
				break;
			}
			case Encoding::UTF16LE:
			{
				KXF_SCOPEDLOG.Info() << "Saving as UTF-16LE";

				auto content = m_INI.Save();
				if (addSignature)
				{
					KXF_SCOPEDLOG.Info() << "Writing BOM";
					fileStream.Write(BOM_UTF16_LE, std::size(BOM_UTF16_LE));
				}

				kxf::IO::OutputStreamWriter writer(fileStream);
				if (writer.WriteStringUTF16(content))
				{
					KXF_SCOPEDLOG.LogReturn(true);
					return true;
				}
				break;
			}
			case Encoding::UTF32LE:
			{
				KXF_SCOPEDLOG.Info() << "Saving as UTF-32LE";

				auto content = m_INI.Save();
				if (addSignature)
				{
					KXF_SCOPEDLOG.Info() << "Writing BOM";
					fileStream.Write(BOM_UTF32_LE, std::size(BOM_UTF32_LE));
				}

				kxf::IO::OutputStreamWriter writer(fileStream);
				if (writer.WriteStringUTF16(content))
				{
					KXF_SCOPEDLOG.LogReturn(true);
					return true;
				}
				break;
			}
		};

		KXF_SCOPEDLOG.LogReturn(false);
		KXF_SCOPEDLOG.SetFail();
		return false;
	}

	std::vector<kxf::String> INIWrapper::GetSectionNames() const
	{
		std::vector<kxf::String> items;
		m_INI.EnumSectionNames(kxf::Utility::VectorCallbackAdapter(items));

		return items;
	}
	std::vector<kxf::String> INIWrapper::GetKeyNames(const kxf::String& section) const
	{
		std::vector<kxf::String> items;
		m_INI.EnumKeyNames(section, kxf::Utility::VectorCallbackAdapter(items));

		return items;
	}
}
