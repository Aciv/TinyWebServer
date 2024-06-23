#include "httpResponse.h"

const std::unordered_map<std::string, std::string> cHttpResponse::m_SuffixType = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const std::unordered_map<int, std::string> cHttpResponse::m_CodeStatus = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const std::unordered_map<int, std::string> cHttpResponse::m_CodePath = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

cHttpResponse::cHttpResponse() {
    m_code = -1;
    m_path = m_SrcDir = "";
    m_IsKeepAlive = false;
    m_mmFile = nullptr; 
    m_mmFileStat = { 0 };
};

cHttpResponse::~cHttpResponse() {
    UnMapFile();
}

void cHttpResponse::Init(const std::string& a_SrcDir, std::string& a_path, bool a_IsKeepAlive, int a_code) {
    assert(a_SrcDir != "");
    if(m_mmFile) { UnMapFile(); }
    m_code = a_code;
    m_IsKeepAlive = a_IsKeepAlive;
    m_path = a_path;
    m_SrcDir = a_SrcDir;
    m_mmFile = nullptr; 
    m_mmFileStat = { 0 };
}

char* cHttpResponse::File() const {
    return m_mmFile;
}

size_t cHttpResponse::FileLen() const {
    return m_mmFileStat.st_size;
}

void cHttpResponse::MakeResponse(cBuffer& a_WriteBuffer) {
    /* 判断请求的资源文件 */
    if(stat((m_SrcDir + m_path).data(), &m_mmFileStat) < 0 || S_ISDIR(m_mmFileStat.st_mode)) {
        m_code = 404;
    }
    else if(!(m_mmFileStat.st_mode & S_IROTH)) {
        m_code = 403;
    }
    else if(m_code == -1) { 
        m_code = 200; 
    }
    ErrorHtml();
    AddStateLine(a_WriteBuffer);
    AddHeader(a_WriteBuffer);
    AddContent(a_WriteBuffer);
}

void cHttpResponse::ErrorHtml() {
    if(m_CodePath.count(m_code) == 1) {
        m_path = m_CodePath.find(m_code)->second;
        stat((m_SrcDir + m_path).data(), &m_mmFileStat);
    }
}

void cHttpResponse::AddStateLine(cBuffer& a_WriteBuffer) {
    std::string Status;
    if(m_CodeStatus.count(m_code) == 1) {
        Status = m_CodeStatus.find(m_code)->second;
    }
    else {
        m_code = 400;
        Status = m_CodeStatus.find(400)->second;
    }
    a_WriteBuffer.Append("HTTP/1.1 " + std::to_string(m_code) + " " + Status + "\r\n");
}

void cHttpResponse::AddHeader(cBuffer& a_WriteBuffer) {
    a_WriteBuffer.Append("Connection: ");
    if(m_IsKeepAlive) {
        a_WriteBuffer.Append("keep-alive\r\n");
        a_WriteBuffer.Append("keep-alive: max=6, timeout=120\r\n");
    } else{
        a_WriteBuffer.Append("close\r\n");
    }
    a_WriteBuffer.Append("Content-type: " + GetFileType() + "\r\n");
}

void cHttpResponse::AddContent(cBuffer& a_WriteBuffer) {
    int srcFd = open((m_SrcDir + m_path).data(), O_RDONLY);
    if(srcFd < 0) { 
        ErrorContent(a_WriteBuffer, "File NotFound!");
        return; 
    }

    /* 将文件映射到内存提高文件的访问速度 
        MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
    LOG_DEBUG("file path %s", (m_SrcDir + m_path).data());
    int* mmRet = (int*)mmap(0, m_mmFileStat.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if(*mmRet == -1) {
        ErrorContent(a_WriteBuffer, "File NotFound!");
        return; 
    }
    m_mmFile = (char*)mmRet;
    close(srcFd);
    a_WriteBuffer.Append("Content-length: " + std::to_string(m_mmFileStat.st_size) + "\r\n\r\n");
}

void cHttpResponse::UnMapFile() {
    if(m_mmFile) {
        munmap(m_mmFile, m_mmFileStat.st_size);
        m_mmFile = nullptr;
    }
}

std::string cHttpResponse::GetFileType() {
    /* 判断文件类型 */
    std::string::size_type idx = m_path.find_last_of('.');
    if(idx == std::string::npos) {
        return "text/plain";
    }
    std::string suffix = m_path.substr(idx);
    if(m_SuffixType.count(suffix) == 1) {
        return m_SuffixType.find(suffix)->second;
    }
    return "text/plain";
}

void cHttpResponse::ErrorContent(cBuffer& a_WriteBuffer, std::string a_Message) 
{
    std::string body;
    std::string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(m_CodeStatus.count(m_code) == 1) {
        status = m_CodeStatus.find(m_code)->second;
    } else {
        status = "Bad Request";
    }
    body += std::to_string(m_code) + " : " + status  + "\n";
    body += "<p>" + a_Message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    a_WriteBuffer.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    a_WriteBuffer.Append(body);
}
