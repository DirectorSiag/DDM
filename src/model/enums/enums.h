#ifndef TYPESDATA_H
#define TYPESDATA_H

#include <QObject>    // Q_GADGET / Q_ENUM
#include <QMetaEnum>
#include <QString>

class TrackData {
    Q_GADGET

public:
    // ==== Enums (estilo ButtonsData: NO "enum class") ====
    enum Type { Air, Surface, Subsurface, RP, ESM };
    Q_ENUM(Type)

    enum Identity { Pending, PossHostile, PossFriend, ConfHostile, ConfFriend, EvalUnknown, Heli };
    Q_ENUM(Identity)

    enum TrackMode { FC1, FC2, FC3, FC4, FC5, Auto, TentativeAuto, AutomaticLost, RAM, DR, Lost, Blank };
    Q_ENUM(TrackMode)

    // ==== Helpers: de enum a QString sin switch ====
    static inline QString toQString(Type v)       { return enumName(v, "Unknown"); }
    static inline QString toQString(Identity v)   { return enumName(v, "Unknown"); }
    static inline QString toQString(TrackMode v)  { return enumName(v, "Unknown"); }

private:
    template <typename E>
    static inline QString enumName(E v, const char* fallback) {
        const QMetaEnum me = QMetaEnum::fromType<E>();
        const char* key = me.valueToKey(static_cast<int>(v));
        return key ? QString::fromLatin1(key) : QString::fromLatin1(fallback);
    }
};

#endif // TYPESDATA_H
