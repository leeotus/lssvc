#ifndef __LSSVC_PIPE_EVENT_H__
#define __LSSVC_PIPE_EVENT_H__

#include <memory>
#include "network/net/lssvc_event.h"

namespace lssvc::network {

class LSSEventLoop;
class LSSPipeEvent : public LSSEvent {
public:
  LSSPipeEvent(LSSEventLoop *loop);
  ~LSSPipeEvent();

  // @brief handle read event
  void onRead() override;

  // @brief handle close event
  void onClose() override;

  // @brief handle error event
  void onError(const std::string &msg) override;

  /**
   * @brief write data through pipe file
   * @param data [in]
   * @param len [in] size of the data
   */
  void write(const char *data, size_t len);

private:
  int write_fd_{-1};
};

using LSSPipeEventPtr = std::shared_ptr<LSSPipeEvent>;

}   // namespace lssvc::network

#endif
