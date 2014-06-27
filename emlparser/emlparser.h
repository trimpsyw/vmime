#ifndef EMLPARSER_H
#define EMLPARSER_H

#include <string>
#include <vector>

#ifdef EMLPARSER_EXPORTS
#define EMLPARSER_API __declspec(dllexport)
#else
#define EMLPARSER_API __declspec(dllimport)
#endif

class EMLParserImpl;
class EMLPARSER_API EMLParser
{
public: 
	EMLParser();

	bool LoadData(const char* body, size_t n);
	bool LoadFile(const std::string& filename);

	std::string	GetFrom();
    void    GetTo(std::vector<std::string>& pi_Emails);
    void    GetCc(std::vector<std::string>& pi_Emails);
	std::string	GetSubject();
    unsigned int GetDate();
	std::string	GetBody(bool plaintext = true);
	size_t	GetAttachmentCount();
    bool    GetAttachmentAt(size_t u32_Index, std::string& s_Name, std::string& s_ContentType, std::string& s_Data);
	
	size_t	GetEmbeddedObjectCount();
	bool    GetEmbeddedObjectAt(size_t u32_Index, std::string& s_Id, std::string& s_ContentType, std::string& s_Data);
private:
	EMLParserImpl* impl;
};

class EMLPARSER_API EmailReader
{
public:
	virtual ~EmailReader(){}
	virtual int Connect(const std::string& username, const std::string& password) = 0;

	virtual int GetEmailCount() = 0;

	virtual std::string GetUid(int i) = 0;

	virtual int GetEmailMessage(int i, std::string& data) = 0;

	virtual void Close() = 0;
};

class Pop3ReaderImpl;
class EMLPARSER_API Pop3Reader : public EmailReader
{
public:
	Pop3Reader(const std::string& addr, unsigned short port, bool ssl);
	~Pop3Reader();

	int Connect(const std::string& username, const std::string& password);

	int GetEmailCount();

	std::string GetUid(int i);

	int GetEmailMessage(int i, std::string& data);

	void Close();

private:
	Pop3ReaderImpl* impl;
};

class ImapReaderImpl;
class EMLPARSER_API ImapReader : public EmailReader
{
public:
	ImapReader(const std::string& addr, unsigned short port, bool ssl);
	~ImapReader();

	int Connect(const std::string& username, const std::string& password);

	int GetEmailCount();

	std::string GetUid(int i);

	int GetEmailMessage(int i, std::string& data);

	void Close();

private:
	ImapReaderImpl* impl;
};

#endif