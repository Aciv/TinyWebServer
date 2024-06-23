#ifndef HTTPRESPONDSE_H
#define HTTPRESPONDSE_H

#include <unordered_map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap

#include "../buffer/buffer.h"
#include "../log/log.h"

class cHttpResponse{
public:

    cHttpResponse();
    ~cHttpResponse();

    void Init(const std::string& a_SrcDir, std::string& a_path, bool a_IsKeepAlive = false, int a_code = -1);
    void MakeResponse(cBuffer& a_WriteBuffer);
    void UnMapFile();
    size_t FileLen() const;
    char* File() const;

    void ErrorContent(cBuffer& a_WriteBuffer, std::string a_Message);
    int Code() const { return m_code; }

private:
    void AddStateLine(cBuffer& a_WriteBuffer);
    void AddHeader(cBuffer& a_WriteBuffer);
    void AddContent(cBuffer& a_WriteBuffer);

    void ErrorHtml();
    std::string GetFileType();

    int m_code;
    bool m_IsKeepAlive;

    std::string m_path;
    std::string m_SrcDir;
    
    char* m_mmFile; 
    struct stat m_mmFileStat;

    static const std::unordered_map<std::string, std::string> m_SuffixType;
    static const std::unordered_map<int, std::string> m_CodeStatus;
    static const std::unordered_map<int, std::string> m_CodePath;

};

#endif  // HTTPRESPONDSE_H