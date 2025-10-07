
#include "../Buffer.h"
#include "../Logger.h"
#include "HttpContext.h"

bool HttpContext::processRequestLine(const char *begin, const char *end)
{
  bool succeed = false;
  const char *start = begin;
  const char *space = std::find(start, end, ' ');
  if (space != end && request_.setMethod(start, space))
  {
    start = space + 1;
    space = std::find(start, end, ' ');
    if (space != end)
    {
      const char *question = std::find(start, space, '?');
      if (question != space)
      {
        request_.setPath(start, question);
        request_.setQuery(question, space);
      }
      else
      {
        request_.setPath(start, space);
      }
      start = space + 1;
      succeed = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
      if (succeed)
      {
        if (*(end - 1) == '1')
        {
          request_.setVersion(HttpRequest::kHttp11);
        }
        else if (*(end - 1) == '0')
        {
          request_.setVersion(HttpRequest::kHttp10);
        }
        else
        {
          succeed = false;
        }
      }
    }
  }
  return succeed;
}

// return false if any error
bool HttpContext::parseRequest(Buffer *buf, Timestamp receiveTime)
{
  bool ok = true;
  bool hasMore = true;
  while (hasMore)
  {
    if (state_ == kExpectRequestLine)
    {
      const char *crlf = buf->findCRLF();
      if (crlf)
      {
        ok = processRequestLine(buf->peek(), crlf);
        if (ok)
        {
          request_.setReceiveTime(receiveTime);
          buf->retrieveUntil(crlf + 2);
          state_ = kExpectHeaders;
        }
        else
        {
          hasMore = false;
        }
      }
      else
      {
        hasMore = false;
      }
    }
    else if (state_ == kExpectHeaders)
    {
      const char *crlf = buf->findCRLF();
      if (crlf)
      {
        const char *colon = std::find(buf->peek(), crlf, ':');
        if (colon != crlf)
        {
          request_.addHeader(buf->peek(), colon, crlf);
        }
        else
        {
          // empty line, end of header
          // FIXME:
          if (request_.method() == HttpRequest::kPost && buf->readableBytes() > 0)
          {
            state_ = kExpectBody;
          }
          else
          {
            state_ = kGotAll;
            hasMore = false;
          }
        }
        buf->retrieveUntil(crlf + 2);
      }
      else
      {
        hasMore = false;
      }
    }
    else if (state_ == kExpectBody)
    {
      // FIXME:
      LOG_INFO("kExpectBody, readableBytes: %d", 1);
      const std::string &contentLength = request_.getHeader("Content-Length");
      if (!contentLength.empty())
      {
        int contentLengthInt = atoi(contentLength.c_str());
        if (buf->readableBytes() >= contentLengthInt)
        {
          request_.setBody(buf->peek(), buf->peek() + contentLengthInt);
          buf->retrieve(contentLengthInt);
          state_ = kGotAll;
          hasMore = false;
        }
      }
      else
      {
        // 无Content-Length头，假设为短连接，直接处理
        request_.setBody(buf->retrieveAllAsString());
        state_ = kGotAll;
        hasMore = false;
      }
    }
  }
  return ok;
}

bool HttpContext::parseResponse(Buffer *buf, Timestamp receiveTime)
{
  bool ok = true;
  bool hasMore = true;

  // std::vector<char> &buffer = buf->buffer();
  // Buffer buf1 = *buf;
  // std::string ss = buf1.retrieveAllAsString();
  // LOG_INFO("parseResponse: %d", buffer.size());
  // LOG_INFO("parseResponse: %s", ss.c_str());

  while (hasMore)
  {
    if (state_ == kExpectRequestLine)
    {
      // 解析响应行，格式如: HTTP/1.1 200 OK
      const char *crlf = buf->findCRLF();
      if (crlf)
      {
        const char *start = buf->peek();
        const char *space1 = std::find(start, crlf, ' ');

        if (space1 != crlf)
        {
          // 解析HTTP版本
          if (space1 - start == 8 && std::equal(start, start + 5, "HTTP/"))
          {
            if (*(start + 5) == '1' && *(start + 6) == '.' && (*(start + 7) == '0' || *(start + 7) == '1'))
            {
              // 设置版本信息（这里简化处理，实际应该在HttpResponse中添加版本字段）
            }
            else
            {
              ok = false;
              hasMore = false;
              break;
            }
          }
          else
          {
            ok = false;
            hasMore = false;
            break;
          }

          // 解析状态码
          const char *space2 = std::find(space1 + 1, crlf, ' ');
          if (space2 != crlf)
          {
            // 提取状态码
            std::string statusCodeStr(space1 + 1, space2);
            int statusCode = atoi(statusCodeStr.c_str());
            // 设置状态码
            response_.setStatusCode(statusCode);

            // 这里应该设置HttpResponse的状态码，但由于当前设计是基于HttpRequest的，
            // 我们暂时跳过这一步，实际应用中应该有HttpResponse对象来存储这些信息

            // 提取状态消息
            std::string statusMessage(space2 + 1, crlf);

            buf->retrieveUntil(crlf + 2);
            state_ = kExpectHeaders;
          }
          else
          {
            ok = false;
            hasMore = false;
          }
        }
        else
        {
          ok = false;
          hasMore = false;
        }
      }
      else
      {
        hasMore = false;
      }
    }
    else if (state_ == kExpectHeaders)
    {
      // LOG_INFO("kExpectHeaders");
      const char *crlf = buf->findCRLF();
      if (crlf)
      {
        const char *colon = std::find(buf->peek(), crlf, ':');
        if (colon != crlf)
        {
          // 解析响应头
          std::string headerName(buf->peek(), colon);
          std::string headerValue(colon + 1, crlf);
          // LOG_INFO("headerName: %s, headerValue: %s", headerName.c_str(), headerValue.c_str());
          response_.addHeader(headerName, headerValue);
        }
        else
        {
          // 空行，表示响应头结束
          // 判断是否有body
          if (buf->readableBytes() > 0)
          {
            state_ = kExpectBody;
          }
          else
          {
            state_ = kGotAll;
            hasMore = false;
          }
        }
        buf->retrieveUntil(crlf + 2);
      }
      else
      {
        hasMore = false;
      }
    }
    else if (state_ == kExpectBody)
    {
      // LOG_INFO("kExpectBody");
      // 处理响应体（如果需要）
      // 这里需要根据Content-Length或者Transfer-Encoding来判断body的长度
      // 由于当前设计限制，我们暂时不处理body
      // 实际应用中，应该根据Content-Length或Transfer-Encoding来判断body的长度
      // 并在kExpectBody状态下，根据长度读取body数据
      // 读取完成后，设置state_为kGotAll
      // 这里只是简单地假设body为空

      // 找到body结束的位置
      // const char *crlf = buf->findCRLF();
      // LOG_INFO("crlf: %s", crlf);
      // const char *start = buf->peek();
      // LOG_INFO("body: %d", crlf - start);
      // if (crlf)
      // {
      std::string body = buf->retrieveAllAsString();
      // const char *start = buf->peek();
      // LOG_INFO("body: %d", crlf - start);
      response_.setBody(body);
      // }
      state_ = kGotAll;
      hasMore = false;
    }
  }

  return ok;
}