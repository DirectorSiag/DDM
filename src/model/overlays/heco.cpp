// HECO.cpp
#include "heco.h"
#include <QDebug>

void HECO::execute20() { addTrack(Type::Air, TrackMode::Auto);}
void HECO::execute21() { addTrack(Type::Air, TrackMode::RAM);}
void HECO::execute22() { addTrack(Type::Air, TrackMode::DR);}
void HECO::execute23() {}
void HECO::execute24() {}
void HECO::execute25() {}
void HECO::execute26() {}
void HECO::execute27() {}
void HECO::execute30() { assignTrackMode(TrackMode::Auto);}
void HECO::execute31() { assignTrackMode(TrackMode::RAM);}
void HECO::execute32() { assignTrackMode(TrackMode::DR);}
void HECO::execute33() { assignTrackMode(TrackMode::Lost);}
void HECO::execute34() { assignTrackMode(TrackMode::AutomaticLost);}
void HECO::execute35() { nextTrack(); }
void HECO::execute36() { correct(); }
void HECO::execute37() { closeControl(); }
void HECO::execute40() { changeIdentity(Identity::Pending);}
void HECO::execute41() { changeIdentity(Identity::PossHostile);}
void HECO::execute42() { changeIdentity(Identity::PossFriend);}
void HECO::execute43() { changeIdentity(Identity::ConfHostile);}
void HECO::execute44() { changeIdentity(Identity::ConfFriend);}
void HECO::execute45() { changeIdentity(Identity::EvalUnknown);}
void HECO::execute46() { changeIdentity(Identity::Heli);}
void HECO::execute47() {}
void HECO::execute50() {}
void HECO::execute51() {}
void HECO::execute52() {}
void HECO::execute53() {}
void HECO::execute54() {}
void HECO::execute55() {}
void HECO::execute56() {}
void HECO::execute57() { wipeTrack(); }
