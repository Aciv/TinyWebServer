#include "httpRequest.h"

const std::unordered_set<std::string> cHttpRequest::m_DefaultHtml{
            "/index", "/register", "/login",
             "/welcome", "/video", "/picture", };

const std::unordered_map<std::string, int> cHttpRequest::m_DefaultHtmlTag {
            {"/register.html", 0}, {"/login.html", 1},  };

void cHttpRequest::Init(){
    m_method = m_path = m_Version = m_body = "";
    m_State = eParseState::REQUEST_LINE;
    m_Header.clear();
    m_Post.clear();
}

bool cHttpRequest::IsKeepAlive() const {
    if(m_Header.count("Connection") == 1) {
        return m_Header.find("Connection")->second == "keep-alive" && m_Version == "1.1";
    }
    return false;
}

std::string cHttpRequest::Method() const {
    return m_method;
}
std::string cHttpRequest::Version() const {
    return m_Version;
}

std::string cHttpRequest::GetPost(const std::string& a_key) const {
    assert(a_key != "");
    if(m_Post.count(a_key) == 1) {
        return m_Post.find(a_key)->second;
    }
    return "";
}
std::string cHttpRequest::GetPost(const char* a_key) const {
    assert(a_key != nullptr);
    if(m_Post.count(a_key) == 1) {
        return m_Post.find(a_key)->second;
    }
    return "";
}

std::string cHttpRequest::Path() const {
    return m_path;
}
std::string& cHttpRequest:: Path() {
    return m_path;
}

bool cHttpRequest::Parse(cBuffer &a_ReadBuffer) {
    const char CRLF[] = "\r\n";
    if(a_ReadBuffer.ReadableBytes() <= 0) {
        return false;
    }

    while(a_ReadBuffer.ReadableBytes() && m_State != eParseState::FINISH) {
        const char* lineEnd = std::search(a_ReadBuffer.Peek(), a_ReadBuffer.BeginWriteConst(), CRLF, CRLF + 2);
    

        std::string line(a_ReadBuffer.Peek(), lineEnd);
        switch(m_State)
        {
        case eParseState::REQUEST_LINE:
            if(!ParseRequestLine(line)) {
                return false;
            }
            ParsePath();
            break;    
        case eParseState::HEADERS:
            ParseHeader(line);
            if(a_ReadBuffer.ReadableBytes() <= 2) {
                m_State = eParseState::FINISH;
            }
            break;
        case eParseState::BODY:
            ParseBody(line);
            break;
        default:
            break;
        }
        if(lineEnd == a_ReadBuffer.BeginWrite()) { break; }
        a_ReadBuffer.RetrieveUntil(lineEnd + 2);
    }

    LOG_DEBUG("[%s], [%s], [%s]", m_method.c_str(), m_path.c_str(), m_Version.c_str());
    return true;
}

bool cHttpRequest::ParseRequestLine(const std::string& a_line){
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if(std::regex_match(a_line, subMatch, patten)) {   
        m_method = subMatch[1];
        m_path = subMatch[2];
        m_Version = subMatch[3];
        m_State = eParseState::HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void cHttpRequest::ParsePath() {
    if(m_path == "/") {
        m_path = "/index.html"; 
    }
    else {
        for(auto &item: m_DefaultHtml) {
            if(item == m_path) {
                m_path += ".html";
                break;
            }
        }
    }
}

void cHttpRequest::ParseHeader(const std::string& a_line) {
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if(regex_match(a_line, subMatch, patten)) {
        m_Header[subMatch[1]] = subMatch[2];
    }
    else {
        m_State = eParseState::BODY;
    }
}

void cHttpRequest::ParseBody(const std::string& a_line) {
    m_body= a_line;
    ParsePost();
    m_State = eParseState::FINISH;
    LOG_DEBUG("Body:%s, len:%d", a_line.c_str(), a_line.size());
}

void cHttpRequest::ParsePost() {
    if(m_method == "POST" && m_Header["Content-Type"] == "application/x-www-form-urlencoded") {
        ParseFromUrlencoded();
        if(m_DefaultHtmlTag.count(m_path)){
            int tag = m_DefaultHtmlTag.find(m_path)->second;
            LOG_DEBUG("Tag:%d", tag);
            if(tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if(UserVerify(m_Post["username"], m_Post["password"], isLogin)) {
                    m_path = "/welcome.html";
                } 
                else {
                    m_path = "/error.html";
                }
            }
        }
    }   
}
void cHttpRequest::ParseFromUrlencoded() {
    if(m_body.size() == 0) { return; }

    std::string key, value;
    int num = 0;
    int n = m_body.size();
    int i = 0, j = 0;

    for(; i < n; i++) {
        char ch = m_body[i];
        switch (ch) {
        case '=':
            key = m_body.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            m_body[i] = ' ';
            break;
        case '%':
            num = ConverHex(m_body[i + 1]) * 16 + ConverHex(m_body[i + 2]);
            m_body[i + 2] = num % 10 + '0';
            m_body[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = m_body.substr(j, i - j);
            j = i + 1;
            m_Post[key] = value;
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    if(m_Post.count(key) == 0 && j < i) {
        value = m_body.substr(j, i - j);
        m_Post[key] = value;
    }
}

bool cHttpRequest::UserVerify(const std::string& a_name, const std::string& a_pwd, bool a_IsLogin){
    if(a_name == "" || a_pwd == "") { return false; }
    LOG_INFO("Verify name:%s pwd:%s", a_name.c_str(), a_pwd.c_str());
    MYSQL* sql;
    cSqlConnRAII(&sql,  cSqlConnPool::Instance());
    assert(sql);
    
    bool flag = false;
    unsigned int j = 0;
    char order[256] = { 0 };
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;
    
    if(!a_IsLogin) { flag = true; }
    /* 查询用户及密码 */
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", a_name.c_str());
    LOG_DEBUG("%s", order);

    if(mysql_query(sql, order)) { 
        mysql_free_result(res);
        return false; 
    }
    
    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    while(MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        std::string password(row[1]);
        /* 注册行为 且 用户名未被使用*/
        if(a_IsLogin) {
            if(a_pwd == password) { flag = true; }
            else {
                flag = false;
                LOG_DEBUG("pwd error!");
            }
        } 
        else { 
            flag = false; 
            LOG_DEBUG("user used!");
        }
    }
    mysql_free_result(res);

    /* 注册行为 且 用户名未被使用*/
    if(!a_IsLogin && flag == true) {
        LOG_DEBUG("regirster!");
        bzero(order, 256);
        snprintf(order, 256,"INSERT INTO user(username, password) VALUES('%s','%s')", a_name.c_str(), a_pwd.c_str());
        LOG_DEBUG( "%s", order);
        if(mysql_query(sql, order)) { 
            LOG_DEBUG( "Insert error!");
            flag = false; 
        }
        flag = true;
    }
    cSqlConnPool::Instance()->FreeConn(sql);
    LOG_DEBUG( "UserVerify success!!");
    return flag;
}

int cHttpRequest::ConverHex(char a_ch) {
    if(a_ch >= 'A' && a_ch <= 'F') return a_ch -'A' + 10;
    if(a_ch >= 'a' && a_ch <= 'f') return a_ch -'a' + 10;
    return a_ch;
}