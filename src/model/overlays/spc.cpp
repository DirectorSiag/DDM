// spc.cpp
#include "spc.h"
#include <QDebug>

void SPC::execute20() { addTrack(Type::SPC, TrackMode::Auto); }
void SPC::execute21() { addTrack(Type::SPC, TrackMode::RAM); }
void SPC::execute22() { addTrack(Type::SPC, TrackMode::DR); }
void SPC::execute23() { addTrack(Type::SPC, TrackMode::Auto); }
void SPC::execute24() { addTrack(Type::SPC, TrackMode::Auto); }
void SPC::execute25() { addTrack(Type::SPC, TrackMode::Auto); }
void SPC::execute26() { addTrack(Type::SPC, TrackMode::Auto); }
void SPC::execute27() { addTrack(Type::SPC, TrackMode::Auto); }
void SPC::execute30() {}
void SPC::execute31() {}
void SPC::execute32() {}
void SPC::execute33() {}
void SPC::execute34() {}
void SPC::execute35() {}
void SPC::execute36() {}
void SPC::execute37() {}
void SPC::execute40() { addTrackWithIdentity(Type::SPC, TrackMode::Auto, Identity::Pending); }
void SPC::execute41() { addTrackWithIdentity(Type::SPC, TrackMode::Auto, Identity::PossHostile); }
void SPC::execute42() { addTrackWithIdentity(Type::SPC, TrackMode::Auto, Identity::PossFriend); }
void SPC::execute43() { addTrackWithIdentity(Type::SPC, TrackMode::Auto, Identity::ConfHostile); }
void SPC::execute44() { addTrackWithIdentity(Type::SPC, TrackMode::Auto, Identity::ConfFriend); }
void SPC::execute45() { addTrackWithIdentity(Type::SPC, TrackMode::Auto, Identity::EvalUnknown); }
void SPC::execute46() { addTrackWithIdentity(Type::SPC, TrackMode::Auto, Identity::Heli); }
void SPC::execute47() {}
void SPC::execute50() {}
void SPC::execute51() {}
void SPC::execute52() {}
void SPC::execute53() {}
void SPC::execute54() {}
void SPC::execute55() {}
void SPC::execute56() {}
void SPC::execute57() { wipeTrack(); }
