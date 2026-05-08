// LINCO.cpp
#include "linco.h"
#include <QDebug>

void LINCO::execute20() { addTrack(Type::Surface, TrackMode::Auto);}
void LINCO::execute21() { addTrack(Type::Surface, TrackMode::RAM);}
void LINCO::execute22() { addTrack(Type::Surface, TrackMode::DR);}
void LINCO::execute23() {}
void LINCO::execute24() {}
void LINCO::execute25() {}
void LINCO::execute26() {}
void LINCO::execute27() {}
void LINCO::execute30() { assignTrackMode(TrackMode::Auto);}
void LINCO::execute31() { assignTrackMode(TrackMode::RAM);}
void LINCO::execute32() { assignTrackMode(TrackMode::DR);}
void LINCO::execute33() { assignTrackMode(TrackMode::Lost);}
void LINCO::execute34() { assignTrackMode(TrackMode::AutomaticLost);}
void LINCO::execute35() { nextTrack(); }
void LINCO::execute36() { correct(); }
void LINCO::execute37() { closeControl(); }
void LINCO::execute40() { changeIdentity(Identity::Pending);}
void LINCO::execute41() { changeIdentity(Identity::PossHostile);}
void LINCO::execute42() { changeIdentity(Identity::PossFriend);}
void LINCO::execute43() { changeIdentity(Identity::ConfHostile);}
void LINCO::execute44() { changeIdentity(Identity::ConfFriend);}
void LINCO::execute45() { changeIdentity(Identity::EvalUnknown);}
void LINCO::execute46() { changeIdentity(Identity::Heli);}
void LINCO::execute47() {}
void LINCO::execute50() {}
void LINCO::execute51() {}
void LINCO::execute52() {}
void LINCO::execute53() {}
void LINCO::execute54() {}
void LINCO::execute55() {}
void LINCO::execute56() {}
void LINCO::execute57() { wipeTrack(); }
