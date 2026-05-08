// APC.cpp
#include "apc.h"
#include <QDebug>

void APC::execute20() { addTrack(Type::Air, TrackMode::Auto);}
void APC::execute21() { addTrack(Type::Air, TrackMode::RAM);}
void APC::execute22() { addTrack(Type::Air, TrackMode::DR);}
void APC::execute23() {}
void APC::execute24() { addTrack(Type::Surface, TrackMode::Auto);}
void APC::execute25() { addTrack(Type::Surface, TrackMode::Auto);}
void APC::execute26() { addTrack(Type::Surface, TrackMode::Auto);}
void APC::execute27() {}
void APC::execute30() { assignTrackMode(TrackMode::Auto);}
void APC::execute31() { assignTrackMode(TrackMode::RAM);}
void APC::execute32() { assignTrackMode(TrackMode::DR);}
void APC::execute33() { assignTrackMode(TrackMode::Lost);}
void APC::execute34() { assignTrackMode(TrackMode::AutomaticLost);}
void APC::execute35() { nextTrack(); }
void APC::execute36() { correct(); }
void APC::execute37() { closeControl(); }
void APC::execute40() { changeIdentity(Identity::Pending);}
void APC::execute41() { changeIdentity(Identity::PossHostile);}
void APC::execute42() { changeIdentity(Identity::PossFriend);}
void APC::execute43() { changeIdentity(Identity::ConfHostile);}
void APC::execute44() { changeIdentity(Identity::ConfFriend);}
void APC::execute45() { changeIdentity(Identity::EvalUnknown);}
void APC::execute46() { changeIdentity(Identity::Heli);}
void APC::execute47() {}
void APC::execute50() {}
void APC::execute51() {}
void APC::execute52() {}
void APC::execute53() {}
void APC::execute54() {}
void APC::execute55() {}
void APC::execute56() {}
void APC::execute57() { wipeTrack(); }
