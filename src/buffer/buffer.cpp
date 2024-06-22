#include "buffer.h"

cBuffer::cBuffer(int a_InitBuffSize = 1024) : m_Buffer(a_InitBuffSize),
    m_ReadPos(0),  m_WritePos(0) {
    assert(m_Buffer.size() > 0);
}

char* cBuffer::BeginPtr(){
    return &m_Buffer[0];
}
const char* cBuffer::BeginPtr() const{
    return &m_Buffer[0];
}

size_t cBuffer::WritableBytes() const{
    return m_Buffer.size() - m_WritePos;
}
size_t cBuffer::ReadableBytes() const {
    return m_WritePos - m_ReadPos;
}
size_t cBuffer::PrependableBytes() const{
    return m_ReadPos;
}

const char* cBuffer::Peek() const{
    return BeginPtr() + m_ReadPos;
}

void cBuffer::Retrieve(size_t a_len){
    assert(a_len <= ReadableBytes());
    m_ReadPos += a_len;
}

void cBuffer::RetrieveUntil(const char* a_end){
    assert(a_end >= Peek());
    Retrieve(a_end - Peek());
}

void cBuffer::RetrieveAll(){
    bzero(&m_Buffer[0], m_Buffer.size());
    m_ReadPos = m_WritePos = 0;
}

std::string cBuffer:: RetrieveAllToStr(){
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

const char* cBuffer::BeginWriteConst() const {
    return BeginPtr() + m_WritePos;
}

char* cBuffer::BeginWrite() {
    return BeginPtr() + m_WritePos;
}

void cBuffer::EnsureWriteable(size_t a_len){
    if(a_len > WritableBytes())
        MakeSpace(a_len);
    
    assert(a_len <= WritableBytes());
}

void cBuffer::MakeSpace(size_t a_len){
    if(m_ReadPos + WritableBytes() >= a_len){
        size_t LastReadable = ReadableBytes();
        std::copy(BeginPtr() + m_ReadPos, BeginPtr() + m_WritePos, BeginPtr());
        m_ReadPos = 0;
        m_WritePos = m_ReadPos + LastReadable;
        assert(LastReadable == ReadableBytes());
    }
    else{
        m_Buffer.resize(m_WritePos + a_len + 1);
    }

}

void cBuffer::HasWritten(size_t a_len){
    m_WritePos += a_len;
}

void cBuffer::Append(const std::string& a_str){
    Append(a_str.data(), a_str.length());
}

void cBuffer::Append(const char* a_str, size_t a_len){
    assert(a_str);
    EnsureWriteable(a_len);
    std::copy(a_str, a_str + a_len, BeginWrite());
    HasWritten(a_len);
}

void cBuffer::Append(const void* a_data, size_t a_len){
    assert(a_data);
    Append(static_cast<const char *>(a_data), a_len);
}

void cBuffer::Append(const cBuffer& a_buff){
    Append(a_buff.Peek(), a_buff.ReadableBytes());
}

ssize_t cBuffer::ReadFd(int a_fd, int* a_Errno){
    assert(a_fd > 0);
    char buff[65535];
    struct iovec iov[2];
    const size_t Writable = WritableBytes();
    /* 分散读， 保证数据全部读完 */
    iov[0].iov_base = BeginPtr() + m_WritePos;
    iov[0].iov_len = Writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(a_fd, iov, 2);
    if(len < 0) {
        *a_Errno = errno;
        return len;
    }
    else if(static_cast<size_t>(len) <= Writable) {
        m_ReadPos += len;
    }
    else {
        m_WritePos = m_Buffer.size();
        Append(buff, len - Writable);
    }
    return len;
}
ssize_t cBuffer::WriteFd(int a_fd, int* a_Errno){
    assert(a_fd > 0);
    size_t readSize = ReadableBytes();
    ssize_t len = write(a_fd, Peek(), readSize);
    if(len < 0) {
        *a_Errno = errno;
        return len;
    } 
    m_ReadPos += len;
    return len;
}