#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>     
#include <mysql/mysql.h>  //mysql

#include "../buffer/buffer.h"
#include "../log/log.h"

class cHttpRequest{
public:
    enum class eParseState {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,        
    };

    enum eHttp_Code {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };
    
    cHttpRequest() { Init(); }
    ~cHttpRequest() = default;

    void Init();
    bool Parse(cBuffer &a_ReadBuffer);

    std::string Method() const;
    std::string Version() const;
    std::string GetPost(const std::string& a_key) const;
    std::string GetPost(const char* a_key) const;
    std::string Path() const;
    std::string& Path();

    bool IsKeepAlive() const;

private:
    bool ParseRequestLine(const std::string& a_line);
    void ParseHeader(const std::string& a_line);
    void ParseBody(const std::string& a_line);

    void ParsePath();
    void ParsePost();
    void ParseFromUrlencoded();

    static bool UserVerify(const std::string& a_name, const std::string& a_pwd, bool a_IsLogin);

    eParseState m_State;
    std::string m_method, m_path, m_Version, m_body;
    std::unordered_map<std::string, std::string> m_Header;
    std::unordered_map<std::string, std::string> m_Post;

    static const std::unordered_set<std::string> m_DefaultHtml;
    static const std::unordered_map<std::string, int> m_DefaultHtmlTag;
    static int ConverHex(char a_ch);
};

#endif  // HTTPREQUEST_H