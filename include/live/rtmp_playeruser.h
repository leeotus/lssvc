#ifndef __RTMP_PLAYERUSER_H__
#define __RTMP_PLAYERUSER_H__

#include "player_user.h"
#include <vector>

namespace lssvc::live {

class RtmpPlayerUser : public PlayerUser {
public:
  explicit RtmpPlayerUser(const network::ConnectionPtr &conn,
                          const StreamPtr &stream, const SessionPtr &s);

  bool postFrames() override;
  UserType getUserType() const;
private:
  using LiveUser::setUserType;

  bool pushFrame(mmedia::PacketPtr &pkt, bool is_header);
  bool pushFrames(std::vector<mmedia::PacketPtr> &list);
};

};

#endif
