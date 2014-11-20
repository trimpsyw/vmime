// emlparser.cpp : Defines the exported functions for the DLL application.
//

#include "emlparser.h"
#include "../libvmime/src/vmime/vmime.hpp"
#include <fstream>
#include <iostream>
#include <time.h>

using namespace vmime;
using std::vector;

// NOTE: The entire vmime library uses UTF-8 internally.
// All strings passed to vmime code must be UTF-8 encoded.
// All strings returned from vmime (Trace, exceptions, etc..) must be converted back to Unicode
#define UTF(Unicode)  vmime::charset::WstringToUtf8(Unicode)
#define UNI(Utf)      vmime::charset::Utf8ToWstring(Utf)

// If a Unicode string is stored in a std::string -> copy it to a std::wstring
#define TOW(str)      wstring((const wchar_t*)str.c_str(), str.length()/2)



void SetTraceCallback(void (*tTraceCallback)(const char* s8_Trace))
{
	vmime::SetTraceCallback(tTraceCallback);
}

class EMLParserImpl
{
public:
	EMLParserImpl():mi_Parser(0){}

	ref<const htmlTextPart> GetHtmlTextPart()
	{
		for (size_t i=0; i<mi_Parser->getTextPartCount(); i++)
		{
			const textPart& i_Part = *mi_Parser->getTextPartAt(i);

			if (i_Part.getType().getSubType() == mediaTypes::TEXT_HTML)
			{
				return create<const htmlTextPart>(dynamic_cast<const htmlTextPart&>(i_Part));
			}
		}
		return NULL;
	}

	ref<messageParser> mi_Parser;


};


EMLParser::EMLParser(): impl(new EMLParserImpl())
{
}

bool EMLParser::LoadFile(const string& filename)
{
	std::ifstream infile(filename.c_str(), std::ios::in);
	if(!infile) return false;

	infile.seekg(0, std::ios::end);
	int n = infile.tellg();
	infile.seekg(std::ios::beg);
	string buffer;
	buffer.resize(n+1);
	infile.read((char *)buffer.data(), n);

	return LoadData(buffer.data(), n);
}

bool EMLParser::LoadData(const char *body, size_t n)
{
	if(!body || n == 0)
		return false;

	size_t pos = 0, end = n;
	while(pos < end){
		char_t c = body[pos];
		if(c == ' ') { 
			pos++; 
			continue;
		} else if (c == '\r' && pos + 1 < end && body[pos + 1] == '\n') {
			pos += 2; 
			continue; 
		} else if (c == '\n'){
			pos ++; 
			continue; 
		}
		else 
			break;
	}
		
	string ms_Message = string(body+pos, end-pos);
	
	impl->mi_Parser = create<messageParser>(ms_Message);
	return impl->mi_Parser != NULL;
}

string	EMLParser::GetFrom()
{
	mailbox from = impl->mi_Parser->getExpeditor();
	string name = from.getName().getConvertedText(charsets::UTF_8);
	string email = from.getEmail().toString();
	return name + " <" + email + ">";
}

void EMLParser::GetTo(vector<string>& pi_Emails)
{
	addressList to = impl->mi_Parser->getRecipients();
	ref<mailboxList>  mailboxs = to.toMailboxList();
	pi_Emails.resize(mailboxs->getMailboxCount());
    for (size_t i=0; i< mailboxs->getMailboxCount(); i++)
    {
        ref<mailbox> i_Mailbox = mailboxs->getMailboxAt(i);
		string name = i_Mailbox->getName().getConvertedText(charsets::UTF_8);
		string email = i_Mailbox->getEmail().toString();

		pi_Emails[i] = name + " <" + email + ">";
	}
}
    
void EMLParser::GetCc(vector<string>& pi_Emails)
{
	addressList to = impl->mi_Parser->getCopyRecipients();
	ref<mailboxList>  mailboxs = to.toMailboxList();
	pi_Emails.resize(mailboxs->getMailboxCount());
    for (size_t i=0; i< mailboxs->getMailboxCount(); i++)
    {
        ref<mailbox> i_Mailbox = mailboxs->getMailboxAt(i);
		string name = i_Mailbox->getName().getConvertedText(charsets::UTF_8);
		string email = i_Mailbox->getEmail().toString();

		pi_Emails[i] = name + " <" + email + ">";
	}

}

unsigned int EMLParser::GetDate()
{
	datetime t = impl->mi_Parser->getDate();
	t = utility::datetimeUtils::toUniversalTime(t);
	
	struct tm tt;
	tt.tm_sec = t.getSecond();
	tt.tm_min = t.getMinute();
	tt.tm_hour = t.getHour();
	tt.tm_mday = t.getDay();
	tt.tm_mon = t.getMonth()-1;
	tt.tm_year = t.getYear() - 1900;
	tt.tm_wday = t.getWeekDay();

	time_t t0 = _mkgmtime(&tt);
	
	return t0;
}
	

string EMLParser::GetSubject()
{
	text sub = impl->mi_Parser->getSubject();
	try
	{
		return sub.getConvertedText(charsets::UTF_8);
	}
	catch(...)
	{
		return sub.getWholeBuffer();
	}
}

string	EMLParser::GetBody(bool plaintext)
{
	ref<messageParser>& mi_Parser = impl->mi_Parser;
	if(mi_Parser->getTextPartCount() == 0)
		return "";

	if(plaintext)
	{
		string s_Text;
		utility::outputStreamStringAdapter i_Stream(s_Text);
		for (size_t i=0; i<mi_Parser->getTextPartCount(); i++)
		{
			const textPart& i_Part = *mi_Parser->getTextPartAt(i);

			if (i_Part.getType().getSubType() == mediaTypes::TEXT_HTML)
			{
				const htmlTextPart& i_Html = dynamic_cast<const htmlTextPart&>(i_Part);

				i_Html.getPlainText()->extract(i_Stream);
				try{
					return charset::WstringToUtf8(charset::CpToWstring(s_Text, i_Html.getCharset()));
				}catch(...){
					vmime::Trace("caught exception unknown %s\n", i_Html.getCharset().getName().c_str());
					return s_Text;
				}
			}

			if (i_Part.getType().getSubType() == mediaTypes::TEXT_PLAIN)
			{
				const plainTextPart& i_Plain = dynamic_cast<const plainTextPart&>(i_Part);

				i_Plain.getText()->extract(i_Stream);

				try{
					return charset::WstringToUtf8(charset::CpToWstring(s_Text, i_Plain.getCharset()));
				}catch(...){
					vmime::Trace("caught exception unknown %s\n", i_Plain.getCharset().getName().c_str());
					return s_Text;
				}
			}
		}
		return "";
	}

	ref<const htmlTextPart> i_Html = impl->GetHtmlTextPart();
	if (!i_Html) return "";
	string s_Text;
	utility::outputStreamStringAdapter i_Stream(s_Text);

	i_Html->getText()->extract(i_Stream);
	try{
		return charset::WstringToUtf8(charset::CpToWstring(s_Text, i_Html->getCharset())); 
	}catch(...){
		vmime::Trace("caught exception unknown %s\n", i_Html->getCharset().getName().c_str());
		return s_Text;
	}
}

size_t	EMLParser::GetAttachmentCount()
{
	return impl->mi_Parser->getAttachmentCount();
}
 
bool EMLParser::GetAttachmentAt(size_t u32_Index, string& s_Name, string& s_ContentType, string& s_Data)
{
	if (u32_Index >= impl->mi_Parser->getAttachmentCount())
    {
        s_Name.clear();
        s_ContentType.clear();
        s_Data.clear();
        return false;
    }

    const attachment& i_Attach = *impl->mi_Parser->getAttachmentAt(u32_Index);

    s_Name = i_Attach.getName().getConvertedText(charsets::UTF_8);
    s_ContentType = i_Attach.getType().generate();

    utility::outputStreamStringAdapter i_Stream(s_Data);
    i_Attach.getData()->extract(i_Stream);
	return true;
}


size_t	EMLParser::GetEmbeddedObjectCount()
{
	ref<const htmlTextPart> i_Html = impl->GetHtmlTextPart();
    if (!i_Html)
        return 0;

    return i_Html->getObjectCount();
}

bool EMLParser::GetEmbeddedObjectAt(size_t u32_Index, string& s_Id, string& s_MimeType, string& s_Data)
{
	ref<const htmlTextPart> i_Html = impl->GetHtmlTextPart();
        
    if (!i_Html || u32_Index >= i_Html->getObjectCount())
    {
        s_Id.clear();
        s_MimeType.clear();
        s_Data.clear();
        return false;
    }

    const htmlTextPart::embeddedObject& i_Object = *i_Html->getObjectAt(u32_Index);

    s_Id       = i_Object.getId();
    s_MimeType = i_Object.getType().generate();

    utility::outputStreamStringAdapter i_Stream(s_Data);
    i_Object.getData()->extract(i_Stream);
	return true;
}

#include "cPop3.hpp"
#include "cImap.hpp"
#include "cEmailParser.hpp"
#include "resource.h"

class Pop3ReaderImpl: public vmime::wrapper::cPop3
{
public:
	Pop3ReaderImpl(const wchar_t* u16_Server, vmime_uint16 u16_Port, wrapper::cCommon::eSecurity e_Security, 
		 bool b_AllowInvalidCerts, vmime_uint16 u16_ResIdCert) : 
		cPop3(u16_Server, u16_Port, e_Security, b_AllowInvalidCerts, u16_ResIdCert)
		{
		}

	friend class Pop3Reader;
};


Pop3Reader::Pop3Reader(const std::string& addr, unsigned short port, bool ssl):
impl(new Pop3ReaderImpl(UNI(addr).c_str(), 0, ssl?wrapper::cCommon::Secur_SSL:wrapper::cCommon::Secur_Normal, true, IDR_ROOT_CA))
{
}

Pop3Reader::~Pop3Reader()
{
	delete impl;
}

int Pop3Reader::SelectFolder(const std::string& path)
{
	try
	{
		impl->SelectFolder(UNI(path));
		return 0;
	}
	catch (std::exception& e)
	{
		vmime::Trace("select: %s\n", e.what());
		return -1;
	}
}
	
int Pop3Reader::GetAllFolders(std::vector<std::string> & folders)
{
	std::vector<std::wstring> wfolders;
	impl->EnumFolders(wfolders);

	for(int i = 0; i < wfolders.size(); i++)
		folders.push_back(UTF(wfolders[i]));

	return wfolders.size();
}



int Pop3Reader::Connect(const std::string& username, const std::string& password)
{
	try
	{
		impl->SetAuthData(UNI(username).c_str(), UNI(password).c_str());
		impl->Connect();
		return 0;
	}
	catch (std::exception& e)
	{
		vmime::Trace("connect: %s\n", e.what());
		return -1;
	}
}


int Pop3Reader::GetEmailCount(const std::string& uid, std::vector<int>& nums)
{
	try
	{
		int cnt = impl->GetEmailCount();
		for(int i = 0; i < cnt; i++)
			nums.push_back(i);	
		return cnt;
	}
	catch(...)
	{
		return -1;
	}

}

std::string Pop3Reader::GetUid(int i)
{
	try
	{
		return impl->GetUid(i);
	}
	catch(...)
	{
		return "";
	}
}

void Pop3Reader::Close()
{
	try
	{
		impl->Close();
	}
	catch (std::exception& e)
	{
		vmime::Trace("close: %s\n", e.what());
	}
}

int Pop3Reader::GetEmailMessage(int i, std::string& data)
{
	try
	{
		vmime::wrapper::GuardPtr<vmime::wrapper::cEmailParser> i_Email = impl->FetchEmailAt(i);
		data = i_Email.Ptr()->GetRawMessage();
		return 0;
	}
	catch(...)
	{
		return -1;
	}
}

class ImapReaderImpl: public vmime::wrapper::cImap
{
public:
	ImapReaderImpl(const wchar_t* u16_Server, vmime_uint16 u16_Port, wrapper::cCommon::eSecurity e_Security, 
		 bool b_AllowInvalidCerts, vmime_uint16 u16_ResIdCert) : 
		cImap(u16_Server, u16_Port, e_Security, b_AllowInvalidCerts, u16_ResIdCert)
		{
		}

	friend class ImapReader;
};


ImapReader::ImapReader(const std::string& addr, unsigned short port, bool ssl): 
impl(new ImapReaderImpl(UNI(addr).c_str(), 0, ssl?wrapper::cCommon::Secur_SSL:wrapper::cCommon::Secur_Normal, true, IDR_ROOT_CA))
{
}

ImapReader::~ImapReader()
{
	delete impl;
}


int ImapReader::Connect(const std::string& username, const std::string& password)
{
	int ntry = 0;
	do
	{
		try
		{
			impl->SetAuthData(UNI(username).c_str(), UNI(password).c_str());
			impl->Connect();
			return 0;
		}
		catch (exceptions::operation_timed_out& e)
		{
			vmime::Trace("Timeout Connect %d %s\n", ntry, e.name());
			if(++ntry <= 3) continue;
			return -2;
		}
		catch(exceptions::authentication_error& e)
		{
			vmime::Trace("Failed authentication %s %s\n", e.name(), e.what());
			return -1;
		}
		catch(vmime::exception& e)
		{
			vmime::Trace("Failed to connect %s\n", e.name());
			return -1;
		}
		catch(std::exception& e)
		{
			vmime::Trace("Failed to connect %s\n", e.what());
			return -1;
		}
	}while(1);
}

int ImapReader::SelectFolder(const std::string& path)
{
	int ntry = 0;
	do
	{
		try
		{
			impl->SelectFolder(UNI(path));
			return 0;
		}
		catch(exceptions::folder_already_open& e)
		{
			return 0;
		}
		catch (exceptions::operation_timed_out& e)
		{
			vmime::Trace("Timeout SelectFolder %d %s\n", ntry, e.name());
			if(++ntry <= 3) continue;
			return -2;
		}
		catch (exceptions::illegal_state& e)
		{
			vmime::Trace("Illegal_state %d %s\n", ntry, e.name());
			if(++ntry <= 3) continue;
			return -2;
		}
		catch(vmime::exception& e)
		{
			vmime::Trace("Failed to SelectFolder %s\n", e.name());
			return -1;
		}
		catch(std::exception& e)
		{
			vmime::Trace("Failed to SelectFolder %s\n", e.what());
			return -1;
		}
	}while(1);
}

int ImapReader::GetAllFolders(std::vector<std::string> & folders)
{
	std::vector<std::wstring> wfolders;
	impl->EnumFolders(wfolders);

	for(int i = 0; i < wfolders.size(); i++)
		folders.push_back(UTF(wfolders[i]));
	return wfolders.size();
}


int ImapReader::GetEmailCount(const std::string& uid, std::vector<int>& nums)
{
	int cnt;
	std::vector<int> nums2;
	try
	{
		cnt = impl->GetEmailCount();
	}
	catch (exceptions::operation_timed_out& e)
	{
		vmime::Trace("TimeOut GetEmailCount %s %s\n", e.name(), e.what());
		return -2;
	}
	catch(vmime::exception& e)
	{
		vmime::Trace("Failed to GetEmailCount %s %s\n", e.name(), e.what());
		return -1;
	}
	catch(std::exception& e)
	{
		vmime::Trace("Failed to GetEmailCount %s\n", e.what());
		return -1;
	}

	if(uid.empty()) goto end;

	try
	{
		nums2 = impl->getMessageNumbersStartingOnUID(uid);
		for(int i = 0; i < nums2.size(); i++)
			nums.push_back(nums2[i]-1);
		return nums.size();
	}
	catch(...)
	{
	}

end:
	for(int i = 0; i < cnt; i++)
		nums.push_back(i);	
	return cnt;
}


std::string ImapReader::GetUid(int i)
{
	int ntry = 0;
	std::wstring path = impl->GetCurrentFolder();
	do
	{
		try
		{
			return impl->GetUid(i);
		}
		catch (exceptions::operation_timed_out& e)
		{
			vmime::Trace("Timeout GetUid %d %s\n", ntry, e.name());
			if(++ntry <= 3)
			{
				Close();
				impl->Connect();
				impl->SelectFolder(path);
				continue;
			}
			return "";
		}
		catch (exceptions::invalid_response& e)
		{
			vmime::Trace("InvalidResp GetUid %d %s\n", ntry, e.name());
			if(++ntry <= 3)
			{
				Close();
				impl->Connect();
				impl->SelectFolder(path);
				continue;
			}
			return "";
		}
		catch(vmime::exception& e)
		{
			vmime::Trace("Failed to GetUid@ %s %s\n", e.name(), e.what());
			return "";
		}
		catch(std::exception& e)
		{
			vmime::Trace("Failed to GetUid %s\n", e.what());
			return "";
		}
	}while(1);
}


int ImapReader::GetEmailMessage(int i, std::string& data)
{
	int ntry = 0;
	std::wstring path = impl->GetCurrentFolder();
	do
	{
		try
		{
			vmime::wrapper::GuardPtr<vmime::wrapper::cEmailParser> i_Email = impl->FetchEmailAt(i);
			data = i_Email.Ptr()->GetRawMessage();
			return 0;
		}
		catch (exceptions::operation_timed_out& e)
		{
			vmime::Trace("Timeout GetEmailMessage %d %s\n", ntry, e.name());
			if(++ntry <= 3) 
			{
				Close();
				impl->Connect();
				impl->SelectFolder(path);

				continue;
			}
			return -2;
		}
		catch (exceptions::invalid_response& e)
		{
			vmime::Trace("InvalidResp GetEmailMessage %d %s\n", ntry, e.name());
			if(++ntry <= 3)
			{
				Close();
				impl->Connect();
				impl->SelectFolder(path);
				continue;
			}
			return -2;
		}
		catch(vmime::exception& e)
		{
			vmime::Trace("Failed to GetEmailMessage %s %s\n", e.name(), e.what());
			return -1;
		}
		catch(std::exception& e)
		{
			vmime::Trace("Failed to GetEmailMessage %s\n", e.what());
			return -1;
		}
	}while(1);
}

void ImapReader::Close()
{
	try
	{
		impl->Close();
	}
	catch (std::exception& e)
	{
		vmime::Trace("close: %s\n", e.what());
	}
}


#ifdef WIN64
    #pragma comment(lib, "../libvmime/src/gsasl/libgsasl-7_64.lib") // 64 Bit, Release
#else
    #pragma comment(lib, "../libvmime/src/gsasl/libgsasl-7_32.lib") // 32 Bit, Release
#endif

// Required Dll's: 
// libgnutls-28.dll, 
// libnettle-4-7.dll, 
// libhogweed-2-5.dll, 
// libgmp-10.dll, 
// libp11-kit-0.dll 
#if VMIME_TLS_SUPPORT_LIB_IS_GNUTLS
    #error GnuTLS does not run correctly on Windows.
    #error Additionally a 64 bit version does not exist.
#endif

// The following LIB's may be:
// -- dynamic -> require libeay32.dll and ssleay32.dll
// -- static  -> no Dlls required
// IMPORTANT: Read documentation about the required VC++ runtime!
#if VMIME_TLS_SUPPORT_LIB_IS_OPENSSL

    // You can compile against the Debug version of openssl by setting the following definition = 1
    // But these Lib files are not included, because their total size is 24 MB
    #define DEBUG_LIBS_PRESENT   0

    #ifdef WIN64
        #if _DEBUG && DEBUG_LIBS_PRESENT
            #pragma comment(lib, "../libvmime/src/openssl/libeay64MDd.lib") // MultiThreadedDll, 64 Bit, Debug
            #pragma comment(lib, "../libvmime/src/openssl/ssleay64MDd.lib")
        #else
            #pragma comment(lib, "../libvmime/src/openssl/libeay64MD.lib")  // MultiThreadedDll, 64 Bit, Release
            #pragma comment(lib, "../libvmime/src/openssl/ssleay64MD.lib")
        #endif
    #else
        #if _DEBUG && DEBUG_LIBS_PRESENT
            #pragma comment(lib, "../libvmime/src/openssl/libeay32MDd.lib") // MultiThreadedDll, 32 Bit, Debug
            #pragma comment(lib, "../libvmime/src/openssl/ssleay32MDd.lib")
        #else
            #pragma comment(lib, "../libvmime/src/openssl/libeay32MD.lib")  // MultiThreadedDll, 32 Bit, Release
            #pragma comment(lib, "../libvmime/src/openssl/ssleay32MD.lib")
        #endif
    #endif
    #pragma comment(lib, "Crypt32.lib") // Microsoft Crypt32.dll
#endif

#ifdef WIN64
    #if _DEBUG
        #pragma comment(lib, "../libvmime/Debug64/vmime.lib")
    #else
        #pragma comment(lib, "../libvmime/Release64/vmime.lib")
    #endif
#else
    #if _DEBUG
        #pragma comment(lib, "../libvmime/Debug32/vmime.lib")
    #else
        #pragma comment(lib, "../libvmime/Release32/vmime.lib")
    #endif
#endif
