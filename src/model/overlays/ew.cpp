// EW.cpp
#include "ew.h"
#include <QDebug>

void EW::execute20() { addTrack(Type::Air, TrackMode::Auto);}
void EW::execute21() { addTrack(Type::Air, TrackMode::RAM);}
void EW::execute22() { addTrack(Type::Air, TrackMode::DR);}
void EW::execute23() {}
void EW::execute24() { addTrack(Type::Surface, TrackMode::Auto);}
void EW::execute25() { addTrack(Type::Surface, TrackMode::Auto);}
void EW::execute26() { addTrack(Type::Surface, TrackMode::Auto);}
void EW::execute27() {}
void EW::execute30() { assignTrackMode(TrackMode::Auto);}
void EW::execute31() { assignTrackMode(TrackMode::RAM);}
void EW::execute32() { assignTrackMode(TrackMode::DR);}
void EW::execute33() { assignTrackMode(TrackMode::Lost);}
void EW::execute34() { assignTrackMode(TrackMode::AutomaticLost);}
void EW::execute35() { nextTrack(); }
void EW::execute36() { correct(); }
void EW::execute37() { closeControl(); }
void EW::execute40() { changeIdentity(Identity::Pending);}
void EW::execute41() { changeIdentity(Identity::PossHostile);}
void EW::execute42() { changeIdentity(Identity::PossFriend);}
void EW::execute43() { changeIdentity(Identity::ConfHostile);}
void EW::execute44() { changeIdentity(Identity::ConfFriend);}
void EW::execute45() { changeIdentity(Identity::EvalUnknown);}
void EW::execute46() { changeIdentity(Identity::Heli);}
void EW::execute47() {}
void EW::execute50() {}
void EW::execute51() {}
void EW::execute52() {}
void EW::execute53() {}
void EW::execute54() {}
void EW::execute55() {}
void EW::execute56() {}
void EW::execute57() { wipeTrack(); }
