// spc.cpp
#include "spc.h"
#include <QDebug>

void SPC::execute20() { addTrack(Type::Surface, TrackMode::Auto);}
void SPC::execute21() { addTrack(Type::Surface, TrackMode::RAM);}
void SPC::execute22() { addTrack(Type::Surface, TrackMode::DR);}
void SPC::execute23() {}
void SPC::execute24() {}
void SPC::execute25() {}
void SPC::execute26() {}
void SPC::execute27() {}
void SPC::execute30() { assignTrackMode(TrackMode::Auto);}
void SPC::execute31() { assignTrackMode(TrackMode::RAM);}
void SPC::execute32() { assignTrackMode(TrackMode::DR);}
void SPC::execute33() { assignTrackMode(TrackMode::Lost);}
void SPC::execute34() { assignTrackMode(TrackMode::AutomaticLost);}
void SPC::execute35() { nextTrack(); }
void SPC::execute36() { correct(); }
void SPC::execute37() { closeControl(); }
void SPC::execute40() { changeIdentity(Identity::Pending);}
void SPC::execute41() { changeIdentity(Identity::PossHostile);}
void SPC::execute42() { changeIdentity(Identity::PossFriend);}
void SPC::execute43() { changeIdentity(Identity::ConfHostile);}
void SPC::execute44() { changeIdentity(Identity::ConfFriend);}
void SPC::execute45() { changeIdentity(Identity::EvalUnknown);}
void SPC::execute46() { changeIdentity(Identity::Heli);}
void SPC::execute47() {}
void SPC::execute50() {}
void SPC::execute51() {}
void SPC::execute52() {}
void SPC::execute53() {}
void SPC::execute54() {}
void SPC::execute55() {}
void SPC::execute56() {}
void SPC::execute57() { wipeTrack(); }
