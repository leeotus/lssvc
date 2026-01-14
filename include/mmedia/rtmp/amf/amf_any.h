#ifndef __AMF_ANY_H__
#define __AMF_ANY_H__

#include <memory>
#include <string>
#include <cstdint>

namespace lssvc::mmedia {

enum AMFDataType {
  kAMFNumber = 0,
  kAMFBoolean,
  kAMFString,
  kAMFObject,
  kAMFMovieClip, /* reserved, not used */
  kAMFNull,
  kAMFUndefined,
  kAMFReference,
  kAMFEcmaArray,
  kAMFObjectEnd,
  kAMStrictArray,
  kAMFDate,
  kAMFLongString,
  kAMFUnsupported,
  kAMFRecordset, /* reserved, not used */
  kAMFXMLDoc,
  kAMFTypedObject,
  kAMFAvmplus, /* switch to AMF3 */
  kAMFInvalid = 0xff,
};

class AMFObject;
using AMFObjectPtr = std::shared_ptr<AMFObject>;

class AMFAny : public std::enable_shared_from_this<AMFAny> {
public:
  AMFAny(const std::string &name);
  AMFAny();
  virtual ~AMFAny();

  virtual int decode(const char *data, int size, bool has = false) = 0;
  virtual const std::string &str();
  virtual bool boolean();
  virtual double number();
  virtual double date();
  virtual AMFObjectPtr object();

  virtual bool isString();
  virtual bool isNumber();
  virtual bool isBoolean();
  virtual bool isDate();
  virtual bool isObject();

  virtual void dump() const = 0;
  const std::string &name() const;
  virtual int32_t count() const;

  static int32_t encodeNumber(char *output, double val);
  static int32_t encodeString(char *output, const std::string &str);
  static int32_t encodeBoolean(char *output, bool b);
  static int32_t encodeNameNumber(char *output, const std::string &name, double val);
  static int32_t encodeNameString(char *output, const std::string &name, const std::string &val);
  static int32_t encodeNameBoolean(char *output, const std::string &name, bool val);

protected:
  static int encodeName(char *buf, const std::string &name);
  static int writeNumber(char *buf, double val);
  static std::string decodeString(const char *data);
  std::string name_;
};

}   // namespace lssvc::mmedia

#endif
