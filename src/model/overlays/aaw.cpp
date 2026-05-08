// AAW.cpp
#include "aaw.h"
#include <QDebug>

void AAW::execute20() { addTrack(Type::Air, TrackMode::Auto);}
void AAW::execute21() { addTrack(Type::Air, TrackMode::RAM);}
void AAW::execute22() { addTrack(Type::Air, TrackMode::DR);}
void AAW::execute23() {}
void AAW::execute24() { addTrack(Type::Surface, TrackMode::Auto);}
void AAW::execute25() { addTrack(Type::Surface, TrackMode::Auto);}
void AAW::execute26() { addTrack(Type::Surface, TrackMode::Auto);}
void AAW::execute27() {}
void AAW::execute30() { assignTrackMode(TrackMode::Auto);}
void AAW::execute31() { assignTrackMode(TrackMode::RAM);}
void AAW::execute32() { assignTrackMode(TrackMode::DR);}
void AAW::execute33() { assignTrackMode(TrackMode::Lost);}
void AAW::execute34() { assignTrackMode(TrackMode::AutomaticLost);}
void AAW::execute35() { nextTrack(); }
void AAW::execute36() { correct(); }
void AAW::execute37() { closeControl(); }
void AAW::execute40() { changeIdentity(Identity::Pending);}
void AAW::execute41() { changeIdentity(Identity::PossHostile);}
void AAW::execute42() { changeIdentity(Identity::PossFriend);}
void AAW::execute43() { changeIdentity(Identity::ConfHostile);}
void AAW::execute44() { changeIdentity(Identity::ConfFriend);}
void AAW::execute45() { changeIdentity(Identity::EvalUnknown);}
void AAW::execute46() { changeIdentity(Identity::Heli);}
void AAW::execute47() {}
void AAW::execute50() {}
void AAW::execute51() {}
void AAW::execute52() {}
void AAW::execute53() {}
void AAW::execute54() {}
void AAW::execute55() {}
void AAW::execute56() {}
void AAW::execute57() { wipeTrack(); }
