#ifndef OWNCURS_H
#define OWNCURS_H

#include <QObject>
#include <QPair>
#include <qfloat16.h>

#include "entities/cursorEntity.h"  // Usa setters y tipos de CursorEntity
#include "commandContext.h"
#include "obmHandler.h"
#define ORIGIN QPair<qfloat16, qfloat16>(qfloat16(0.0), qfloat16(0.0))

class OwnCurs : public QObject {
    Q_OBJECT
public:
    explicit OwnCurs(CommandContext* ctx, OBMHandler* newObm, QObject* parent = nullptr);
    bool isActive();

public slots:
    void cuOrOffCent();    // ← faltaba 'void'
    void cuOrCent();
    void ownCursActive(bool value);

    void updateHandwheel(const QPair<qfloat16, qfloat16>& update);     // ← const& para evitar copia

private:
    std::optional<std::reference_wrapper<CursorEntity>> cursorRef;
    CommandContext* ctx    = nullptr;
    OBMHandler*     obm    = nullptr;
    bool            ownCursAct = false;

};

#endif // OWNCURS_H
