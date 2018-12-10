#define NOMINMAX
#include "http.h"
#include "common.h"
#include <algorithm>

#ifndef USE_WINHTTP

#ifdef _MSC_VER

#ifdef _DEBUG
#pragma comment(lib, "libcurld.lib")
#pragma comment(lib, "libssh2d.lib")
#else
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "libssh2.lib")
#endif

#pragma comment(lib, "libeay32.lib")
#pragma comment(lib, "ssleay32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "wldap32.lib")
#endif

#include <curl/curl.h>
#endif

void HttpRequest::addHeader(std::string const& name, std::string const& value) {
  addHeader(name + ": " + value);
}

static std::string urlencode(std::string const& str) {
  std::string dst;
  for (unsigned char c : str) {
    if (isalnum(c)) {
      dst.push_back(c);
    } else if (c == ' ') {
      dst.push_back('+');
    } else {
      char hex[8];
      sprintf(hex, "%%%02X", c);
      dst.append(hex);
    }
  }
  return dst;
}

void HttpRequest::addData(std::string const& key, std::string const& value) {
  if (!post_.empty()) post_.push_back('&');
  post_.append(urlencode(key));
  post_.push_back('=');
  post_.append(urlencode(value));
}

#ifdef USE_WINHTTP

#pragma comment(lib, "wininet.lib")

HttpRequest::SessionHolder::~SessionHolder() {
  if (request) InternetCloseHandle(request);
  if (connect) InternetCloseHandle(connect);
  if (session) InternetCloseHandle(session);
}

HttpRequest::HttpRequest(std::string const& url, RequestType type)
  : type_(type)
  , handles_(new SessionHolder)
{
  URL_COMPONENTS urlComp;
  memset(&urlComp, 0, sizeof urlComp);
  urlComp.dwStructSize = sizeof urlComp;
  urlComp.dwSchemeLength = -1;
  urlComp.dwHostNameLength = -1;
  urlComp.dwUrlPathLength = -1;
  urlComp.dwExtraInfoLength = -1;

  if (!InternetCrackUrl(url.c_str(), url.size(), 0, &urlComp)) {
    return;
  }

  std::string host(urlComp.lpszHostName, urlComp.dwHostNameLength);
  std::string path(urlComp.lpszUrlPath, urlComp.dwUrlPathLength + urlComp.dwExtraInfoLength);

  handles_->session = InternetOpen("SNOParser", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
  if (!handles_->session) return;
  handles_->connect = InternetConnect(handles_->session, host.c_str(), urlComp.nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);
  if (!handles_->connect) return;
  handles_->request = HttpOpenRequest(
    handles_->connect, type == GET ? "GET" : "POST", path.c_str(), "HTTP/1.1", NULL, NULL,
    (urlComp.nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_FLAG_SECURE : 0) | INTERNET_FLAG_RELOAD, NULL);
  if (handles_->request) {
    BOOL value = TRUE;
    InternetSetOption(handles_->request, INTERNET_OPTION_HTTP_DECODING, &value, sizeof value);
  }
}

void HttpRequest::addHeader(std::string const& header) {
  headers_.append(header);
  headers_.append("\r\n");
}

bool HttpRequest::send() {
  if (!handles_->request) return false;
  return HttpSendRequest(handles_->request,
    headers_.empty() ? nullptr : headers_.c_str(), headers_.size(),
    post_.empty() ? nullptr : &post_[0], post_.size());
}

uint32 HttpRequest::status() {
  if (!handles_->request) return 0;

  DWORD statusCode = 0;
  DWORD size = sizeof statusCode;
  HttpQueryInfo(handles_->request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
    &statusCode, &size, NULL);
  return statusCode;
}

class HttpBuffer : public FileBuffer {
  std::shared_ptr<HttpRequest::SessionHolder> handles_;
  size_t size_;
  uint8* data_;
  size_t pos_;
  size_t loaded_;
public:
  HttpBuffer(std::shared_ptr<HttpRequest::SessionHolder> const& handles, size_t size)
    : handles_(handles)
    , size_(size)
    , pos_(0)
    , loaded_(0)
  {
    data_ = new uint8[size];
  }
  ~HttpBuffer() {
    delete[] data_;
  }

  int getc() {
    if (pos_ < loaded_) {
      return data_[pos_++];
    } else if (pos_ >= size_) {
      return EOF;
    } else {
      uint8 chr;
      read(&chr, 1);
      return chr;
    }
  }

  uint64 tell() const {
    return pos_;
  }
  void seek(int64 pos, int mode) {
    switch (mode) {
    case SEEK_CUR:
      pos += pos_;
      break;
    case SEEK_END:
      pos += size_;
      break;
    }
    if (pos < 0) pos = 0;
    if (pos > size_) pos = size_;
    pos_ = pos;
  }
  uint64 size() {
    return size_;
  }

  size_t read(void* ptr, size_t size) {
    if (size + pos_ > size_) {
      size = size_ - pos_;
    }
    while (pos_ + size > loaded_) {
      DWORD nsize = 0, nread;
      if (!InternetQueryDataAvailable(handles_->request, &nsize, 0, 0) || !nsize) break;
      InternetReadFile(handles_->request, data_ + loaded_, std::min<DWORD>(nsize, size_ - loaded_), &nread);
      loaded_ += nread;
    }
    if (size + pos_ > loaded_) {
      size = loaded_ - pos_;
    }
    if (size) {
      memcpy(ptr, data_ + pos_, size);
      pos_ += size;
    }
    return size;
  }

  size_t write(void const* ptr, size_t size) {
    return 0;
  }
};

File HttpRequest::response() {
  if (!handles_->request) return File();

  DWORD contentLength = 0;
  DWORD size = (sizeof contentLength);
  HttpQueryInfo(handles_->request, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
    &contentLength, &size, NULL);

  if (contentLength) {
    return File(std::make_shared<HttpBuffer>(handles_, contentLength));
  } else {
    DWORD size = 0, read;
    MemoryFile file;
    do {
      if (!InternetQueryDataAvailable(handles_->request, &size, 0, 0)) return File();
      if (!size) break;
      size_t cur_size = file.size();
      if (!InternetReadFile(handles_->request, file.alloc(size), size, &read)) return File();
      if (read < size) file.resize(cur_size + read);
    } while (size);
    file.seek(0);
    return file;
  }
}

std::map<std::string, std::string> HttpRequest::headers() {
  std::map<std::string, std::string> result;
  if (!handles_->request) return result;
  DWORD size;
  HttpQueryInfo(handles_->request, HTTP_QUERY_RAW_HEADERS, nullptr, &size, NULL);
  if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return result;
  typedef char char_t;
  std::vector<char_t> raw((size + 1) / sizeof(char_t));
  HttpQueryInfo(handles_->request, HTTP_QUERY_RAW_HEADERS, &raw[0], &size, NULL);
  std::string cur;
  for (char_t wc : raw) {
    if (wc) {
      cur.push_back(wc);
    } else if (!cur.empty()) {
      std::string line = cur;// utf16_to_utf8(cur);
      cur.clear();

      size_t colon = line.find(':');
      if (colon == std::string::npos) continue;
      result.emplace(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
    }
  }
  return result;
}

#else

HttpRequest::HttpRequest(std::string const& url, RequestType type)
  : url_(url)
  , response_(new Response)
  , type_(type)
{
}

HttpRequest::Response::~Response() {
  if (request_headers) {
    curl_slist_free_all(request_headers);
  }
}

void HttpRequest::addHeader(std::string const& header) {
  response_->request_headers = curl_slist_append(response_->request_headers, header.c_str());
}

size_t string_writer(char* ptr, size_t size, size_t count, void* data) {
  std::string* buffer = reinterpret_cast<std::string*>(data);
  size *= count;
  buffer->append(ptr, size);
  return size;
}

size_t file_writer(char* ptr, size_t size, size_t count, void* data) {
  MemoryFile* file = reinterpret_cast<MemoryFile*>(data);
  return file->write(ptr, size * count);
}

bool HttpRequest::send() {
  CURL* curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url_.c_str());
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
  if (response_->request_headers) {
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, response_->request_headers);
  }
  if (type_ == POST) {
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, post_.size());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_.data());
  }
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, string_writer);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_->headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, file_writer);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_->data);
  CURLcode res = curl_easy_perform(curl);
  if (res == CURLE_OK) {
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_->code);
  }
  curl_easy_cleanup(curl);
  response_->data.seek(0);
  return res == CURLE_OK;
}

uint32 HttpRequest::status() {
  return (response_ ? response_->code : 0);
}

File HttpRequest::response() {
  return (response_ ? response_->data : File());
}

std::map<std::string, std::string> HttpRequest::headers() {
  std::map<std::string, std::string> result;
  if (!response_) return result;
  size_t pos = 0;
  size_t next;
  std::string& headers = response_->headers;
  while ((next = headers.find("\r\n", pos)) != std::string::npos) {
    size_t sep = headers.find(": ", pos);
    if (sep != std::string::npos && sep < next) {
      result.emplace(headers.substr(pos, sep - pos), headers.substr(sep + 2, next - sep - 2));
    }
    pos = next + 2;
  }
  return result;
}

#endif

File HttpRequest::get(std::string const& url) {
  HttpRequest request(url);
  if (!request.send() || request.status() != 200) return File();
  return request.response();
}
