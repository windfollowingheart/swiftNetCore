#pragma once

#include "../Types.h"

#include <map>

class Buffer;
class HttpResponse
{
public:
  enum HttpStatusCode
  {
    kUnknown,
    k200Ok = 200,
    k301MovedPermanently = 301,
    k400BadRequest = 400,
    k404NotFound = 404,
  };

  HttpResponse()
      : statusCode_(kUnknown),
        closeConnection_(false)
  {
  }

  explicit HttpResponse(bool close)
      : statusCode_(kUnknown),
        closeConnection_(close)
  {
  }

  void setStatusCode(HttpStatusCode code)
  {
    statusCode_ = code;
  }

  void setStatusCode(int code)
  {
    if (code == 200)
    {
      statusCode_ = k200Ok;
    }
    else if (code == 301)
    {
      statusCode_ = k301MovedPermanently;
    }
    else if (code == 400)
    {
      statusCode_ = k400BadRequest;
    }
    else if (code == 404)
    {
      statusCode_ = k404NotFound;
    }
    else
    {
      statusCode_ = kUnknown;
    }
  }

  HttpStatusCode statusCode() const
  {
    return statusCode_;
  }

  void setStatusMessage(const string &message)
  {
    statusMessage_ = message;
  }

  void setCloseConnection(bool on)
  {
    closeConnection_ = on;
  }

  bool closeConnection() const
  {
    return closeConnection_;
  }

  void setContentType(const string &contentType)
  {
    addHeader("Content-Type", contentType);
  }

  // FIXME: replace string with StringPiece
  void addHeader(const string &key, const string &value)
  {
    headers_[key] = value;
  }

  std::map<string, string> headers() const
  {
    return headers_;
  }

  void setBody(const string &body)
  {
    body_ = body;
  }

  string body() const
  {
    return body_;
  }

  void appendToBuffer(Buffer *output) const;

private:
  std::map<string, string> headers_;
  HttpStatusCode statusCode_;
  // FIXME: add http version
  string statusMessage_;
  bool closeConnection_;
  string body_;
};
