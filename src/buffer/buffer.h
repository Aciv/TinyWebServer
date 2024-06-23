#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>  //perror
#include <iostream>
#include <unistd.h>  // write
#include <sys/uio.h>  //readv
#include <vector>  //readv
#include <atomic>
#include <assert.h>

class cBuffer {
public:
    cBuffer(int a_InitBuffSize = 1024);
    ~cBuffer() = default;

    size_t WritableBytes() const;
    size_t ReadableBytes() const ;
    size_t PrependableBytes() const;
    const char* Peek() const;
    
    void EnsureWriteable(size_t a_len);
    void HasWritten(size_t a_len);

    void Retrieve(size_t a_len);
    void RetrieveUntil(const char* a_end);

    void RetrieveAll() ;
    std::string RetrieveAllToStr();

    const char* BeginWriteConst() const;
    char* BeginWrite();

    void Append(const std::string& a_str);
    void Append(const char* a_str, size_t a_len);
    void Append(const void* a_data, size_t a_len);
    void Append(const cBuffer& a_buff);

    ssize_t ReadFd(int a_fd, int* a_Errno);
    ssize_t WriteFd(int a_fd, int* a_Errno);

private:
    char* BeginPtr();
    const char* BeginPtr() const;
    void MakeSpace(size_t a_len);

    std::vector<char> m_Buffer;
    std::atomic<std::size_t> m_ReadPos;
    std::atomic<std::size_t> m_WritePos;
};

#endif //BUFFER_H