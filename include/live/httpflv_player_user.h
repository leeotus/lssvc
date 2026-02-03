#ifndef __FLV_PLAYER_USER_H__
#define __FLV_PLAYER_USER_H__

#include "mmedia/base/packet.h"
#include "network/net/lssvc_connection.h"
#include "player_user.h"
#include <vector>

namespace lssvc::live {

class HttpFlvPlayerUser : public PlayerUser {
public:
  explicit HttpFlvPlayerUser(const network::ConnectionPtr &ptr,
                             const StreamPtr &stream, const SessionPtr &s);

  bool postFrames();

  UserType getUserType() const;

private:
  using LiveUser::setUserType;

  bool pushFrame(mmedia::PacketPtr &pkt, bool is_header);

  bool pushFrames(std::vector<mmedia::PacketPtr> &list);

  void pushFlvHttpHeader();

  bool http_header_sent_{false};
};

} // namespace lssvc::live

#endif
