// qek.h
#pragma once
#include "commandContext.h"
#include "obmHandler.h"
#include <QString>
#include <QDebug>

using Type      = TrackData::Type;
using Identity  = TrackData::Identity;
using TrackMode = TrackData::TrackMode;

enum class Qek : int {
    QEK_20 = 20,  QEK_21 = 21,  QEK_22 = 22,  QEK_23 = 23,  QEK_24 = 24,  QEK_25 = 25,  QEK_26 = 26,  QEK_27 = 27,
    QEK_30 = 30,  QEK_31 = 31,  QEK_32 = 32,  QEK_33 = 33,  QEK_34 = 34,  QEK_35 = 35,  QEK_36 = 36,  QEK_37 = 37,
    QEK_40 = 40,  QEK_41 = 41,  QEK_42 = 42,  QEK_43 = 43,  QEK_44 = 44,  QEK_45 = 45,  QEK_46 = 46,  QEK_47 = 47,
    QEK_50 = 50,  QEK_51 = 51,  QEK_52 = 52,  QEK_53 = 53,  QEK_54 = 54,  QEK_55 = 55,  QEK_56 = 56,  QEK_57 = 57,
    CantQEK
};

inline bool qekFromString(const QString& s, Qek& out) {
    // acepta "QEK_20" → 20
    if (!s.startsWith("QEK_")) return false;
    bool ok = false;
    int val = s.mid(4).toInt(&ok);
    if (!ok) return false;
    out = static_cast<Qek>(val);
    return true;
}

// Base abstracta: un método por botón (no-op por defecto)
class QEK {
public:
    virtual ~QEK() = default;

    // 20..57 (los que uses). Default: no-op
    virtual void execute20() {}
    virtual void execute21() {}
    virtual void execute22() {}
    virtual void execute23() {}
    virtual void execute24() {}
    virtual void execute25() {}
    virtual void execute26() {}
    virtual void execute27() {}
    virtual void execute30() {}
    virtual void execute31() {}
    virtual void execute32() {}
    virtual void execute33() {}
    virtual void execute34() {}
    virtual void execute35() {}
    virtual void execute36() {}
    virtual void execute37() {}
    virtual void execute40() {}
    virtual void execute41() {}
    virtual void execute42() {}
    virtual void execute43() {}
    virtual void execute44() {}
    virtual void execute45() {}
    virtual void execute46() {}
    virtual void execute47() {}
    virtual void execute50() {}
    virtual void execute51() {}
    virtual void execute52() {}
    virtual void execute53() {}
    virtual void execute54() {}
    virtual void execute55() {}
    virtual void execute56() {}
    virtual void execute57() {}

    //quick
    void addTrack(Type type, TrackMode mode) {
        const auto pos = obmHandler->getPosition(); // QPair<float,float>
        ctx->emplaceTrackFront(
            ctx->nextTrackId++,     // id
            type,                   // type
            Identity::Pending,      // identidad inicial
            mode,                   // modo
            pos.first,              // x
            pos.second              // y
            );
    }

    bool wipeTrack() {
        if (!ctx || !obmHandler) return false;

        Track* t = obmHandler->OBMAssociationProcess(ctx);
        if (!t) return false;

        const int id = t->getId();
        const bool ok = ctx->eraseTrackById(id);
        if (!ok) {
            qWarning() << "QEK::wipeTrack: no se pudo borrar id=" << id;
        }
        return ok;
    }

    bool assignTrackMode(TrackMode mode) {
        if (!ctx || !obmHandler) return false;

        Track* t = obmHandler->OBMAssociationProcess(ctx); // busca (más nuevo primero)
        if (!t) return false;

        t->setTrackMode(mode);  // setter existente en Track
        return true;
    }

    bool changeIdentity(Identity identity) {
        if (!ctx || !obmHandler) return false;

        Track* t = obmHandler->OBMAssociationProcess(ctx);
        if (!t) return false;           // no hay track cerca
        t->setIdentity(identity);       // setter expuesto por Track
        return true;
    }

    bool closeControl(){
        if(!ctx || !obmHandler) return false;

        Track* t= obmHandler->OBMAssociationProcess(ctx);
        if(!t) return false;
        closeControlTrack = t;
        obmHandler->setPosition({t->getX(),t->getY()});
        return true;
    }

    bool correct(){
        if(!closeControlTrack) return false;
        const auto pos = obmHandler->getPosition();
        closeControlTrack->setX(pos.first);
        closeControlTrack->setY(pos.second);
        return true;
    }

    bool nextTrack() {
        if(!closeControlTrack) return false;
        Track* t = ctx->getNextTrackById(closeControlTrack->getId());
        if(!t) return false;
        closeControlTrack = t;
        obmHandler->setPosition({t->getX(),t->getY()});
        return true;
    }

    void setContext(CommandContext * ctx){this->ctx = ctx;}
    void setOBMHandler(OBMHandler* oh){this->obmHandler = oh;}

private:
    CommandContext* ctx;
    OBMHandler* obmHandler;
    Track *closeControlTrack;
};
