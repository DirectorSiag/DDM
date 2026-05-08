// ASW.cpp
#include "asw.h"
#include <QDebug>

void ASW::execute20() { addTrack(Type::Subsurface, TrackMode::DR);}
void ASW::execute21() { addTrack(Type::Subsurface, TrackMode::Auto);}
void ASW::execute22() { addTrack(Type::Air, TrackMode::DR);}
void ASW::execute23() {}
void ASW::execute24() { addTrack(Type::ESM, TrackMode::Auto);}
void ASW::execute25() { addTrack(Type::ESM, TrackMode::Auto);}
void ASW::execute26() { assignTrackMode(TrackMode::Lost);}
void ASW::execute27() { assignTrackMode(TrackMode::AutomaticLost);}
void ASW::execute30() { changeIdentity(Identity::Pending);}
void ASW::execute31() { changeIdentity(Identity::PossHostile);}
void ASW::execute32() { changeIdentity(Identity::PossFriend);}
void ASW::execute33() { changeIdentity(Identity::ConfHostile);}
void ASW::execute34() { changeIdentity(Identity::ConfFriend);}
void ASW::execute35() { changeIdentity(Identity::EvalUnknown); }
void ASW::execute36() { correct(); }
void ASW::execute37() { closeControl(); }
void ASW::execute40() { }
void ASW::execute41() { }
void ASW::execute42() { }
void ASW::execute43() { }
void ASW::execute44() { }
void ASW::execute45() { }
void ASW::execute46() { }
void ASW::execute47() {}
void ASW::execute50() {}
void ASW::execute51() {}
void ASW::execute52() {}
void ASW::execute53() {}
void ASW::execute54() {}
void ASW::execute55() {}
void ASW::execute56() {}
void ASW::execute57() { wipeTrack(); }
