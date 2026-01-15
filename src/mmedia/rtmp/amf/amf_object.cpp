#include "mmedia/rtmp/amf/amf_object.h"
#include "mmedia/base/bytes_reader.h"
#include "mmedia/base/mmedia_logger.h"
#include "mmedia/rtmp/amf/amf_boolean.h"
#include "mmedia/rtmp/amf/amf_date.h"
#include "mmedia/rtmp/amf/amf_longstring.h"
#include "mmedia/rtmp/amf/amf_number.h"
#include "mmedia/rtmp/amf/amf_string.h"

using namespace lssvc::mmedia;

namespace {
static AMFAnyPtr any_ptr_null;
}

AMFObject::AMFObject(const std::string &name) : AMFAny(name) {}
AMFObject::AMFObject() {}
AMFObject::~AMFObject() {}

int AMFObject::decode(const char *data, int size, bool has) {
  std::string nname;
  int32_t parsed = 0;

  while ((parsed + 3) <= size) {
    if (BytesReader::readUint24T(data) == 0x000009) {
      parsed += 3;
      return parsed;
    }
    if (has) {
      nname = decodeString(data);
      if (!nname.empty()) {
        parsed += (nname.size() + 2);
        data += (nname.size() + 2);
      }
    }
    char type = *data++;
    parsed++;
    switch (type) {
    case kAMFNumber: {
      std::shared_ptr<AMFNumber> p = std::make_shared<AMFNumber>(nname);
      auto len = p->decode(data, size - parsed);
      if (len == -1) {
        return -1;
      }
      data += len;
      parsed += len;
      RTMP_TRACE << "Number value:" << p->number();
      properties_.emplace_back(std::move(p));
      break;
    }
    case kAMFBoolean: {
      std::shared_ptr<AMFBoolean> p = std::make_shared<AMFBoolean>(nname);
      auto len = p->decode(data, size - parsed);
      if (len == -1) {
        return -1;
      }
      data += len;
      parsed += len;
      RTMP_TRACE << "Boolean value:" << p->number();
      properties_.emplace_back(std::move(p));
      break;
    }
    case kAMFString: {
      std::shared_ptr<AMFString> p = std::make_shared<AMFString>(nname);
      auto len = p->decode(data, size - parsed);
      if (len == -1) {
        return -1;
      }
      data += len;
      parsed += len;
      RTMP_TRACE << "String value:" << p->str();
      properties_.emplace_back(std::move(p));
      break;
    }
    case kAMFObject: {
      std::shared_ptr<AMFObject> p = std::make_shared<AMFObject>(nname);
      auto len = p->decode(data, size - parsed, true);
      if (len == -1) {
        return -1;
      }
      data += len;
      parsed += len;
      RTMP_TRACE << "Object ";
      p->dump();
      properties_.emplace_back(std::move(p));
      break;
    }
    case kAMFNull: {
      RTMP_TRACE << "Null.";
      break;
    }
    case kAMFEcmaArray: {
      int count = BytesReader::readUint32T(data);
      parsed += 4;
      data += 4;

      std::shared_ptr<AMFObject> p = std::make_shared<AMFObject>(nname);
      auto len = p->decode(data, size - parsed, true);
      if (len == -1) {
        return -1;
      }
      data += len;
      parsed += len;
      RTMP_TRACE << "EcmaArray ";
      p->dump();
      properties_.emplace_back(std::move(p));
      break;
    }
    case kAMFObjectEnd: {
      return parsed;
    }
    case kAMStrictArray: {
      int count = BytesReader::readUint32T(data);
      parsed += 4;
      data += 4;

      std::shared_ptr<AMFObject> p = std::make_shared<AMFObject>(nname);
      while (count > 0) {
        auto len = p->decodeOnce(data, size - parsed, true);
        if (len == -1) {
          return -1;
        }
        data += len;
        parsed += len;
        count--;
      }
      RTMP_TRACE << "EcmaArray ";
      p->dump();
      properties_.emplace_back(std::move(p));
      break;
    }
    case kAMFDate: {
      std::shared_ptr<AMFDate> p = std::make_shared<AMFDate>(nname);
      auto len = p->decode(data, size - parsed);
      if (len == -1) {
        return -1;
      }
      data += len;
      parsed += len;
      RTMP_TRACE << "Date value:" << p->date();
      properties_.emplace_back(std::move(p));
      break;
    }
    case kAMFLongString: {
      std::shared_ptr<AMFLongString> p = std::make_shared<AMFLongString>(nname);
      auto len = p->decode(data, size - parsed);
      if (len == -1) {
        return -1;
      }
      data += len;
      parsed += len;
      RTMP_TRACE << "LongString value:" << p->str();
      properties_.emplace_back(std::move(p));
      break;
    }
    case kAMFMovieClip:
    case kAMFUndefined:
    case kAMFReference:
    case kAMFUnsupported:
    case kAMFRecordset:
    case kAMFXMLDoc:
    case kAMFTypedObject:
    case kAMFAvmplus: {
      RTMP_TRACE << " not surpport type:" << type;
      break;
    }
    }
  }
  return parsed;
}
bool AMFObject::isObject() { return true; }
AMFObjectPtr AMFObject::object() {
  return std::dynamic_pointer_cast<AMFObject>(shared_from_this());
}
void AMFObject::dump() const {
  RTMP_TRACE << "Object start";
  for (auto const &p : properties_) {
    p->dump();
  }
  RTMP_TRACE << "Object end";
}

int AMFObject::decodeOnce(const char *data, int size, bool has) {
  std::string nname;
  int32_t parsed = 0;

  if (has) {
    nname = decodeString(data);
    if (!nname.empty()) {
      parsed += (nname.size() + 2);
      data += (nname.size() + 2);
    }
  }
  char type = *data++;
  parsed++;
  switch (type) {
  case kAMFNumber: {
    std::shared_ptr<AMFNumber> p = std::make_shared<AMFNumber>(nname);
    auto len = p->decode(data, size - parsed);
    if (len == -1) {
      return -1;
    }
    data += len;
    parsed += len;
    RTMP_TRACE << "Number value:" << p->number();
    properties_.emplace_back(std::move(p));
    break;
  }
  case kAMFBoolean: {
    std::shared_ptr<AMFBoolean> p = std::make_shared<AMFBoolean>(nname);
    auto len = p->decode(data, size - parsed);
    if (len == -1) {
      return -1;
    }
    data += len;
    parsed += len;
    RTMP_TRACE << "Boolean value:" << p->number();
    properties_.emplace_back(std::move(p));
    break;
  }
  case kAMFString: {
    std::shared_ptr<AMFString> p = std::make_shared<AMFString>(nname);
    auto len = p->decode(data, size - parsed);
    if (len == -1) {
      return -1;
    }
    data += len;
    parsed += len;
    RTMP_TRACE << "String value:" << p->str();
    properties_.emplace_back(std::move(p));
    break;
  }
  case kAMFObject: {
    std::shared_ptr<AMFObject> p = std::make_shared<AMFObject>(nname);
    auto len = p->decode(data, size - parsed, true);
    if (len == -1) {
      return -1;
    }
    data += len;
    parsed += len;
    RTMP_TRACE << "Object ";
    p->dump();
    properties_.emplace_back(std::move(p));
    break;
  }
  case kAMFNull: {
    RTMP_TRACE << "Null.";
    break;
  }
  case kAMFEcmaArray: {
    int count = BytesReader::readUint32T(data);
    parsed += 4;
    data += 4;

    std::shared_ptr<AMFObject> p = std::make_shared<AMFObject>(nname);
    auto len = p->decode(data, size - parsed, true);
    if (len == -1) {
      return -1;
    }
    data += len;
    parsed += len;
    RTMP_TRACE << "EcmaArray ";
    p->dump();
    properties_.emplace_back(std::move(p));
    break;
  }
  case kAMFObjectEnd: {
    return parsed;
  }
  case kAMStrictArray: {
    int count = BytesReader::readUint32T(data);
    parsed += 4;
    data += 4;

    std::shared_ptr<AMFObject> p = std::make_shared<AMFObject>(nname);
    while (count > 0) {
      auto len = p->decodeOnce(data, size - parsed, true);
      if (len == -1) {
        return -1;
      }
      data += len;
      parsed += len;
      count--;
    }
    RTMP_TRACE << "EcmaArray ";
    p->dump();
    properties_.emplace_back(std::move(p));
    break;
  }
  case kAMFDate: {
    std::shared_ptr<AMFDate> p = std::make_shared<AMFDate>(nname);
    auto len = p->decode(data, size - parsed);
    if (len == -1) {
      return -1;
    }
    data += len;
    parsed += len;
    RTMP_TRACE << "Date value:" << p->date();
    properties_.emplace_back(std::move(p));
    break;
  }
  case kAMFLongString: {
    std::shared_ptr<AMFLongString> p = std::make_shared<AMFLongString>(nname);
    auto len = p->decode(data, size - parsed);
    if (len == -1) {
      return -1;
    }
    data += len;
    parsed += len;
    RTMP_TRACE << "LongString value:" << p->str();
    properties_.emplace_back(std::move(p));
    break;
  }
  case kAMFMovieClip:
  case kAMFUndefined:
  case kAMFReference:
  case kAMFUnsupported:
  case kAMFRecordset:
  case kAMFXMLDoc:
  case kAMFTypedObject:
  case kAMFAvmplus: {
    RTMP_TRACE << " not surpport type:" << type;
    break;
  }
  }

  return parsed;
}
const AMFAnyPtr &AMFObject::property(const std::string &name) const {
  for (auto const &p : properties_) {
    if (p->name() == name) {
      return p;
    } else if (p->isObject()) {
      AMFObjectPtr obj = p->object();
      const AMFAnyPtr &p2 = obj->property(name);
      if (p2) {
        return p2;
      }
    }
  }
  return any_ptr_null;
}
const AMFAnyPtr &AMFObject::property(int index) const {
  if (index < 0 || index >= properties_.size()) {
    return any_ptr_null;
  }
  return properties_[index];
}
